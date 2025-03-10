// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ChaosVDCharacterGroundConstraintDataProviderInterface.h"
#include "ChaosVDCollisionDataProviderInterface.h"
#include "ChaosVDGeometryDataComponent.h"
#include "ChaosVDSceneObjectBase.h"
#include "Chaos/Core.h"
#include "DataWrappers/ChaosVDParticleDataWrapper.h"
#include "DataWrappers/ChaosVDCollisionDataWrappers.h"
#include "GameFramework/Actor.h"
#include "Interfaces/ChaosVDSelectableObject.h"
#include "Visualizers/IChaosVDParticleVisualizationDataProvider.h"

#include "ChaosVDParticleActor.generated.h"

class FChaosVDParticleActorCustomization;
enum class EChaosVDParticleDataVisualizationFlags : uint32;
class FChaosVDScene;
struct FChaosVDParticleDebugData;
class UMeshComponent;
class USceneComponent;
class UStaticMeshComponent;
class UStaticMesh;

namespace Chaos
{
	class FImplicitObject;
}

/** Options flags to control how geometry is updated in a ChaosVDActor */
UENUM()
enum class EChaosVDActorGeometryUpdateFlags : int32
{
	None = 0,
	ForceUpdate = 1 << 0
};
ENUM_CLASS_FLAGS(EChaosVDActorGeometryUpdateFlags)

DECLARE_DELEGATE(FChaosVDParticleDataUpdatedDelegate)

UENUM()
enum class EChaosVDHideParticleFlags
{
	None = 0,
	HiddenByVisualizationFlags = 1 << 0,
	HiddenBySceneOutliner = 1 << 1,
	HiddenByActiveState = 1 << 2,
	HiddenBySolverVisibility = 1 << 3,
	
};
ENUM_CLASS_FLAGS(EChaosVDHideParticleFlags)

