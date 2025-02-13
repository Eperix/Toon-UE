// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Metadata/PCGMetadataAttributeTpl.h"
#include "Metadata/PCGMetadataAttributeTraits.h"
#include "Metadata/Accessors/IPCGAttributeAccessor.h"
#include "Metadata/Accessors/PCGAttributeAccessorKeys.h"

#include "Containers/Array.h"
#include "Templates/UniquePtr.h"
#include "UObject/NameTypes.h"

enum class EPCGExtraProperties : uint8;
class IPCGAttributeAccessor;
class IPCGAttributeAccessorKeyIterator;
class IPCGAttributeAccessorKeys;
class FProperty;
class UClass;
class UPCGData;
class UStruct;
struct FPCGAttributePropertySelector;
struct FPCGDataCollection;
struct FPCGSettingsOverridableParam;


namespace PCGAttributeAccessorHelpers
{
	PCG_API bool IsPropertyAccessorSupported(const FProperty* InProperty);
	PCG_API bool IsPropertyAccessorSupported(const FName InPropertyName, const UStruct* InStruct);
	PCG_API bool IsPropertyAccessorChainSupported(const TArray<FName>& InPropertyNames, const UStruct* InStruct);

	PCG_API TUniquePtr<IPCGAttributeAccessor> CreatePropertyAccessor(const FProperty* InProperty);
	PCG_API TUniquePtr<IPCGAttributeAccessor> CreatePropertyAccessor(const FName InPropertyName, const UStruct* InStruct);

	PCG_API TUniquePtr<IPCGAttributeAccessor> CreatePropertyChainAccessor(TArray<const FProperty*>&& InProperties);
	PCG_API TUniquePtr<IPCGAttributeAccessor> CreatePropertyChainAccessor(const TArray<FName>& InPropertyNames, const UStruct* InStruct);

	PCG_API TUniquePtr<IPCGAttributeAccessor> CreateExtraAccessor(EPCGExtraProperties InExtraProperties);

	PCG_API TUniquePtr<IPCGAttributeAccessor> CreateChainAccessor(TUniquePtr<IPCGAttributeAccessor> InAccessor, FName Name, bool& bOutSuccess);

	struct AccessorParamResult
	{
		FName AttributeName = NAME_None;
		FName AliasUsed = NAME_None;
		bool bUsedAliases = false;
		bool bPinConnected = false;
		bool bHasMultipleAttributeSetsOnOverridePin = false;
		bool bHasMultipleDataInAttributeSet = false;
	};

	/**
	* Create a const accessor depending on an overridable param
	*/
	PCG_API TUniquePtr<const IPCGAttributeAccessor> CreateConstAccessorForOverrideParamWithResult(const FPCGDataCollection& InInputData, const FPCGSettingsOverridableParam& InParam, AccessorParamResult* OutResult = nullptr);

	/**
	* Creates a const accessor to the property or attribute pointed at by the InSelector.
	* Note that InData must not be null if the selector points to an attribute,
	* but in the case of properties, it either has to be the appropriate type or null.
	* Make sure to update your selector before-hand if you want to support "@Last"
	*/
	PCG_API TUniquePtr<const IPCGAttributeAccessor> CreateConstAccessor(const UPCGData* InData, const FPCGAttributePropertySelector& InSelector, bool bQuiet = false);

	/** 
	* Creates a const accessor to an attribute without requiring a selector.
	*/
	PCG_API TUniquePtr<const IPCGAttributeAccessor> CreateConstAccessor(const FPCGMetadataAttributeBase* InAttribute, const UPCGMetadata* InMetadata, bool bQuiet = false);

	/**
	* Creates an accessor to the property or attribute pointed at by the InSelector.
	* Note that InData must not be null if the selector points to an attribute,
	* but in the case of properties, it either has to be the appropriate type or null.
	* Make sure to update your selector before-hand if you want to support "@Source". Otherwise the creation will fail.
	*/
	PCG_API TUniquePtr<IPCGAttributeAccessor> CreateAccessor(UPCGData* InData, const FPCGAttributePropertySelector& InSelector, bool bQuiet = false);

