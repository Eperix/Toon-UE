// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RigVMDefines.h: Global defines
=============================================================================*/

#pragma once

#ifndef UE_RIGVM_UCLASS_BASED_STORAGE_DISABLED
	#define UE_RIGVM_UCLASS_BASED_STORAGE_DISABLED 0
#endif

#ifndef UE_RIGVM_UOBJECT_PROPERTIES_ENABLED
	#define UE_RIGVM_UOBJECT_PROPERTIES_ENABLED 1
#endif

#ifndef UE_RIGVM_UINTERFACE_PROPERTIES_ENABLED
	#define UE_RIGVM_UINTERFACE_PROPERTIES_ENABLED 1
#endif

#ifndef UE_RIGVM_AGGREGATE_NODES_ENABLED
	#define UE_RIGVM_AGGREGATE_NODES_ENABLED 1
#endif

#ifndef UE_RIGVM_DEBUG_TYPEINDEX
	#define UE_RIGVM_DEBUG_TYPEINDEX 1
#endif

// in shipping always disable debugging the type index
#ifdef UE_BUILD_SHIPPING
	#if UE_BUILD_SHIPPING
		#undef UE_RIGVM_DEBUG_TYPEINDEX
		#define UE_RIGVM_DEBUG_TYPEINDEX 0
	#endif
#endif

#ifndef UE_RIGVM_DEBUG_EXECUTION
	#define UE_RIGVM_DEBUG_EXECUTION 0
#endif

#ifndef UE_RIGVMCONTROLLER_VERBOSE_REPOPULATE
	#define UE_RIGVMCONTROLLER_VERBOSE_REPOPULATE 0
#endif
