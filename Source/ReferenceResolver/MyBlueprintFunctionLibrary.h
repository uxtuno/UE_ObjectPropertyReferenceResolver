// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyBlueprintFunctionLibrary.generated.h"



USTRUCT()
struct REFERENCERESOLVER_API FReferenceResolveRulesRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	// �v���p�e�B�p�X
	// ��: "PropertyA.MemberA"
	UPROPERTY(EditAnywhere)
	FString PropertyPath;

	// �A�Z�b�g�������������
	// ���K�\���Ŏw�肷��
	UPROPERTY(EditAnywhere)
	FString AssetMatchRule;
};


USTRUCT(BlueprintType)
struct REFERENCERESOLVER_API FTestStruct
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere)
	TObjectPtr<UMaterial> RedMaterial;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UMaterial> GreenMaterial;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UMaterial> BlueMaterial;
};


UCLASS(Blueprintable)
class REFERENCERESOLVER_API UTestObject : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FTestStruct Members;
};

/**
 * 
 */
UCLASS()
class REFERENCERESOLVER_API UMyBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static void ResolveReference(UObject* WorldContextObject, UObject* Target, class UDataTable* RulesDataTable, const FString& AssetDirectoryPath);

private:
	struct FPropertyData
	{
		FPropertyData(const FProperty* InProperty, const void* InContainer)
			: Property(InProperty)
			, Container(InContainer)
		{
		}

		const FProperty* Property;
		const void* Container;
	};

	// �v���p�e�B�}�b�v���\�z����
	static TMap<FString, FPropertyData> BuildPropertyMap(UObject* Target);
};
