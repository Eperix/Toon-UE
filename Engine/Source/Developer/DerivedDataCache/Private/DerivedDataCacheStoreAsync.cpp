// Copyright Epic Games, Inc. All Rights Reserved.

#include "Async/AsyncWork.h"
#include "DerivedDataBackendInterface.h"
#include "DerivedDataCachePrivate.h"
#include "DerivedDataCacheStore.h"
#include "DerivedDataCacheUsageStats.h"
#include "DerivedDataLegacyCacheStore.h"
#include "DerivedDataRequestOwner.h"
#include "DerivedDataValueId.h"
#include "Memory/SharedBuffer.h"
#include "MemoryCacheStore.h"
#include "ProfilingDebugging/CookStats.h"
#include "Stats/Stats.h"
#include "Templates/Invoke.h"

namespace UE::DerivedData
{

/**
 * A cache store that executes non-blocking requests in a dedicated thread pool.
 *
 * Puts can be stored in a memory cache while they are in flight.
 */
class FCacheStoreAsync final : public ILegacyCacheStore
{
public:
	FCacheStoreAsync(ILegacyCacheStore* InnerCache, IMemoryCacheStore* MemoryCache, bool bDeleteInnerCache);
	~FCacheStoreAsync() final;

	virtual void Put(
		TConstArrayView<FCachePutRequest> Requests,
		IRequestOwner& Owner,
		FOnCachePutComplete&& OnComplete) override
	{
		if (MemoryCache)
		{
			MemoryCache->Put(Requests, Owner, [](auto&&){});
			Execute(COOK_STAT(&FDerivedDataCacheUsageStats::TimePut,) Requests, Owner,
				[this, OnComplete = MoveTemp(OnComplete)](FCachePutResponse&& Response)
				{
					MemoryCache->Delete(Response.Key, Response.Name);
					OnComplete(MoveTemp(Response));
				}, &ICacheStore::Put);
		}
		else
		{
			Execute(COOK_STAT(&FDerivedDataCacheUsageStats::TimePut,) Requests, Owner, MoveTemp(OnComplete), &ICacheStore::Put);
		}
	}

	virtual void Get(
		TConstArrayView<FCacheGetRequest> Requests,
		IRequestOwner& Owner,
		FOnCacheGetComplete&& OnComplete) override
	{
		Execute(COOK_STAT(&FDerivedDataCacheUsageStats::TimeGet,) Requests, Owner, MoveTemp(OnComplete), &ICacheStore::Get);
	}

	virtual void PutValue(
		TConstArrayView<FCachePutValueRequest> Requests,
		IRequestOwner& Owner,
		FOnCachePutValueComplete&& OnComplete) override
	{
		if (MemoryCache)
		{
			MemoryCache->PutValue(Requests, Owner, [](auto&&){});
			Execute(COOK_STAT(&FDerivedDataCacheUsageStats::TimePut,) Requests, Owner,
				[this, OnComplete = MoveTemp(OnComplete)](FCachePutValueResponse&& Response)
				{
					MemoryCache->DeleteValue(Response.Key, Response.Name);
					OnComplete(MoveTemp(Response));
				}, &ICacheStore::PutValue);
		}
		else
		{
			Execute(COOK_STAT(&FDerivedDataCacheUsageStats::TimePut,) Requests, Owner, MoveTemp(OnComplete), &ICacheStore::PutValue);
		}
	}

	virtual void GetValue(
		TConstArrayView<FCacheGetValueRequest> Requests,
		IRequestOwner& Owner,
		FOnCacheGetValueComplete&& OnComplete) override
	{
		Execute(COOK_STAT(&FDerivedDataCacheUsageStats::TimeGet,) Requests, Owner, MoveTemp(OnComplete), &ICacheStore::GetValue);
	}

	virtual void GetChunks(
		TConstArrayView<FCacheGetChunkRequest> Requests,
		IRequestOwner& Owner,
		FOnCacheGetChunkComplete&& OnComplete) override
	{
		Execute(COOK_STAT(&FDerivedDataCacheUsageStats::TimeGet,) Requests, Owner, MoveTemp(OnComplete), &ICacheStore::GetChunks);
	}

	virtual void LegacyStats(FDerivedDataCacheStatsNode& OutNode) override;

