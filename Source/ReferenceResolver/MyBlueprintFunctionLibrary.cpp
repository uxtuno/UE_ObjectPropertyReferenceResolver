// Fill out your copyright notice in the Description page of Project Settings.


#include "MyBlueprintFunctionLibrary.h"
#include <regex>
#include "AssetRegistry/AssetRegistryModule.h"


void UMyBlueprintFunctionLibrary::ResolveReference(UObject* WorldContextObject, UObject* Target, UDataTable* RulesDataTable, const FString& AssetDirectoryPath)
{
	if (!WorldContextObject || !Target || !RulesDataTable)
	{
		return;
	}

	// Blueprint�̏ꍇ�̓R���p�C����ɐ��������N���X��Ώۂɂ���K�v������
	if (UBlueprint* BluePrint = Cast<UBlueprint>(Target))
	{
		Target = BluePrint->GeneratedClass->GetDefaultObject();
	}
	TMap<FString, FPropertyData> PropertyMap = BuildPropertyMap(Target);

	TArray<FReferenceResolveRulesRow*> Rows;
	RulesDataTable->GetAllRows(TEXT(""), Rows);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// �w�肵���f�B���N�g���ȉ��̃A�Z�b�g���擾
	TArray<FAssetData> AssetDataList;
	AssetRegistry.GetAssetsByPath(*AssetDirectoryPath, AssetDataList, true, true);

	for (const FReferenceResolveRulesRow* Row : Rows)
	{
		if (!Row)
		{
			continue;
		}

		FString PropertyPath = Row->PropertyPath;
		FString AssetMatchRule = Row->AssetMatchRule;

		// ���K�\����Asset������
		std::regex Regex(TCHAR_TO_UTF8(*Row->AssetMatchRule));
		FAssetData* MatchAssetData = AssetDataList.FindByPredicate([&Regex](const FAssetData& AssetData)
			{
				return std::regex_search(TCHAR_TO_UTF8(*AssetData.PackageName.ToString()), Regex);
			});

		if (MatchAssetData)
		{
			UObject* Asset = MatchAssetData->GetAsset();
			if (Asset)
			{
				// ���t���N�V�������g�p���āA�w�肵���v���p�e�B�ɃA�Z�b�g���Z�b�g����
				FPropertyData* PropertyData = PropertyMap.Find(PropertyPath);
				if (PropertyData)
				{
					if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(PropertyData->Property))
					{
						ObjectProperty->SetObjectPropertyValue(const_cast<void*>(PropertyData->Container), Asset);
					}
				}
			}
		}
	}
}


TMap<FString, UMyBlueprintFunctionLibrary::FPropertyData> UMyBlueprintFunctionLibrary::BuildPropertyMap(UObject* Target)
{
	TMap<FString, FPropertyData> PropertyMap;

	for (TPropertyValueIterator<FObjectProperty> ObjectPropertyIterator(Target->GetClass(), Target); ObjectPropertyIterator; ++ObjectPropertyIterator)
	{
		const FObjectProperty* ObjectProperty = ObjectPropertyIterator.Key();
		FString PropertyPath;

		// PropertyA.PropertyB.PropertyC... �Ƃ����`���ɕϊ�����
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

