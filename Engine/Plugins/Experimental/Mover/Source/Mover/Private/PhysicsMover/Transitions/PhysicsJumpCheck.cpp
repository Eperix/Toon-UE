// Copyright Epic Games, Inc. All Rights Reserved.

#include "PhysicsMover/Transitions/PhysicsJumpCheck.h"
#include "DefaultMovementSet/InstantMovementEffects/BasicInstantMovementEffects.h"
#include "MoverComponent.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

UPhysicsJumpCheck::UPhysicsJumpCheck(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	TransitionToMode = DefaultModeNames::Falling;
}

FTransitionEvalResult UPhysicsJumpCheck::OnEvaluate(const FSimulationTickParams& Params) const
{
	FTransitionEvalResult EvalResult; 

	const FMoverTickStartData& StartState = Params.StartState;
	const FCharacterDefaultInputs* CharacterInputs = StartState.InputCmd.InputCollection.FindDataByType<FCharacterDefaultInputs>();
	check(CharacterInputs);

	if (CharacterInputs->bIsJumpJustPressed)
	{
		EvalResult.NextMode = TransitionToMode;
	}

	return EvalResult;
}

void UPhysicsJumpCheck::OnTrigger(const FSimulationTickParams& Params)
{
	TSharedPtr<FJumpImpulseEffect> JumpMove = MakeShared<FJumpImpulseEffect>();
	JumpMove->UpwardsSpeed = JumpUpwardsSpeed;

	Params.MovingComps.MoverComponent->QueueInstantMovementEffect(JumpMove);
}

#if WITH_EDITOR
EDataValidationResult UPhysicsJumpCheck::IsDataValid(FDataValidationContext& Context) const
{
	return Super::IsDataValid(Context);
}
#endif // WITH_EDITOR
