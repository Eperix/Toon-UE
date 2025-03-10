// Copyright Epic Games, Inc. All Rights Reserved.

#include "Elements/PCGSampleTexture.h"

#include "PCGContext.h"
#include "PCGPin.h"
#include "Data/PCGPointData.h"
#include "Data/PCGTextureData.h"
#include "Helpers/PCGAsync.h"
#include "Helpers/PCGHelpers.h"
#include "Metadata/Accessors/PCGAttributeAccessorHelpers.h"

#define LOCTEXT_NAMESPACE "PCGSampleTextureElement"

TArray<FPCGPinProperties> UPCGSampleTextureSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;

	FPCGPinProperties& InputPin = PinProperties.Emplace_GetRef(PCGSampleTextureConstants::InputPointLabel,
		EPCGDataType::Point,
		/*bAllowMultipleConnections=*/true,
		/*bAllowMultipleData=*/true);
	InputPin.SetRequiredPin();

	PinProperties.Emplace(PCGSampleTextureConstants::InputTextureLabel,
		EPCGDataType::BaseTexture,
		/*bAllowMultipleConnections=*/false,
		/*bAllowMultipleData=*/false);

	return PinProperties;
}

FPCGElementPtr UPCGSampleTextureSettings::CreateElement() const
{
	return MakeShared<FPCGSampleTextureElement>();
}

bool FPCGSampleTextureElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGSampleTextureElement::Execute);
	check(Context);

	const UPCGSampleTextureSettings* Settings = Context->GetInputSettings<UPCGSampleTextureSettings>();
	check(Settings);

	const TArray<FPCGTaggedData> PointInputs = Context->InputData.GetInputsByPin(PCGSampleTextureConstants::InputPointLabel);
	const TArray<FPCGTaggedData> BaseTextureInput = Context->InputData.GetInputsByPin(PCGSampleTextureConstants::InputTextureLabel);
	TArray<FPCGTaggedData>& Outputs = Context->OutputData.TaggedData;

	if (BaseTextureInput.Num() > 1)
	{
		PCGE_LOG(Warning, GraphAndLog, LOCTEXT("InvalidNumberOfTextureData", "Only 1 texture input is allowed."));
	}

	const UPCGBaseTextureData* BaseTextureData = !BaseTextureInput.IsEmpty() ? Cast<UPCGBaseTextureData>(BaseTextureInput[0].Data) : nullptr;
	if (!BaseTextureData)
	{
		return true;
	}

	auto DensityMergeFunc = PCGHelpers::GetDensityMergeFunction(Settings->DensityMergeFunction);

	for (int i = 0; i < PointInputs.Num(); ++i)
	{
		const UPCGPointData* PointData = Cast<UPCGPointData>(PointInputs[i].Data);
		if (!PointData)
		{
			PCGE_LOG(Warning, GraphAndLog, FText::Format(LOCTEXT("InvalidPointData", "Point Input {0} is not point data."), i));
			continue;
		}

		const TArray<FPCGPoint>& InputPoints = PointData->GetPoints();

		TUniquePtr<const IPCGAttributeAccessor> InputAccessor = nullptr;
		TUniquePtr<const IPCGAttributeAccessorKeys> InputKeys = nullptr;

		if (Settings->TextureMappingMethod == EPCGTextureMappingMethod::UVCoordinates)
		{
			const FPCGAttributePropertyInputSelector UVSource = Settings->UVCoordinatesAttribute.CopyAndFixLast(PointData);
	
			InputAccessor = PCGAttributeAccessorHelpers::CreateConstAccessor(PointData, UVSource);
			InputKeys = PCGAttributeAccessorHelpers::CreateConstKeys(PointData, UVSource);
			if (!InputAccessor || !InputKeys)
			{
				PCGE_LOG(Warning, GraphAndLog, FText::Format(LOCTEXT("InvalidUVAccessor", "Could not create coordinate accessor {0} for Point Input {1}."), FText::FromName(UVSource.GetName()), i));
				continue;
			}

			if (!PCG::Private::IsOfTypes<FVector, FVector2D>(InputAccessor->GetUnderlyingType()))
			{
				PCGE_LOG(Warning, GraphAndLog, FText::Format(LOCTEXT("InvalidAccessorType", "Accessor {0} must be of type Vector2 or Vector3"), FText::FromName(UVSource.GetName())));
				continue;
			}
		}

		UPCGPointData* OutPointData = FPCGContext::NewObject_AnyThread<UPCGPointData>(Context);
		OutPointData->InitializeFromData(PointData);

		auto ProcessPoint = [Settings, BaseTextureData, &InputAccessor, &InputKeys, &InputPoints, &DensityMergeFunc, OutPointData](int32 Index, FPCGPoint& OutPoint)
		{
			OutPoint = InputPoints[Index];

			if (Settings->TextureMappingMethod == EPCGTextureMappingMethod::UVCoordinates)
			{
				FVector OutSamplePosition;
				InputAccessor->Get(OutSamplePosition, Index, *InputKeys, EPCGAttributeAccessorFlags::AllowBroadcast | EPCGAttributeAccessorFlags::AllowConstructible);

				if (Settings->TilingMode == EPCGTextureAddressMode::Clamp)
				{
					OutSamplePosition.X = FMath::Clamp(OutSamplePosition.X, 0.0, 1.0);
					OutSamplePosition.Y = FMath::Clamp(OutSamplePosition.Y, 0.0, 1.0);
				}

				float SampleDensity = 1.0f;
				if (BaseTextureData->SamplePointLocal(FVector2D(OutSamplePosition), OutPoint.Color, SampleDensity))
				{
					float ComputedDensity = DensityMergeFunc(OutPoint.Density, SampleDensity);

					if (Settings->bClampOutputDensity)
					{
						ComputedDensity = FMath::Clamp(ComputedDensity, 0.0f, 1.0f);
					}

					OutPoint.Density = ComputedDensity;
					return true;
				}
				else
				{
					return false;
				}
			}
			else
			{
				if (BaseTextureData->SamplePoint(OutPoint.Transform, OutPoint.GetLocalBounds(), OutPoint, OutPointData->Metadata))
				{
					float ComputedDensity = DensityMergeFunc(InputPoints[Index].Density, OutPoint.Density);

					if (Settings->bClampOutputDensity)
					{
						ComputedDensity = FMath::Clamp(ComputedDensity, 0.0f, 1.0f);
					}

					OutPoint.Density = ComputedDensity;
					return true;
				}
				else
				{
					return false;
				}
			}
		};

		TArray<FPCGPoint>& OutPoints = OutPointData->GetMutablePoints();
		OutPoints.SetNumUninitialized(InputPoints.Num());
		FPCGTaggedData& Output = Outputs.Add_GetRef(PointInputs[i]);
		Output.Data = OutPointData;

		FPCGAsync::AsyncPointProcessing(Context, InputPoints.Num(), OutPoints, ProcessPoint);
	}

	return true;
}

#undef LOCTEXT_NAMESPACE
