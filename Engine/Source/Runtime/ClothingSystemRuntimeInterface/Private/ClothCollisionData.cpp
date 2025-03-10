// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClothCollisionData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ClothCollisionData)

void FClothCollisionData::Reset()
{
	Spheres.Reset();
	SphereConnections.Reset();
	Convexes.Reset();
	Boxes.Reset();
}

void FClothCollisionData::Append(const FClothCollisionData& InOther)
{
	const int32 NumSpheresBefore = Spheres.Num();
	const int32 NumSphereConnectionsBefore = SphereConnections.Num();

	Spheres.Append(InOther.Spheres);
	SphereConnections.Append(InOther.SphereConnections);

	const int32 NumSphereConnectionsAfter = SphereConnections.Num();

	if(NumSpheresBefore > 0)
	{
		// Each connection that was added needs to have its sphere indices increased to match the new spheres that were added
		for(int32 NewConnectionIndex = NumSphereConnectionsBefore; NewConnectionIndex < NumSphereConnectionsAfter; ++NewConnectionIndex)
		{
			FClothCollisionPrim_SphereConnection& Connection = SphereConnections[NewConnectionIndex];
			Connection.SphereIndices[0] += NumSpheresBefore;
			Connection.SphereIndices[1] += NumSpheresBefore;
		}
	}

	Convexes.Append(InOther.Convexes);

	Boxes.Append(InOther.Boxes);
}

void FClothCollisionData::AppendTransformed(const FClothCollisionData& InOther, const TArray<FTransform>& BoneTransforms)
{
	const int32 NumSpheresBefore = Spheres.Num();
	const int32 NumConvexesBefore = Convexes.Num();
	const int32 NumBoxesBefore = Boxes.Num();

	Append(InOther);

	for (int32 Index = NumSpheresBefore; Index < Spheres.Num(); ++Index)
	{
		const FTransform& Transform = BoneTransforms[Spheres[Index].BoneIndex];
		Spheres[Index].LocalPosition = Transform.TransformPosition(Spheres[Index].LocalPosition);
	}
	for (int32 Index = NumConvexesBefore; Index < Convexes.Num(); ++Index)
	{
		const FTransform& Transform = BoneTransforms[Convexes[Index].BoneIndex];
		for (FVector& SurfacePoint : Convexes[Index].SurfacePoints)
		{
			SurfacePoint = Transform.TransformPosition(SurfacePoint);
		}
	}
	for (int32 Index = NumBoxesBefore; Index < Boxes.Num(); ++Index)
	{
		const FTransform& Transform = BoneTransforms[Boxes[Index].BoneIndex];
		Boxes[Index].LocalPosition = Transform.TransformPosition(Boxes[Index].LocalPosition);
		Boxes[Index].LocalRotation = Transform.TransformRotation(Boxes[Index].LocalRotation);
	}
}
