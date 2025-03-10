// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once


#include "CustomizableObjectIdentifier.generated.h"

USTRUCT(BlueprintType)
struct CUSTOMIZABLEOBJECT_API FCustomizableObjectIdPair
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(Category = CustomizableObject, BlueprintReadOnly, EditDefaultsOnly)
	FString CustomizableObjectGroupName;

	UPROPERTY(Category = CustomizableObject, BlueprintReadOnly, EditDefaultsOnly)
	FString CustomizableObjectName;

	FCustomizableObjectIdPair() { };
	FCustomizableObjectIdPair(FString ObjectGroupName, FString ObjectName);

	bool operator ==(const FCustomizableObjectIdPair& Other) const
	{
		return CustomizableObjectGroupName == Other.CustomizableObjectGroupName && CustomizableObjectName == Other.CustomizableObjectName;
	}

	friend FArchive& operator <<(FArchive& Ar, FCustomizableObjectIdPair& IdPair)
	{
		Ar << IdPair.CustomizableObjectGroupName;
		Ar << IdPair.CustomizableObjectName;

		return Ar;
	}
};