/** Actor used to represent a Chaos Particle in the Visual Debugger's world */
UCLASS(HideCategories=(Transform))
class AChaosVDParticleActor : public AActor, public IChaosVDParticleVisualizationDataProvider,
								public FChaosVDSceneObjectBase, public IChaosVDCollisionDataProviderInterface, public IChaosVDGeometryOwnerInterface, public IChaosVDSelectableObject,
								public IChaosVDCharacterGroundConstraintDataProviderInterface
{
	GENERATED_BODY()

public:
	AChaosVDParticleActor(const FObjectInitializer& ObjectInitializer);

	void UpdateFromRecordedParticleData(const TSharedPtr<FChaosVDParticleDataWrapper>& InRecordedData, const Chaos::FRigidTransform3& SimulationTransform);

	void UpdateGeometry(const Chaos::FConstImplicitObjectPtr& InImplicitObject, EChaosVDActorGeometryUpdateFlags OptionsFlags = EChaosVDActorGeometryUpdateFlags::None);

	void UpdateGeometry(uint32 NewGeometryHash, EChaosVDActorGeometryUpdateFlags OptionsFlags = EChaosVDActorGeometryUpdateFlags::None);

	virtual void Destroyed() override;

	virtual TSharedPtr<const FChaosVDParticleDataWrapper> GetParticleData() override { return ParticleDataPtr; }
	
#if WITH_EDITOR
	virtual bool IsSelectedInEditor() const override;
	virtual void SetIsTemporarilyHiddenInEditor(bool bIsHidden) override;
#endif

	void UpdateGeometryComponentsVisibility();
	void UpdateGeometryColors();

	/** Changes the active state of this CVD Particle Actor */
	void SetIsActive(bool bNewActive);

	void AddHiddenFlag(EChaosVDHideParticleFlags Flag);

	void RemoveHiddenFlag(EChaosVDHideParticleFlags Flag);

	EChaosVDHideParticleFlags GetHideFlags() const { return HideParticleFlags; }

	bool IsVisible() const
	{
		return HideParticleFlags == EChaosVDHideParticleFlags::None;
	}

	/** Returns true if this particle actor is active - Inactive Particle actors are still in the world but with outdated data
	 * and hidden from the viewport and outliner. They represent particles that were destroyed.
	 */
	bool IsActive() const { return bIsActive; }

	virtual FBox GetComponentsBoundingBox(bool bNonColliding, bool bIncludeFromChildActors) const override;

	// BEGIN IChaosVDCollisionDataProvider Interface
	virtual TConstArrayView<TSharedPtr<FChaosVDParticlePairMidPhase>> GetCollisionData() override;
	virtual bool HasCollisionData() override;
	virtual FName GetProviderName() override;
	// END IChaosVDCollisionDataProvider Interface

	// BEGIN IChaosVDCharacterGroundConstraintDataProviderInterface
	virtual void GetCharacterGroundConstraintData(TArray<TSharedPtr<FChaosVDCharacterGroundConstraint>>& OutConstraintsFound) override;
	virtual bool HasCharacterGroundConstraintData() override;
	// END IChaosVDCharacterGroundConstraintDataProviderInterface

	void SetIsServerParticle(bool bNewIsServer) { bIsServer = bNewIsServer; }
	bool GetIsServerParticle() const { return bIsServer; }

	void UpdateMeshInstancesSelectionState();
	virtual void PushSelectionToProxies() override;

	FChaosVDParticleDataUpdatedDelegate& OnParticleDataUpdated() { return ParticleDataUpdatedDelegate; };

	// BEGIN IChaosVDGeometryOwner Interface
	virtual TConstArrayView<TSharedPtr<FChaosVDMeshDataInstanceHandle>> GetMeshInstances() const override { return MeshDataHandles; }
	virtual void SetSelectedMeshInstance(const TWeakPtr<FChaosVDMeshDataInstanceHandle>& GeometryInstanceToSelect) override;
	virtual TWeakPtr<FChaosVDMeshDataInstanceHandle> GetSelectedMeshInstance() const override { return CurrentSelectedGeometryInstance; }
	virtual void HandleNewGeometryLoaded(uint32 GeometryID, const Chaos::FConstImplicitObjectPtr& InGeometryData) override;
	// END IChaosVDGeometryOwner Interface
	
	// BEGIN IChaosVDSelectableObject Interface
	virtual void HandleSelected() override;
	virtual void HandleDeSelected() override;
	// END IChaosVDSelectableObject Interface

	virtual bool Modify(bool bAlwaysMarkDirty) override;

protected:

	void ProcessUpdatedAndRemovedHandles(TArray<TSharedPtr<FChaosVDExtractedGeometryDataHandle>>& OutExtractedGeometryDataHandles);

	const TArray<TSharedPtr<FChaosVDParticlePairMidPhase>>* GetCollisionMidPhasesArray() const;

	const TArray<TSharedPtr<FChaosVDConstraintDataWrapperBase>>* GetCharacterGroundConstraintArray() const;

	void UpdateShapeDataComponents();

	template<typename TTaskCallback>
	void VisitGeometryInstances(const TTaskCallback& VisitorCallback);

	TSharedPtr<FChaosVDParticleDataWrapper> ParticleDataPtr;

	FTransform CachedSimulationTransform;

	bool bIsGeometryDataGenerationStarted = false;

	FDelegateHandle GeometryUpdatedDelegate;

	TArray<TSharedPtr<FChaosVDMeshDataInstanceHandle>> MeshDataHandles;

	FChaosVDParticleDataUpdatedDelegate ParticleDataUpdatedDelegate;

	bool bIsActive = false;

	bool bIsServer = false;

	EChaosVDHideParticleFlags HideParticleFlags;

	TWeakPtr<FChaosVDMeshDataInstanceHandle> CurrentSelectedGeometryInstance;

	friend FChaosVDParticleActorCustomization;
};

template <typename TVisitorCallback>
void AChaosVDParticleActor::VisitGeometryInstances(const TVisitorCallback& VisitorCallback)
{
	for (TSharedPtr<FChaosVDMeshDataInstanceHandle>& MeshDataHandle : MeshDataHandles)
	{
		if (MeshDataHandle)
		{
			VisitorCallback(MeshDataHandle.ToSharedRef());
		}
	}
}
