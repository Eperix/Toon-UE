// Copyright Epic Games, Inc. All Rights Reserved.

#include "SkeletalMeshComponentInstanceData.h"

#include "Components/SkeletalMeshComponent.h"


FSkeletalMeshComponentInstanceData::FSkeletalMeshComponentInstanceData(const USkeletalMeshComponent* SourceComponent)
	: FSceneComponentInstanceData(SourceComponent)
	, bUpdateAnimationInEditor(0)
	, bUpdateClothInEditor(0)

{
#if WITH_EDITOR
	// Only Blueprint components would reset tansient values when the construction script is re-run.
	// Hence, we only need to apply instance cache for Blueprint created components.
	const bool bIsBlueprintCreatedComponent = SourceComponent->CreationMethod == EComponentCreationMethod::SimpleConstructionScript
		|| SourceComponent->CreationMethod == EComponentCreationMethod::UserConstructionScript;
	if (bIsBlueprintCreatedComponent)
	{
		bUpdateAnimationInEditor = SourceComponent->GetUpdateAnimationInEditor();
		bUpdateClothInEditor = SourceComponent->GetUpdateClothInEditor();
	}
#endif // WITH_EDITOR
}

bool FSkeletalMeshComponentInstanceData::ContainsData() const
{
	return bUpdateAnimationInEditor || bUpdateClothInEditor;
}

void FSkeletalMeshComponentInstanceData::ApplyToComponent(UActorComponent* Component, const ECacheApplyPhase CacheApplyPhase)
{
	Super::ApplyToComponent(Component, CacheApplyPhase);

#if WITH_EDITOR
	if (USkeletalMeshComponent * SkeletalMesh = Cast<USkeletalMeshComponent>(Component))
	{
		SkeletalMesh->SetUpdateAnimationInEditor(bUpdateAnimationInEditor);
		SkeletalMesh->SetUpdateClothInEditor(bUpdateClothInEditor);
	}
#endif // WITH_EDITOR
}
