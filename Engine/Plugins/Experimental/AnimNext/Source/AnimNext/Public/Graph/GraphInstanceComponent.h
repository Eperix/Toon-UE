// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FAnimNextTraitEvent;
struct FAnimNextGraphInstance;

// Declares a graph instance component and implements the necessary boilerplate
#define DECLARE_ANIM_GRAPH_INSTANCE_COMPONENT(ComponentType) \
	static FName StaticComponentName() { return FName(TEXT(#ComponentType)); } \
	virtual FName GetComponentName() const override { return StaticComponentName(); }

namespace UE::AnimNext
{
	struct FExecutionContext;

	/**
	 * FGraphInstanceComponent
	 *
	 * A graph instance component is attached and owned by a graph instance.
	 * It persists as long as it is needed.
	 */
	struct FGraphInstanceComponent
	{
		explicit FGraphInstanceComponent(FAnimNextGraphInstance& InOwnerInstance) : OwnerInstance(InOwnerInstance) {}
		virtual ~FGraphInstanceComponent() {}

		static FName StaticComponentName() { return FName(TEXT("FGraphInstanceComponent")); }
		virtual FName GetComponentName() const { return StaticComponentName(); }

		// Returns the owning graph instance this component lives on
		FAnimNextGraphInstance& GetGraphInstance() { return OwnerInstance; }

		// Returns the owning graph instance this component lives on
		const FAnimNextGraphInstance& GetGraphInstance() const { return OwnerInstance; }

		// Called before the update traversal begins, before any node has been visited
		// Note that PreUpdate won't be called if a component is created during the update traversal until the next update
		// The execution context provided is bound to the graph root and can be bound to anything the component wishes
		virtual void PreUpdate(FExecutionContext& Context) {}

		// Called after the update traversal completes, after every node has been visited
		// The execution context provided is bound to the graph root and can be bound to anything the component wishes
		virtual void PostUpdate(FExecutionContext& Context) {}

		// Called before PreUpdate with input events and before PostUpdate with output events
		virtual void OnTraitEvent(FExecutionContext& Context, FAnimNextTraitEvent& Event) {}

	private:
		// The owning graph instance this component lives on
		FAnimNextGraphInstance& OwnerInstance;
	};
}
