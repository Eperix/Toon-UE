// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "Engine/Engine.h"
#include "Framework/Application/SlateApplication.h"
#include "LiveLinkHub.h"
#include "LiveLinkHubInputProcessor.h"
#include "LiveLinkHubTicker.h"
#include "Misc/App.h"
#include "Misc/CoreDelegates.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Settings/LiveLinkHubSettings.h"
#include "UObject/UObjectGlobals.h"

#if PLATFORM_MAC
#include "HAL/PlatformApplicationMisc.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogLiveLinkHubApplication, Log, All);

void LiveLinkHubLoop(const TSharedPtr<FLiveLinkHub>& LiveLinkHub)
{
	check(FSlateApplication::IsInitialized());
	FSlateApplication::Get().RegisterInputPreProcessor(MakeShared<FLiveLinkHubInputProcessor>());
	{
		UE_LOG(LogLiveLinkHubApplication, Display, TEXT("LiveLinkHub Initialized (Version: %d.%d)"), ENGINE_MAJOR_VERSION, ENGINE_MINOR_VERSION);

		double LastTime = FPlatformTime::Seconds();

		const double IdealFrameTime = 1 / GetDefault<ULiveLinkHubSettings>()->TargetFrameRate;

		while (!IsEngineExitRequested())
		{
			const double CurrentTime = FPlatformTime::Seconds();
			const double DeltaTime = CurrentTime - LastTime;

			FApp::SetDeltaTime(DeltaTime);
			GEngine->UpdateTimeAndHandleMaxTickRate();

			CommandletHelpers::TickEngine(nullptr, DeltaTime);

			FSlateApplication::Get().PollGameDeviceState();

			// Run garbage collection for the UObjects for the rest of the frame or at least to 2 ms
			IncrementalPurgeGarbage(true, FMath::Max<float>(0.002f, IdealFrameTime - (FPlatformTime::Seconds() - LastTime)));

#if PLATFORM_MAC
			// Pumps the message from the main loop. (On Mac, we have a full application to get a proper console window to output logs)
			FPlatformApplicationMisc::PumpMessages(true);
#endif

			FCoreDelegates::OnEndFrame.Broadcast();

			// Throttle main thread main fps by sleeping if we still have time
			FPlatformProcess::Sleep(FMath::Max<float>(0.0f, IdealFrameTime - (FPlatformTime::Seconds() - LastTime)));

			LastTime = CurrentTime;
		}
	}
}
