// Copyright Epic Games, Inc. All Rights Reserved. 
#pragma once

#if WITH_EDITOR

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "Rendering/SkeletalMeshLODImporterData.h"
//#include "ReferenceSkeleton.h"

struct FReferenceSkeleton;
class UInterchangeBaseNode;
class UInterchangeBaseNodeContainer;
class UInterchangeResultsContainer;
class USkeleton;
struct FMeshBoneInfo;

namespace UE::Interchange::Private
{

	struct FJointInfo
	{
		FString Name;
		int32 ParentIndex;  // 0 if this is the root bone.  
		FTransform	LocalTransform; // local transform
	};
	
	struct INTERCHANGEIMPORT_API FSkeletonHelper
	{
	public:
		static bool ProcessImportMeshSkeleton(TObjectPtr<UInterchangeResultsContainer> Results
			, const USkeleton* SkeletonAsset
			, FReferenceSkeleton& RefSkeleton
			, const UInterchangeBaseNodeContainer* NodeContainer
			, const FString& RootJointNodeId
			, TArray<SkeletalMeshImportData::FBone>& RefBonesBinary
			, const bool bUseTimeZeroAsBindPose
			, bool& bOutDiffPose);

		static bool IsCompatibleSkeleton(const USkeleton* Skeleton, const FString RootJoinUid, const UInterchangeBaseNodeContainer* BaseNodeContainer, bool bConvertStaticToSkeletalActive, bool bCheckForIdenticalSkeleton = false);
		static void RecursiveAddSkeletonMetaDataValues(UInterchangeBaseNodeContainer* NodeContainer, UInterchangeBaseNode* DestinationNode, const FString& JointUid);

		static void RecursiveBoneHasBindPose(const UInterchangeBaseNodeContainer* NodeContainer, const FString& JointNodeId, bool& bHasBoneWithoutBindPose);

		static void RecursiveAddBones(const UInterchangeBaseNodeContainer* NodeContainer
			, const FString& JointNodeId
			, TArray <FJointInfo>& JointInfos
			, int32 ParentIndex
			, TArray<SkeletalMeshImportData::FBone>& RefBonesBinary
			, const bool bUseTimeZeroAsBindPose
			, bool& bOutDiffPose
			, TArray<FString>& OutBoneNotBindNames);

	private:
		static FName SkeletalLodGetBoneName(const TArray<FMeshBoneInfo>& SkeletalLodRawInfos, int32 BoneIndex);
		static int32 SkeletalLodFindBoneIndex(const TArray<FMeshBoneInfo>& SkeletalLodRawInfos, FName BoneName);
		static int32 SkeletalLodGetParentIndex(const TArray<FMeshBoneInfo>& SkeletalLodRawInfos, int32 BoneIndex);
		static bool DoesParentChainMatch(int32 StartBoneIndex, const FReferenceSkeleton& SkeletonRef, const TArray<FMeshBoneInfo>& SkeletalLodRawInfos);

		static void RecursiveBuildSkeletalSkeleton(const FString JoinToAddUid
			, const int32 ParentIndex
			, const UInterchangeBaseNodeContainer* BaseNodeContainer
			, TArray<FMeshBoneInfo>& SkeletalLodRawInfos
			, bool bConvertStaticToSkeletalActive = false);
	};
} //namespace UE::Interchange::Private

#endif //WITH_EDITOR
