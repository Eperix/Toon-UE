// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MuCOE/Nodes/CustomizableObjectNodeSwitchBase.h"

#include "CustomizableObjectNodeColorSwitch.generated.h"


UCLASS()
class CUSTOMIZABLEOBJECTEDITOR_API UCustomizableObjectNodeColorSwitch : public UCustomizableObjectNodeSwitchBase
{
public:
	GENERATED_BODY()

	// UCustomizableObjectNode interface
	virtual void BackwardsCompatibleFixup(int32 CustomizableObjectCustomVersion) override;

	// UCustomizableObjectNodeSwitchBase interface
	virtual FName GetCategory() const override;
};

