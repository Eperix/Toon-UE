// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChaosVDRecordingStateScreenMessageHandler.h"

#if WITH_CHAOS_VISUAL_DEBUGGER

#include "ChaosVDRuntimeModule.h"
#include "ChaosVisualDebugger/ChaosVDTraceMacros.h"
#include "ChaosVisualDebugger/ChaosVisualDebuggerTrace.h"
#include "DataWrappers/ChaosVDCollisionDataWrappers.h"
#include "Engine/Engine.h"
#include "Serialization/MemoryWriter.h"

void FChaosVDRecordingStateScreenMessageHandler::AddOnScreenRecordingMessage()
{
	if (!GEngine)
	{
		return;
	}

	static FText ChaosVDRecordingStartedMessage = NSLOCTEXT("ChaosVisualDebugger", "OnScreenChaosVDRecordingStartedMessage", "Chaos Visual Debugger recording in progress...");

	if (CVDRecordingMessageKey == 0)
	{
		CVDRecordingMessageKey = GetTypeHash(ChaosVDRecordingStartedMessage.ToString());
	}
	
	// Add a long duration value, we will remove the message manually when the recording stops
	constexpr float MessageDurationSeconds = 3600.0f;
	GEngine->AddOnScreenDebugMessage(CVDRecordingMessageKey, MessageDurationSeconds, FColor::Red,ChaosVDRecordingStartedMessage.ToString());
}

void FChaosVDRecordingStateScreenMessageHandler::RemoveOnScreenRecordingMessage()
{
	if (!GEngine)
	{
		return;
	}

	if (CVDRecordingMessageKey != 0)
	{
		GEngine->RemoveOnScreenDebugMessage(CVDRecordingMessageKey);
	}
}

void FChaosVDRecordingStateScreenMessageHandler::HandleCVDRecordingStarted()
{
	SerializeCollisionChannelsNames();

	AddOnScreenRecordingMessage();
}

void FChaosVDRecordingStateScreenMessageHandler::HandleCVDRecordingStopped()
{
	RemoveOnScreenRecordingMessage();
}

void FChaosVDRecordingStateScreenMessageHandler::HandleCVDRecordingStartFailed(const FText& InFailureReason) const
{
#if !WITH_EDITOR
	// In non-editor builds we don't have an error pop-up, therefore we want to show the error message on screen
	FText ErrorMessage = FText::FormatOrdered(NSLOCTEXT("ChaosVisualDebugger","StartRecordingFailedOnScreenMessage", "Failed to start CVD recording. {0}"), InFailureReason);

	constexpr float MessageDurationSeconds = 4.0f;
	GEngine->AddOnScreenDebugMessage(CVDRecordingMessageKey, MessageDurationSeconds, FColor::Red, ErrorMessage.ToString());
#endif
}

void FChaosVDRecordingStateScreenMessageHandler::HandlePIEStarted(UGameInstance* GameInstance)
{
	// If we were already recording, show the message
	if (FChaosVDRuntimeModule::Get().IsRecording())
	{
		HandleCVDRecordingStarted();
	}
}

void FChaosVDRecordingStateScreenMessageHandler::SerializeCollisionChannelsNames()
{
	TArray<uint8> CollisionChannelsDataBuffer;
	FMemoryWriter MemWriterAr(CollisionChannelsDataBuffer);

	FChaosVDCollisionChannelsInfoContainer CollisionChannelInfoContainer;

	if (UCollisionProfile* CollisionProfileData = UCollisionProfile::Get())
	{
		constexpr int32 MaxSupportedChannels = 32;
		for (int32 ChannelIndex = 0; ChannelIndex < MaxSupportedChannels; ++ChannelIndex)
		{
			FChaosVDCollisionChannelInfo Info;
			Info.DisplayName = CollisionProfileData->ReturnChannelNameFromContainerIndex(ChannelIndex).ToString();
			Info.CollisionChannel = ChannelIndex;
			Info.bIsTraceType = CollisionProfileData->ConvertToTraceType(static_cast<ECollisionChannel>(ChannelIndex)) != TraceTypeQuery_MAX;
			CollisionChannelInfoContainer.CustomChannelsNames[ChannelIndex] = Info;
		}
	}

	Chaos::VisualDebugger::WriteDataToBuffer(CollisionChannelsDataBuffer, CollisionChannelInfoContainer);

	CVD_TRACE_BINARY_DATA(CollisionChannelsDataBuffer, FChaosVDCollisionChannelsInfoContainer::WrapperTypeName, EChaosVDTraceBinaryDataOptions::ForceTrace)
}

FChaosVDRecordingStateScreenMessageHandler& FChaosVDRecordingStateScreenMessageHandler::Get()
{
	static FChaosVDRecordingStateScreenMessageHandler MessageHandler;
	return MessageHandler;
}

void FChaosVDRecordingStateScreenMessageHandler::Initialize()
{
	RecordingStartedHandle = FChaosVDRuntimeModule::Get().RegisterRecordingStartedCallback(FChaosVDRecordingStateChangedDelegate::FDelegate::CreateRaw(this, &FChaosVDRecordingStateScreenMessageHandler::HandleCVDRecordingStarted));
	RecordingStoppedHandle = FChaosVDRuntimeModule::Get().RegisterRecordingStopCallback(FChaosVDRecordingStateChangedDelegate::FDelegate::CreateRaw(this, &FChaosVDRecordingStateScreenMessageHandler::HandleCVDRecordingStopped));
	RecordingStartFailedHandle = FChaosVDRuntimeModule::Get().RegisterRecordingStartFailedCallback(FChaosVDRecordingStartFailedDelegate::FDelegate::CreateRaw(this, &FChaosVDRecordingStateScreenMessageHandler::HandleCVDRecordingStartFailed));

#if WITH_EDITOR
	PIEStartedHandle = FWorldDelegates::OnPIEStarted.AddRaw(this, &FChaosVDRecordingStateScreenMessageHandler::HandlePIEStarted);
#endif

	// If we were already recording, show the message
	if (FChaosVDRuntimeModule::Get().IsRecording())
	{
		HandleCVDRecordingStarted();
	}
}

void FChaosVDRecordingStateScreenMessageHandler::TearDown()
{
	// Note: This works during engine shutdown because the Module Manager doesn't free the dll on module unload to account for use cases like this
	// If this appears in a callstack crash it means that assumption changed or was not correct to begin with.
	// A possible solution is just check if the module is loaded querying the module manager just using the module's name
	if (FChaosVDRuntimeModule::IsLoaded())
	{
		FChaosVDRuntimeModule::Get().RemoveRecordingStartedCallback(RecordingStartedHandle);
		FChaosVDRuntimeModule::Get().RemoveRecordingStopCallback(RecordingStoppedHandle);

#if WITH_EDITOR
		 FWorldDelegates::OnPIEStarted.Remove(PIEStartedHandle);
#endif

		// Make sure of removing the message from the screen in case the recording didn't quite stopped yet
		if (FChaosVDRuntimeModule::Get().IsRecording())
		{
			HandleCVDRecordingStopped();
		}
	}

}
#endif