	/**
	* Creates an accessor to the property or attribute pointed at by the InSelector. If the selector points to a base attribute and the attribute doesn't exist 
	* or the types doesn't match with the Matching Accessor, create the attribute using the Matching Accessor.
	* Note that InData must not be null if the selector points to an attribute,
	* but in the case of properties, it either has to be the appropriate type or null.
	* Make sure to update your selector before-hand if you want to support "@Source". Otherwise the creation will fail.
	*/
	PCG_API TUniquePtr<IPCGAttributeAccessor> CreateAccessorWithAttributeCreation(UPCGData* InData, const FPCGAttributePropertySelector& InSelector, const IPCGAttributeAccessor* InMatchingAccessor, EPCGAttributeAccessorFlags InTypeMatching = EPCGAttributeAccessorFlags::AllowBroadcastAndConstructible, bool bQuiet = false);

	/**
	* Creates an accessor to an attribute without requiring a selector.
	*/
	PCG_API TUniquePtr<IPCGAttributeAccessor> CreateAccessor(FPCGMetadataAttributeBase* InAttribute, UPCGMetadata* InMetadata, bool bQuiet = false);

	PCG_API TUniquePtr<const IPCGAttributeAccessorKeys> CreateConstKeys(const UPCGData* InData, const FPCGAttributePropertySelector& InSelector);
	PCG_API TUniquePtr<IPCGAttributeAccessorKeys> CreateKeys(UPCGData* InData, const FPCGAttributePropertySelector& InSelector);

	/** 
	 * Utility function to create the accessor and keys and write all the values for all the keys coming from InValues. Also, automatically resolve the selector for @Last.
	 *
	 * @param OutputData      Input data to extract the value from. Will also be used to resolve @Source on the selector.
	 * @param OutputSelector  Selector to know which value to write to. Will be resolved if it is @Source.
	 * @param InValues        Input view where the values will be read from. InValues MUST be the same size as the keys that will be created by CreateKeys on the OutputData. 
	 * @param SourceSelector  Optional source selector for @Source resolve. Can be null.
	 * @param Context         Optional context for logging. Can be null.
	 * @param SetFlags        Flags for the set accessor. By default, is broadcast and constructible.
	 * @return true if it succeeded.
	 */
	template <typename T>
	bool WriteAllValues(UPCGData* OutputData, const FPCGAttributePropertyOutputSelector& OutputSelector, TArrayView<const T> InValues, const FPCGAttributePropertyInputSelector* SourceSelector = nullptr, FPCGContext* Context = nullptr, EPCGAttributeAccessorFlags SetFlags = EPCGAttributeAccessorFlags::AllowBroadcastAndConstructible)
	{
		// TODO: Provide a version that take advantage of compressed attributes (Keys + Values)
		
		const FPCGAttributePropertyOutputSelector Selector = OutputSelector.CopyAndFixSource(SourceSelector, OutputData);

		TUniquePtr<IPCGAttributeAccessor> Accessor = CreateAccessor(OutputData, Selector);
		TUniquePtr<IPCGAttributeAccessorKeys> Keys = CreateKeys(OutputData, Selector);
		if (!Accessor.IsValid() || !Keys.IsValid())
		{
			PCGLog::Metadata::LogFailToCreateAccessorError(Selector, Context);
			return false;
		}

		if (!ensureMsgf(Keys->GetNum() == InValues.Num(), TEXT("Number of values passed (%d) mismatches with the number of keys (%d)"), Keys->GetNum(), InValues.Num()))
		{
			return false;
		}

		if (!Accessor->SetRange<T>(InValues, 0, *Keys, SetFlags))
		{
			PCGLog::LogErrorOnGraph(FText::Format(NSLOCTEXT("PCGAttributeAccessorHelpers", "SetTypedAttributeFailure", "Couldn't set attribute '{0}' value."), Selector.GetDisplayText()));
			return false;
		}

		return true;
	}

	namespace Private
	{
		// Use a lambda to have this code more likely to be inlined in SortByAttribute.
		static inline auto DefaultIndexGetter = [](int32 Index) -> int32 { return Index; };

		// Use bAscending bool to know if you need to negate the condition for equal, since CompareDescending is !CompareAscending
		template<typename T>
		bool DefaultStableCompareLess(const T& A, const T& B, int32 IndexA, int32 IndexB, bool bAscending)
		{
			if (PCG::Private::MetadataTraits<T>::Equal(A, B))
			{
				return (bAscending == (IndexA < IndexB));
			}

			return PCG::Private::MetadataTraits<T>::Less(A, B);
		}

