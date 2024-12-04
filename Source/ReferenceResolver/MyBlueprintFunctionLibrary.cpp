// Fill out your copyright notice in the Description page of Project Settings.


#include "MyBlueprintFunctionLibrary.h"
#include <regex>
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet/KismetSystemLibrary.h"


void UMyBlueprintFunctionLibrary::ResolveReference(UObject* WorldContextObject, UObject* Target, UDataTable* RulesDataTable, const FString& AssetDirectoryPath)
{
	if (!WorldContextObject || !Target || !RulesDataTable)
	{
		return;
	}

	// Blueprintの場合はコンパイル後に生成されるクラスを対象にする必要がある
	if (UBlueprint* BluePrint = Cast<UBlueprint>(Target))
	{
		Target = BluePrint->GeneratedClass->GetDefaultObject();
	}
	TMap<FString, FPropertyData> PropertyMap = BuildPropertyMap(Target);

	TArray<FReferenceResolveRulesRow*> Rows;
	RulesDataTable->GetAllRows(TEXT(""), Rows);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// 指定したディレクトリ以下のアセットを取得
	TArray<FAssetData> AssetDataList;
	AssetRegistry.GetAssetsByPath(*AssetDirectoryPath, AssetDataList, true, true);

	int32 Transaction = UKismetSystemLibrary::BeginTransaction(TEXT("ResolveReference"), FText::FromString(TEXT("オブジェクトプロパティの参照解決")), Target);
	if (Transaction < 0)
	{
		return;
	}
	UKismetSystemLibrary::TransactObject(Target);

	for (const FReferenceResolveRulesRow* Row : Rows)
	{
		if (!Row)
		{
			continue;
		}

		FString PropertyPath = Row->PropertyPath;
		FString AssetMatchRule = Row->AssetMatchRule;

		// 正規表現でAssetを検索
		std::regex Regex(TCHAR_TO_UTF8(*AssetMatchRule));
		FAssetData* MatchAssetData = AssetDataList.FindByPredicate([&Regex](const FAssetData& AssetData)
			{
				return std::regex_search(TCHAR_TO_UTF8(*AssetData.PackageName.ToString()), Regex);
			});

		if (MatchAssetData)
		{
			UObject* Asset = MatchAssetData->GetAsset();
			if (Asset)
			{
				// リフレクションを使用して、指定したプロパティにアセットをセットする
				FPropertyData* PropertyData = PropertyMap.Find(PropertyPath);
				if (PropertyData)
				{
					if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(PropertyData->Property))
					{
						// プロパティにアセット参照をセット
						ObjectProperty->SetObjectPropertyValue(const_cast<void*>(PropertyData->Container), Asset);
					}
				}
			}
		}
	}

	// プロパティを変更したことをマーク
	if (UBlueprint* Blueprint = Cast<UBlueprint>(Target))
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	}
	else
	{
		Target->MarkPackageDirty();
	}

	UKismetSystemLibrary::EndTransaction();
}


TMap<FString, UMyBlueprintFunctionLibrary::FPropertyData> UMyBlueprintFunctionLibrary::BuildPropertyMap(UObject* Target)
{
	TMap<FString, FPropertyData> PropertyMap;

	for (TPropertyValueIterator<FObjectProperty> ObjectPropertyIterator(Target->GetClass(), Target); ObjectPropertyIterator; ++ObjectPropertyIterator)
	{
		const FObjectProperty* ObjectProperty = ObjectPropertyIterator.Key();
		FString PropertyPath;

		// PropertyA.PropertyB.PropertyC... という形式に変換する
		TArray<const FProperty*> PropertyChain;
		ObjectPropertyIterator.GetPropertyChain(PropertyChain);
		for (const FProperty* Property : PropertyChain)
		{
			PropertyPath = Property->GetName() + (PropertyPath.IsEmpty() ? "" : ".") + PropertyPath;
		}

		PropertyMap.Emplace(PropertyPath, FPropertyData(ObjectPropertyIterator.Key(), ObjectPropertyIterator.Value()));
	}

	return PropertyMap;
}

