// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Internationalization/Text.h"
#include "NiagaraConvertInPlaceUtilityBase.generated.h"

class UNiagaraScript;
class UNiagaraNodeFunctionCall;
class UNiagaraClipboardContent;
class UNiagaraStackScriptHierarchyRoot;

UCLASS(Abstract, MinimalAPI)
class UNiagaraConvertInPlaceUtilityBase : public UObject
{
	GENERATED_BODY()
public:
	virtual bool Convert(UNiagaraScript* InOldScript, UNiagaraClipboardContent* InOldClipboardContent, UNiagaraScript* InNewScript, UNiagaraStackScriptHierarchyRoot* InHierarchyRoot, UNiagaraClipboardContent* InNewClipboardContent, UNiagaraNodeFunctionCall* InCallingNode, FText& OutMessage) { return true; };
};