	virtual bool LegacyDebugOptions(FBackendDebugOptions& Options) override
	{
		return InnerCache->LegacyDebugOptions(Options);
	}

private:
	COOK_STAT(using CookStatsFunction = FCookStats::FScopedStatsCounter (FDerivedDataCacheUsageStats::*)());

	template <typename RequestType, typename OnCompleteType, typename OnExecuteType>
	void Execute(
		COOK_STAT(CookStatsFunction OnAddStats,)
		TConstArrayView<RequestType> Requests,
		IRequestOwner& Owner,
		OnCompleteType&& OnComplete,
		OnExecuteType&& OnExecute);

	ILegacyCacheStore* InnerCache;
	IMemoryCacheStore* MemoryCache;
	FDerivedDataCacheUsageStats UsageStats;
	bool bDeleteInnerCache;
};

FCacheStoreAsync::FCacheStoreAsync(ILegacyCacheStore* InInnerCache, IMemoryCacheStore* InMemoryCache, bool bInDeleteInnerCache)
	: InnerCache(InInnerCache)
	, MemoryCache(InMemoryCache)
	, bDeleteInnerCache(bInDeleteInnerCache)
{
	check(InnerCache);
}

FCacheStoreAsync::~FCacheStoreAsync()
{
	if (bDeleteInnerCache)
	{
		delete InnerCache;
	}
}

void FCacheStoreAsync::LegacyStats(FDerivedDataCacheStatsNode& OutNode)
{
	OutNode = {TEXT("Async"), TEXT(""), /*bIsLocal*/ true};
	OutNode.UsageStats.Add(TEXT(""), UsageStats);

	InnerCache->LegacyStats(OutNode.Children.Add_GetRef(MakeShared<FDerivedDataCacheStatsNode>()).Get());
}

template <typename RequestType, typename OnCompleteType, typename OnExecuteType>
void FCacheStoreAsync::Execute(
	COOK_STAT(CookStatsFunction OnAddStats,)
	const TConstArrayView<RequestType> Requests,
	IRequestOwner& Owner,
	OnCompleteType&& OnComplete,
	OnExecuteType&& OnExecute)
{
	auto ExecuteWithStats = [this, COOK_STAT(OnAddStats,) OnExecute](TConstArrayView<RequestType> Requests, IRequestOwner& Owner, OnCompleteType&& OnComplete) mutable
	{
		Invoke(OnExecute, *InnerCache, Requests, Owner, [this, COOK_STAT(OnAddStats,) OnComplete = MoveTemp(OnComplete)](auto&& Response)
		{
			if (COOK_STAT(auto Timer = Invoke(OnAddStats, UsageStats);) Response.Status == EStatus::Ok)
			{
				COOK_STAT(Timer.AddHit(0));
			}
			else if (Response.Status == EStatus::Canceled)
			{
				// The request was cancelled so only track the cycles lost, rather than adding a miss
				COOK_STAT(Timer.TrackCyclesOnly());
			}

			OnComplete(MoveTemp(Response));
			Private::AddToAsyncTaskCounter(-1);
		});
	};

	Private::AddToAsyncTaskCounter(Requests.Num());
	if (Owner.GetPriority() == EPriority::Blocking)
	{
		return ExecuteWithStats(Requests, Owner, MoveTemp(OnComplete));
	}

	Private::LaunchTaskInCacheThreadPool(Owner,
		[this,
		&Owner,
		Requests = TArray<RequestType>(Requests),
		OnComplete = MoveTemp(OnComplete),
		ExecuteWithStats = MoveTemp(ExecuteWithStats)]() mutable
		{
			if (!Owner.IsCanceled())
			{
				ExecuteWithStats(Requests, Owner, MoveTemp(OnComplete));
			}
			else
			{
				CompleteWithStatus(Requests, MoveTemp(OnComplete), EStatus::Canceled);
				Private::AddToAsyncTaskCounter(-Requests.Num());
			}
		});
}

ILegacyCacheStore* CreateCacheStoreAsync(ILegacyCacheStore* InnerCache, IMemoryCacheStore* MemoryCache, bool bDeleteInnerCache)
{
	return new FCacheStoreAsync(InnerCache, MemoryCache, bDeleteInnerCache);
}

} // UE::DerivedData