		// We need the lambda, because we can't use a templated function as a default parameter without specifying the template, but we can with a generic lambda.
		static inline auto DefaultStableCompareLessLambda = [](const auto& A, const auto& B, int32 IndexA, int32 IndexB, bool bAscending) -> bool { return DefaultStableCompareLess(A, B, IndexA, IndexB, bAscending); };
	}

	/**
	* Sorts array given the accessors and keys of the array. Sort is stable by default.
	* A custom function can be used to extract the index of the elements of the array,
	* and sort ascending or descending by the values associated with that index.
	* CompareLess method can also be provided, method signature needs to follow DefaultStableCompareLess.
	* We need the decltypes on both function types to allow to provide default values for both callbacks.
	* Check DefaultIndexGetter and DefaultStableCompareLess for their signatures.
	*/
	template <typename T, typename GetIndexFunc = decltype(Private::DefaultIndexGetter), typename CompareLessFunc = decltype(Private::DefaultStableCompareLessLambda)>
	void SortByAttribute(const IPCGAttributeAccessor& InAccessor, const IPCGAttributeAccessorKeys& InKeys, TArray<T>& InArray, bool bAscending, GetIndexFunc CustomGetIndex = Private::DefaultIndexGetter, CompareLessFunc CompareLess = Private::DefaultStableCompareLessLambda)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(PCGAttributeAccessorHelpers::SortByAttribute);
		check(InArray.Num() <= InKeys.GetNum())

		if (InArray.IsEmpty())
		{
			return;
		}

		// Prepare integer sequence
		TArray<int32> ElementsIndexes;
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(PCGAttributeAccessorHelpers::SortByAttribute::PrepareIndexes);
			ElementsIndexes.Reserve(InArray.Num());
			for (int i = 0; i < InArray.Num(); ++i)
			{
				ElementsIndexes.Add(i);
			}
		}

		auto SortIndexes = [&InAccessor, &InKeys, &InArray, bAscending, &CustomGetIndex, &CompareLess, &ElementsIndexes](auto Dummy)
		{
			using ValueType = decltype(Dummy);

			if constexpr (PCG::Private::MetadataTraits<ValueType>::CanCompare)
			{
				TArray<ValueType> CachedValues;

				{
					TRACE_CPUPROFILER_EVENT_SCOPE(PCGAttributeAccessorHelpers::SortByAttribute::AllocateCachedValues);
					if constexpr (std::is_trivially_copyable_v<ValueType>)
					{
						CachedValues.SetNumUninitialized(InKeys.GetNum());
					}
					else
					{
						CachedValues.SetNum(InKeys.GetNum());
					}
				}

				{
					TRACE_CPUPROFILER_EVENT_SCOPE(PCGAttributeAccessorHelpers::SortByAttribute::GatherCachedValues);
					InAccessor.GetRange(TArrayView<ValueType>(CachedValues), 0, InKeys);
				}


				// Pass bAscending bool to the compare function to be able to negate the condition on equal, for it to be also stable in descending mode.
				auto CompareAscending = [&CachedValues, &CustomGetIndex, &CompareLess, bAscending](int LHS, int RHS)
				{
					const int32 LHSIndex = CustomGetIndex(LHS);
					const int32 RHSIndex = CustomGetIndex(RHS);
					return CompareLess(CachedValues[LHSIndex], CachedValues[RHSIndex], LHSIndex, RHSIndex, bAscending);
				};

				auto CompareDescending = [&CompareAscending](int LHS, int RHS) { return !CompareAscending(LHS, RHS); };

				{
					TRACE_CPUPROFILER_EVENT_SCOPE(PCGAttributeAccessorHelpers::SortByAttribute::SortIndexes);
					if (bAscending)
					{
						ElementsIndexes.Sort(CompareAscending);
					}
					else
					{
						ElementsIndexes.Sort(CompareDescending);
					}
				}
			}
		};

		// Perform the sorting on the indexes using the attribute values provided by the accessor.
		PCGMetadataAttribute::CallbackWithRightType(InAccessor.GetUnderlyingType(), SortIndexes);

		// Write back the values according to the sorted indexes.
		TArray<T> SortedArray;
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(PCGAttributeAccessorHelpers::SortByAttribute::WriteBackInSortedArray);
			SortedArray.Reserve(InArray.Num());

			for (int i = 0; i < InArray.Num(); ++i)
			{
				SortedArray.Add(MoveTemp(InArray[ElementsIndexes[i]]));
			}
		}

		InArray = MoveTemp(SortedArray);
	}
}
