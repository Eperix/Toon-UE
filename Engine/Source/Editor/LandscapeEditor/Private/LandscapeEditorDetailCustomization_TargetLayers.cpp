// Copyright Epic Games, Inc. All Rights Reserved.

#include "LandscapeEditorDetailCustomization_TargetLayers.h"
#include "IDetailChildrenBuilder.h"
#include "Framework/Commands/UIAction.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "Brushes/SlateColorBrush.h"
#include "Layout/WidgetPath.h"
#include "SlateOptMacros.h"
#include "Framework/Application/MenuStack.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "EditorModeManager.h"
#include "EditorModes.h"
#include "LandscapeEditorModule.h"
#include "LandscapeEditorObject.h"
#include "Landscape.h"
#include "LandscapeUtils.h"
#include "Styling/AppStyle.h"
#include "DetailLayoutBuilder.h"
#include "IDetailPropertyRow.h"
#include "DetailCategoryBuilder.h"
#include "PropertyCustomizationHelpers.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Materials/MaterialExpressionLandscapeVisibilityMask.h"

#include "SLandscapeEditor.h"
#include "Dialogs/DlgPickAssetPath.h"
#include "ObjectTools.h"
#include "ScopedTransaction.h"
#include "DesktopPlatformModule.h"

#include "LandscapeRender.h"
#include "LandscapeEdit.h"
#include "Landscape.h"
#include "LandscapeEditorUtils.h"

#define LOCTEXT_NAMESPACE "LandscapeEditor.TargetLayers"

TSharedRef<IDetailCustomization> FLandscapeEditorDetailCustomization_TargetLayers::MakeInstance()
{
	return MakeShareable(new FLandscapeEditorDetailCustomization_TargetLayers);
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FLandscapeEditorDetailCustomization_TargetLayers::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TSharedRef<IPropertyHandle> PropertyHandle_PaintingRestriction = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(ULandscapeEditorObject, PaintingRestriction));
	TSharedRef<IPropertyHandle> PropertyHandle_TargetDisplayOrder = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(ULandscapeEditorObject, TargetDisplayOrder));
	PropertyHandle_TargetDisplayOrder->MarkHiddenByCustomization();

	TSharedRef<IPropertyHandle> PropertyHandle_TargetShowUnusedLayers = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(ULandscapeEditorObject, ShowUnusedLayers));
	PropertyHandle_TargetShowUnusedLayers->MarkHiddenByCustomization();

	if (!ShouldShowTargetLayers())
	{
		PropertyHandle_PaintingRestriction->MarkHiddenByCustomization();

		return;
	}

	IDetailCategoryBuilder& TargetsCategory = DetailBuilder.EditCategory("Target Layers");
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	check(LandscapeEdMode);

	TargetsCategory.AddProperty(PropertyHandle_PaintingRestriction)
		.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateStatic(&FLandscapeEditorDetailCustomization_TargetLayers::GetPaintingRestrictionVisibility)))
		.IsEnabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([LandscapeEdMode]() { return LandscapeEdMode->HasValidLandscapeEditLayerSelection(); })));

	TargetsCategory.AddCustomRow(FText())
		.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateStatic(&FLandscapeEditorDetailCustomization_TargetLayers::GetVisibilityMaskTipVisibility)))
		[
			SNew(SMultiLineEditableTextBox)
				.IsReadOnly(true)
				.Font(DetailBuilder.GetDetailFontBold())
				.BackgroundColor(FAppStyle::GetColor("ErrorReporting.WarningBackgroundColor"))
				.Text(LOCTEXT("Visibility_Tip", "Note: There are some areas where visibility painting is disabled because Component/Proxy don't have a \"Landscape Visibility Mask\" node in their material."))
				.AutoWrapText(true)
				.IsEnabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([LandscapeEdMode]() { return LandscapeEdMode->HasValidLandscapeEditLayerSelection(); })))
		];

	TargetsCategory.AddCustomBuilder(MakeShareable(new FLandscapeEditorCustomNodeBuilder_TargetLayers(DetailBuilder.GetThumbnailPool().ToSharedRef(), PropertyHandle_TargetDisplayOrder, PropertyHandle_TargetShowUnusedLayers)));

	TargetsCategory.AddCustomRow(FText())
		.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateStatic(&FLandscapeEditorDetailCustomization_TargetLayers::GetPopulateTargetLayersInfoTipVisibility)))
		[
			SNew(SMultiLineEditableTextBox)
				.IsReadOnly(true)
				.Font(DetailBuilder.GetDetailFontBold())
				.BackgroundColor(FAppStyle::GetColor("InfoReporting.BackgroundColor"))
				.Text(LOCTEXT("PopulateTargetLayers_Tip", "There are currently no target layers assigned to this landscape. Use the buttons above to add new ones or populate them from the material(s) currently assigned to the landscape"))
				.AutoWrapText(true)
		];

	TargetsCategory.AddCustomRow(FText())
		.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateStatic(&FLandscapeEditorDetailCustomization_TargetLayers::GetFilteredTargetLayersListInfoTipVisibility)))
		[
			SNew(SMultiLineEditableTextBox)
				.IsReadOnly(true)
				.Font(DetailBuilder.GetDetailFontBold())
				.BackgroundColor(FAppStyle::GetColor("InfoReporting.BackgroundColor"))
				.Text(LOCTEXT("FilteredTargetLayers_Tip", "All target layers assigned to this landscape are currently filtered. Use the buttons and/or the filter above to un-hide them."))
				.AutoWrapText(true)
		];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

bool FLandscapeEditorDetailCustomization_TargetLayers::ShouldShowTargetLayers()
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode && LandscapeEdMode->CurrentToolMode)
	{
		const FName CurrentToolName = LandscapeEdMode->CurrentTool->GetToolName();

		//bool bSupportsHeightmap = (LandscapeEdMode->CurrentToolMode->SupportedTargetTypes & ELandscapeToolTargetTypeMask::Heightmap) != 0;
		//bool bSupportsWeightmap = (LandscapeEdMode->CurrentToolMode->SupportedTargetTypes & ELandscapeToolTargetTypeMask::Weightmap) != 0;
		//bool bSupportsVisibility = (LandscapeEdMode->CurrentToolMode->SupportedTargetTypes & ELandscapeToolTargetTypeMask::Visibility) != 0;

		//// Visible if there are possible choices
		//if (bSupportsWeightmap || bSupportsHeightmap || bSupportsVisibility)
		if (LandscapeEdMode->CurrentToolMode->SupportedTargetTypes != 0 
			&& CurrentToolName != TEXT("BlueprintBrush")
			&& CurrentToolName != TEXT("Mask"))
		{
			return true;
		}
	}

	return false;
}

EVisibility FLandscapeEditorDetailCustomization_TargetLayers::GetPaintingRestrictionVisibility()
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();

	if (LandscapeEdMode && LandscapeEdMode->CurrentToolMode)
	{
		const FName CurrentToolName = LandscapeEdMode->CurrentTool->GetToolName();

		// Tool target type "Invalid" means Weightmap with no valid paint layer, so technically, it is weightmap and we therefore choose to show PaintingRestriction : 
		if ((LandscapeEdMode->CurrentToolTarget.TargetType == ELandscapeToolTargetType::Weightmap && CurrentToolName != TEXT("BlueprintBrush"))
			|| (LandscapeEdMode->CurrentToolTarget.TargetType == ELandscapeToolTargetType::Invalid)
			|| (LandscapeEdMode->CurrentToolTarget.TargetType == ELandscapeToolTargetType::Visibility))
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}

EVisibility FLandscapeEditorDetailCustomization_TargetLayers::GetVisibilityMaskTipVisibility()
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode && LandscapeEdMode->CurrentToolTarget.LandscapeInfo.IsValid())
	{
		if (LandscapeEdMode->CurrentToolTarget.TargetType == ELandscapeToolTargetType::Visibility)
		{
			ULandscapeInfo* LandscapeInfo = LandscapeEdMode->CurrentToolTarget.LandscapeInfo.Get();
			bool bHasValidHoleMaterial = true;
			LandscapeInfo->ForAllLandscapeComponents([&](ULandscapeComponent* LandscapeComponent)
			{
				bHasValidHoleMaterial &= LandscapeComponent->IsLandscapeHoleMaterialValid();
			});

			return bHasValidHoleMaterial ? EVisibility::Collapsed : EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}

EVisibility FLandscapeEditorDetailCustomization_TargetLayers::GetPopulateTargetLayersInfoTipVisibility()
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode && LandscapeEdMode->CurrentToolTarget.LandscapeInfo.IsValid())
	{
		if ((LandscapeEdMode->CurrentToolTarget.TargetType == ELandscapeToolTargetType::Weightmap)
			|| (LandscapeEdMode->CurrentToolTarget.TargetType == ELandscapeToolTargetType::Invalid)) // ELandscapeToolTargetType::Invalid means "weightmap with no valid paint layer" 
		{
			ULandscapeInfo* LandscapeInfo = LandscapeEdMode->CurrentToolTarget.LandscapeInfo.Get();
			return LandscapeInfo->Layers.IsEmpty() ? EVisibility::Visible : EVisibility::Collapsed;
		}
	}

	return EVisibility::Collapsed;
}

EVisibility FLandscapeEditorDetailCustomization_TargetLayers::GetFilteredTargetLayersListInfoTipVisibility()
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode && LandscapeEdMode->CurrentToolTarget.LandscapeInfo.IsValid())
	{
		if ((LandscapeEdMode->CurrentToolTarget.TargetType == ELandscapeToolTargetType::Weightmap)
			|| (LandscapeEdMode->CurrentToolTarget.TargetType == ELandscapeToolTargetType::Invalid)) // ELandscapeToolTargetType::Invalid means "weightmap with no valid paint layer" 
		{
			const TArray<TSharedRef<FLandscapeTargetListInfo>>& TargetList = LandscapeEdMode->GetTargetList();
			// The first target layers are for heightmap and visibility so only consider target layers above the starting index : 
			const bool bHasTargetLayers = TargetList.Num() > LandscapeEdMode->GetTargetLayerStartingIndex();
			const TArray<TSharedRef<FLandscapeTargetListInfo>> TargetDisplayList = FLandscapeEditorCustomNodeBuilder_TargetLayers::PrepareTargetLayerList(/*bInSort =*/ false, /*bInFilter = */true);
			return (bHasTargetLayers && TargetDisplayList.IsEmpty()) ? EVisibility::Visible : EVisibility::Collapsed;
		}
	}

	return EVisibility::Collapsed;
}


//////////////////////////////////////////////////////////////////////////

FEdModeLandscape* FLandscapeEditorCustomNodeBuilder_TargetLayers::GetEditorMode()
{
	return (FEdModeLandscape*)GLevelEditorModeTools().GetActiveMode(FBuiltinEditorModes::EM_Landscape);
}

FLandscapeEditorCustomNodeBuilder_TargetLayers::FLandscapeEditorCustomNodeBuilder_TargetLayers(TSharedRef<FAssetThumbnailPool> InThumbnailPool, TSharedRef<IPropertyHandle> InTargetDisplayOrderPropertyHandle, TSharedRef<IPropertyHandle> InTargetShowUnusedLayersPropertyHandle)
	: ThumbnailPool(InThumbnailPool)
	, TargetDisplayOrderPropertyHandle(InTargetDisplayOrderPropertyHandle)
	, TargetShowUnusedLayersPropertyHandle(InTargetShowUnusedLayersPropertyHandle)
{
}

FLandscapeEditorCustomNodeBuilder_TargetLayers::~FLandscapeEditorCustomNodeBuilder_TargetLayers()
{
	FEdModeLandscape::TargetsListUpdated.RemoveAll(this);
}

void FLandscapeEditorCustomNodeBuilder_TargetLayers::SetOnRebuildChildren(FSimpleDelegate InOnRegenerateChildren)
{
	FEdModeLandscape::TargetsListUpdated.RemoveAll(this);
	if (InOnRegenerateChildren.IsBound())
	{
		FEdModeLandscape::TargetsListUpdated.Add(InOnRegenerateChildren);
	}
}

void FLandscapeEditorCustomNodeBuilder_TargetLayers::GenerateHeaderRowContent(FDetailWidgetRow& NodeRow)
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	
	if (LandscapeEdMode == nullptr)
	{
		return;	
	}

	NodeRow.NameWidget
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.Text(LOCTEXT("LayersLabel", "Layers"))
		];

	if (LandscapeEdMode->CurrentToolMode->SupportedTargetTypes & ELandscapeToolTargetTypeMask::Weightmap)
	{
		NodeRow.ValueWidget
		[
			SNew(SHorizontalBox)
			
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SComboButton)
				.ComboButtonStyle(FAppStyle::Get(), "SimpleComboButtonWithIcon")
				.ForegroundColor(FSlateColor::UseForeground())
				.HasDownArrow(true)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.ToolTipText(LOCTEXT("TargetLayerSortButtonTooltip", "Define how we want to sort the displayed layers"))
				.OnGetMenuContent(this, &FLandscapeEditorCustomNodeBuilder_TargetLayers::GetTargetLayerDisplayOrderButtonMenuContent)
				.ButtonContent()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew( SOverlay )
						+SOverlay::Slot()
						[
							SNew(SImage)
							.Image(FAppStyle::GetBrush("LandscapeEditor.Target_DisplayOrder.Default"))
						]	
						+SOverlay::Slot()
						[
							SNew(SImage)
							.Image(this, &FLandscapeEditorCustomNodeBuilder_TargetLayers::GetTargetLayerDisplayOrderBrush)
						]
					]
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(5.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SComboButton)
				.ComboButtonStyle(FAppStyle::Get(), "SimpleComboButtonWithIcon")
				.ForegroundColor(FSlateColor::UseForeground())
				.HasDownArrow(true)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.ToolTipText(LOCTEXT("TargetLayerUnusedLayerButtonTooltip", "Define if we want to display unused layers"))
				.OnGetMenuContent(this, &FLandscapeEditorCustomNodeBuilder_TargetLayers::GetTargetLayerShowUnusedButtonMenuContent)
				.ButtonContent()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SBox)
						.WidthOverride(16.0f)
						.HeightOverride(16.0f)
						[
							SNew(SImage)
								.Image(this, &FLandscapeEditorCustomNodeBuilder_TargetLayers::GetShowUnusedBrush)
						]
					]
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(5.0f, 0.0f, 0.0f, 0.0f)
			[
				PropertyCustomizationHelpers::MakeAddButton(FSimpleDelegate::CreateRaw(this, &FLandscapeEditorCustomNodeBuilder_TargetLayers::HandleCreateLayer), NSLOCTEXT("Landscape", "CreateLayer", "Create Layer"))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(5.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SButton)
				.ButtonStyle( FAppStyle::Get(), "SimpleButton" )
				.ToolTipText(NSLOCTEXT("Landscape", "CreateLayersFromMaterials", "Create Layers From Assigned Materials"))
				.OnClicked(this, &FLandscapeEditorCustomNodeBuilder_TargetLayers::HandleCreateLayersFromMaterials)
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("LandscapeEditor.Layer.Sync"))
				]
			]
		];
	}
	NodeRow.IsEnabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([LandscapeEdMode]() { return LandscapeEdMode->HasValidLandscapeEditLayerSelection(); })));
}



FReply FLandscapeEditorCustomNodeBuilder_TargetLayers::HandleCreateLayersFromMaterials()
{
	FScopedTransaction Transaction(LOCTEXT("LandscapeTargetLayer_CreateFromMaterials", "Create Target Layers from Assigned materials"));
	
	FEdModeLandscape* LandscapeEdMode = GetEditorMode(); 
	if (LandscapeEdMode == nullptr)
	{
		return FReply::Handled(); 
	}
	
	ALandscape* LandscapeActor = LandscapeEdMode->CurrentToolTarget.LandscapeInfo->LandscapeActor.Get();
	
	TSet<FName> LayerNames;
	LandscapeActor->GetLandscapeInfo()->ForEachLandscapeProxy([&LayerNames](ALandscapeProxy* Proxy)
	{
		LayerNames.Append(Proxy->RetrieveTargetLayerNamesFromMaterials());
		return true;
	});

	UE::Landscape::FLayerInfoFinder LayerInfoFinder;
	for (const FName& LayerName : LayerNames)
	{
		if (!LandscapeActor->GetTargetLayers().Find(LayerName))
		{
			ULandscapeLayerInfoObject* LandscapeLayerInfo = LayerInfoFinder.Find(LayerName);
			LandscapeActor->AddTargetLayer(LayerName, FLandscapeTargetLayerSettings(LandscapeLayerInfo));
		}
	}
	
	if (LandscapeEdMode)
	{
		LandscapeEdMode->GetLandscape()->GetLandscapeInfo()->UpdateLayerInfoMap();
		LandscapeEdMode->UpdateTargetList();
	}

	return FReply::Handled();
}

void FLandscapeEditorCustomNodeBuilder_TargetLayers::HandleCreateLayer()
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode == nullptr)
	{
		return; 
	}
	
	TWeakObjectPtr<ALandscape> Landscape = LandscapeEdMode->CurrentToolTarget.LandscapeInfo->LandscapeActor;

	if (!Landscape.Get())
	{
		return; 
	}

	FScopedTransaction Transaction(LOCTEXT("LandscapeTargetLayer_Create", "Create a Target Layer"));
	
	Landscape->AddTargetLayer();
	
	LandscapeEdMode->CurrentToolTarget.LandscapeInfo->UpdateLayerInfoMap();
	LandscapeEdMode->UpdateTargetList();
	LandscapeEdMode->RefreshDetailPanel();
}

TSharedRef<SWidget> FLandscapeEditorCustomNodeBuilder_TargetLayers::GetTargetLayerDisplayOrderButtonMenuContent()
{
	FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection=*/true, nullptr, nullptr, /*bCloseSelfOnly=*/ true);

	MenuBuilder.BeginSection("TargetLayerSortType", LOCTEXT("SortTypeHeading", "Sort Type"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("TargetLayerDisplayOrderDefault", "Default"),
			LOCTEXT("TargetLayerDisplayOrderDefaultToolTip", "Sort using order defined in the material."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &FLandscapeEditorCustomNodeBuilder_TargetLayers::SetSelectedDisplayOrder, ELandscapeLayerDisplayMode::Default),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &FLandscapeEditorCustomNodeBuilder_TargetLayers::IsSelectedDisplayOrder, ELandscapeLayerDisplayMode::Default)
			),
			NAME_None,
			EUserInterfaceActionType::RadioButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("TargetLayerDisplayOrderAlphabetical", "Alphabetical"),
			LOCTEXT("TargetLayerDisplayOrderAlphabeticalToolTip", "Sort using alphabetical order."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &FLandscapeEditorCustomNodeBuilder_TargetLayers::SetSelectedDisplayOrder, ELandscapeLayerDisplayMode::Alphabetical),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &FLandscapeEditorCustomNodeBuilder_TargetLayers::IsSelectedDisplayOrder, ELandscapeLayerDisplayMode::Alphabetical)
			),
			NAME_None,
			EUserInterfaceActionType::RadioButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("TargetLayerDisplayOrderCustom", "Custom"),
			LOCTEXT("TargetLayerDisplayOrderCustomToolTip", "This sort options will be set when changing manually display order by dragging layers"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &FLandscapeEditorCustomNodeBuilder_TargetLayers::SetSelectedDisplayOrder, ELandscapeLayerDisplayMode::UserSpecific),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &FLandscapeEditorCustomNodeBuilder_TargetLayers::IsSelectedDisplayOrder, ELandscapeLayerDisplayMode::UserSpecific)
			),
			NAME_None,
			EUserInterfaceActionType::RadioButton
		);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> FLandscapeEditorCustomNodeBuilder_TargetLayers::GetTargetLayerShowUnusedButtonMenuContent()
{
	FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection=*/true, nullptr, nullptr, /*bCloseSelfOnly=*/ true);

	MenuBuilder.BeginSection("TargetLayerUnusedType", LOCTEXT("UnusedTypeHeading", "Layer Visibility"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("TargetLayerShowUnusedLayer", "Show all layers"),
			LOCTEXT("TargetLayerShowUnusedLayerToolTip", "Show all layers"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &FLandscapeEditorCustomNodeBuilder_TargetLayers::ShowUnusedLayers, true),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &FLandscapeEditorCustomNodeBuilder_TargetLayers::ShouldShowUnusedLayers, true)
			),
			NAME_None,
			EUserInterfaceActionType::RadioButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("TargetLayerHideUnusedLayer", "Hide unused layers"),
			LOCTEXT("TargetLayerHideUnusedLayerToolTip", "Only show used layer"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &FLandscapeEditorCustomNodeBuilder_TargetLayers::ShowUnusedLayers, false),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &FLandscapeEditorCustomNodeBuilder_TargetLayers::ShouldShowUnusedLayers, false)
			),
			NAME_None,
			EUserInterfaceActionType::RadioButton
		);
	}

	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

const FSlateBrush* FLandscapeEditorCustomNodeBuilder_TargetLayers::GetShowUnusedBrush() const
{
	const FSlateBrush* Brush = FAppStyle::GetBrush("Level.VisibleIcon16x");
	if (FEdModeLandscape* LandscapeEdMode = GetEditorMode(); (LandscapeEdMode != nullptr) && !LandscapeEdMode->UISettings->ShowUnusedLayers)
	{
		Brush = FAppStyle::GetBrush("Level.NotVisibleIcon16x");
	}
	return Brush;
}

void FLandscapeEditorCustomNodeBuilder_TargetLayers::ShowUnusedLayers(bool Result)
{
	TargetShowUnusedLayersPropertyHandle->SetValue(Result);
}

bool FLandscapeEditorCustomNodeBuilder_TargetLayers::ShouldShowUnusedLayers(bool Result) const
{
	if (FEdModeLandscape* LandscapeEdMode = GetEditorMode())
	{
		return LandscapeEdMode->UISettings->ShowUnusedLayers == Result;
	}

	return false;
}

void FLandscapeEditorCustomNodeBuilder_TargetLayers::SetSelectedDisplayOrder(ELandscapeLayerDisplayMode InDisplayOrder)
{
	TargetDisplayOrderPropertyHandle->SetValue((uint8)InDisplayOrder);	
}

bool FLandscapeEditorCustomNodeBuilder_TargetLayers::IsSelectedDisplayOrder(ELandscapeLayerDisplayMode InDisplayOrder) const
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode != nullptr)
	{
		return LandscapeEdMode->UISettings->TargetDisplayOrder == InDisplayOrder;
	}

	return false;
}

const FSlateBrush* FLandscapeEditorCustomNodeBuilder_TargetLayers::GetTargetLayerDisplayOrderBrush() const
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode != nullptr)
	{
		switch (LandscapeEdMode->UISettings->TargetDisplayOrder)
		{
			case ELandscapeLayerDisplayMode::Alphabetical: return FAppStyle::Get().GetBrush("LandscapeEditor.Target_DisplayOrder.Alphabetical");
			case ELandscapeLayerDisplayMode::UserSpecific: return FAppStyle::Get().GetBrush("LandscapeEditor.Target_DisplayOrder.Custom");
			default:
				return nullptr;
		}
	}

	return nullptr;
}

EVisibility FLandscapeEditorCustomNodeBuilder_TargetLayers::ShouldShowLayer(TSharedRef<FLandscapeTargetListInfo> Target) const
{
	if ((Target->TargetType == ELandscapeToolTargetType::Weightmap)
		|| (Target->TargetType == ELandscapeToolTargetType::Invalid)) // Invalid means weightmap with no selected target layer
	{
		FEdModeLandscape* LandscapeEdMode = GetEditorMode();

		if (LandscapeEdMode != nullptr)
		{
			return LandscapeEdMode->ShouldShowLayer(Target) ? EVisibility::Visible : EVisibility::Collapsed;
		}
	}

	return EVisibility::Visible;
}

void FLandscapeEditorCustomNodeBuilder_TargetLayers::OnFilterTextChanged(const FText& InFilterText)
{
	if (FEdModeLandscape* LandscapeEdMode = GetEditorMode())
	{
		LandscapeEdMode->UISettings->TargetLayersFilterString = InFilterText.ToString();
	}
}

void FLandscapeEditorCustomNodeBuilder_TargetLayers::OnFilterTextCommitted(const FText& InFilterText, ETextCommit::Type InCommitType)
{
	if (InCommitType == ETextCommit::OnCleared)
	{
		LayersFilterSearchBox->SetText(FText::GetEmpty());
		OnFilterTextChanged(FText::GetEmpty());
		FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::Cleared);
	}
}

EVisibility FLandscapeEditorCustomNodeBuilder_TargetLayers::GetLayersFilterVisibility() const
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();

	if (LandscapeEdMode && LandscapeEdMode->CurrentToolMode)
	{
		// ELandscapeToolTargetType::Invalid means "weightmap with no valid paint layer" so we still want to display that property if it has been marked to be displayed in Weightmap target type, to be consistent 
		if ((LandscapeEdMode->CurrentToolTarget.TargetType == ELandscapeToolTargetType::Weightmap)
			|| (LandscapeEdMode->CurrentToolTarget.TargetType == ELandscapeToolTargetType::Invalid))
		{
			const bool bContainsWeightmapLayers = LandscapeEdMode->GetTargetList().ContainsByPredicate([](TSharedRef<FLandscapeTargetListInfo> InInfo)
				{
					FName LayerName = InInfo->GetLayerName();
					return (LayerName != NAME_None) && (LayerName != UMaterialExpressionLandscapeVisibilityMask::ParameterName);
				});

			return bContainsWeightmapLayers ? EVisibility::Visible : EVisibility::Collapsed;
		}
	}

	return EVisibility::Collapsed;
}

FText FLandscapeEditorCustomNodeBuilder_TargetLayers::GetLayersFilterText() const
{
	if (FEdModeLandscape* LandscapeEdMode = GetEditorMode())
	{
		return FText::FromString(LandscapeEdMode->UISettings->TargetLayersFilterString);
	}

	return FText();
}

TArray<TSharedRef<FLandscapeTargetListInfo>> FLandscapeEditorCustomNodeBuilder_TargetLayers::PrepareTargetLayerList(bool bInSort, bool bInFilter)
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode == nullptr)
	{
		return {};
	}
	const TArray<TSharedRef<FLandscapeTargetListInfo>>& TargetList = LandscapeEdMode->GetTargetList();
	const TArray<FName>* TargetDisplayOrderList = LandscapeEdMode->GetTargetDisplayOrderList();
	if (TargetDisplayOrderList == nullptr)
	{
		return {};
	}

	TArray<TSharedRef<FLandscapeTargetListInfo>> FinalList(TargetList);
	if (bInFilter)
	{
		FinalList.RemoveAllSwap([LandscapeEdMode](TSharedRef<FLandscapeTargetListInfo> InTargetInfo)
			{
				return !LandscapeEdMode->ShouldShowLayer(InTargetInfo);
			});
	}

	if (bInSort)
	{
		Algo::SortBy(FinalList, [TargetDisplayOrderList](TSharedRef<FLandscapeTargetListInfo> InTargetInfo)
			{
				return TargetDisplayOrderList->Find(InTargetInfo->GetLayerName());
			});
	}
	return FinalList;
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FLandscapeEditorCustomNodeBuilder_TargetLayers::GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder)
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode != nullptr)
	{
		TSharedPtr<SDragAndDropVerticalBox> TargetLayerList = SNew(SDragAndDropVerticalBox)
			.OnCanAcceptDrop(this, &FLandscapeEditorCustomNodeBuilder_TargetLayers::HandleCanAcceptDrop)
			.OnAcceptDrop(this, &FLandscapeEditorCustomNodeBuilder_TargetLayers::HandleAcceptDrop)
			.OnDragDetected(this, &FLandscapeEditorCustomNodeBuilder_TargetLayers::HandleDragDetected)
			.IsEnabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([LandscapeEdMode]() { return LandscapeEdMode->HasValidLandscapeEditLayerSelection(); })));

		TargetLayerList->SetDropIndicator_Above(*FAppStyle::GetBrush("LandscapeEditor.TargetList.DropZone.Above"));
		TargetLayerList->SetDropIndicator_Below(*FAppStyle::GetBrush("LandscapeEditor.TargetList.DropZone.Below"));

		ChildrenBuilder.AddCustomRow(LOCTEXT("LayersLabel", "Layers"))
			.Visibility(EVisibility::Visible)
			[
				SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.VAlign(VAlign_Top)
					.HAlign(HAlign_Fill)
					.Padding(FMargin(2))
					[
						SAssignNew(LayersFilterSearchBox, SSearchBox)
							.InitialText(this, &FLandscapeEditorCustomNodeBuilder_TargetLayers::GetLayersFilterText)
							.SelectAllTextWhenFocused(true)
							.HintText(LOCTEXT("LayersSearch", "Filter Target Layers"))
							.OnTextChanged(this, &FLandscapeEditorCustomNodeBuilder_TargetLayers::OnFilterTextChanged)
							.OnTextCommitted(this, &FLandscapeEditorCustomNodeBuilder_TargetLayers::OnFilterTextCommitted)
							.Visibility(this, &FLandscapeEditorCustomNodeBuilder_TargetLayers::GetLayersFilterVisibility)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.VAlign(VAlign_Top)
					.HAlign(HAlign_Fill)
					.Padding(0.0f, 0.0f)
					[
						TargetLayerList.ToSharedRef()
					]
			];

		// Generate a row for all target layers, including those that will be filtered and let the row's visibility lambda to compute their visibility dynamically. This allows 
		//  filtering to work without refreshing the details panel (which causes the search box to lose focus) :
		for (const TSharedRef<FLandscapeTargetListInfo>& TargetInfo : PrepareTargetLayerList(/*bInSort = */true, /*bInFilter = */false))
		{
			TSharedPtr<SWidget> GeneratedRowWidget = GenerateRow(TargetInfo);
			if (GeneratedRowWidget.IsValid())
			{
				TargetLayerList->AddSlot()
					.AutoHeight()
					[
						GeneratedRowWidget.ToSharedRef()
					];
			}
		}
	}
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
TSharedPtr<SWidget> FLandscapeEditorCustomNodeBuilder_TargetLayers::GenerateRow(const TSharedRef<FLandscapeTargetListInfo> Target)
{
	TSharedPtr<SWidget> RowWidget;

	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode)
	{
		if ((LandscapeEdMode->CurrentTool->GetSupportedTargetTypes() & LandscapeEdMode->CurrentToolMode->SupportedTargetTypes & ELandscapeToolTargetTypeMask::FromType(Target->TargetType)) == 0)
		{
			return RowWidget;
		}
	}

	if (Target->TargetType != ELandscapeToolTargetType::Weightmap)
	{
		RowWidget = SNew(SLandscapeEditorSelectableBorder)
			.Padding(0.0f)
			.VAlign(VAlign_Center)
			.OnContextMenuOpening_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::OnTargetLayerContextMenuOpening, Target)
			.OnSelected_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::OnTargetSelectionChanged, Target)
			.IsSelected_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::GetTargetLayerIsSelected, Target)
			.Visibility(this, &FLandscapeEditorCustomNodeBuilder_TargetLayers::ShouldShowLayer, Target)
			[
				SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(FMargin(2))
					[
						SNew(SImage)
							.Image(FAppStyle::GetBrush(Target->TargetType == ELandscapeToolTargetType::Heightmap ? TEXT("LandscapeEditor.Target_Heightmap") : TEXT("LandscapeEditor.Target_Visibility")))
					]
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.Padding(4, 0)
					[
						SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							.VAlign(VAlign_Center)
							.Padding(0, 2)
							[
								SNew(STextBlock)
									.Font(IDetailLayoutBuilder::GetDetailFont())
									.Text(Target->TargetLayerDisplayName)
									.ColorAndOpacity_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::GetTargetTextColor, Target)
							]
					]
			];
	}
	else
	{
		static const FSlateColorBrush SolidWhiteBrush = FSlateColorBrush(FColorList::White);

		RowWidget = SNew(SLandscapeEditorSelectableBorder)
			.Padding(0.0f)
			.VAlign(VAlign_Center)
			.OnContextMenuOpening_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::OnTargetLayerContextMenuOpening, Target)
			.OnSelected_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::OnTargetSelectionChanged, Target)
			.IsSelected_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::GetTargetLayerIsSelected, Target)
			.Visibility(this, &FLandscapeEditorCustomNodeBuilder_TargetLayers::ShouldShowLayer, Target)
			[
				SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SBox)
							.Padding(FMargin(2.0f, 0.0f, 2.0f, 0.0f))
							[
								SNew(SImage)
									.Image(FCoreStyle::Get().GetBrush("VerticalBoxDragIndicator"))
							]
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(FMargin(2))
					[
						SNew(SBox)
							.Visibility_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::GetDebugModeLayerUsageVisibility, Target)
							.WidthOverride(48.0f)
							.HeightOverride(48.0f)
							[
								SNew(SImage)
									.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
									.ColorAndOpacity_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::GetLayerUsageDebugColor, Target)
							]
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(FMargin(2))
					[
						(Target->bValid)
							? (TSharedRef<SWidget>)(
								SNew(SLandscapeAssetThumbnail, Target->ThumbnailMIC.Get(), ThumbnailPool)
								.Visibility_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::GetDebugModeLayerUsageVisibility_Invert, Target)
								.ThumbnailSize(FIntPoint(48, 48))
								// Open landscape layer info asset on double-click on the thumbnail : 
								.OnAccessAsset_Lambda([Target](UObject* InObject)
								{ 
									// Note : the object being returned here is the landscape MIC so it's not what we use for opening the landscape layer info asset : 
									if ((Target->TargetType == ELandscapeToolTargetType::Weightmap) && (Target->LayerInfoObj != nullptr))
									{
										UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
										return AssetEditorSubsystem->OpenEditorForAsset(Target->LayerInfoObj.Get());
									}
									return false;
								}))
							: (TSharedRef<SWidget>)(
								SNew(SImage)
								.Visibility_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::GetDebugModeLayerUsageVisibility_Invert, Target)
								.Image(FAppStyle::GetBrush(TEXT("LandscapeEditor.Target_Invalid")))
								)
					]
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.Padding(4, 0)
					[
						SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							.VAlign(VAlign_Center)
							.Padding(4, 3, 0, 3)
							[
								SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									[
										SNew(SInlineEditableTextBlock)
											.Font(IDetailLayoutBuilder::GetDetailFontBold())
											.Text(Target->TargetLayerDisplayName)
											.ColorAndOpacity_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::GetTargetTextColor, Target)
											.OnVerifyTextChanged_Lambda([Target](const FText& InNewText, FText& OutErrorMessage)
											{
												const FName NewName(InNewText.ToString());

												if (Target->LayerName == NewName)
												{
													return true;
												}

												if (NewName == UMaterialExpressionLandscapeVisibilityMask::ParameterName)
												{
													OutErrorMessage = LOCTEXT("LandscapeTargetLayer_RenameFailed_ReservedName", "This target layer name is reserved for internal usage");
													return false;
												}

												ALandscape* Landscape = Cast<ALandscape>(Target->Owner);
												if (Landscape->HasTargetLayer(NewName))
												{
													OutErrorMessage = LOCTEXT("LandscapeTargetLayer_RenameFailed_AlreadyExists", "This target layer name already exists");
													return false;
												}

												return true;
											})
											.OnTextCommitted_Lambda([Target](const FText& Text, ETextCommit::Type Type)
											{
												const FName NewName(Text.ToString());
												if (Target->LayerName == NewName)
												{
													return;
												}

												FScopedTransaction Transaction(LOCTEXT("LandscapeTargetLayer_Rename", "Rename Target Layer"));
												ALandscape* Landscape = Cast<ALandscape>(Target->Owner);
												
												const TMap<FName, FLandscapeTargetLayerSettings>& TargetLayers = Landscape->GetTargetLayers();
												const FLandscapeTargetLayerSettings* LayerSettings = nullptr;
												
												Landscape->RemoveTargetLayer(FName(Target->TargetLayerDisplayName.ToString()));
											
												Target->TargetLayerDisplayName = Text;
												Target->LayerName = FName(Text.ToString());
												Landscape->AddTargetLayer(Target->LayerName, LayerSettings ? *LayerSettings : FLandscapeTargetLayerSettings());	
												
												Target->LandscapeInfo->UpdateLayerInfoMap();
												if (FEdModeLandscape* LandscapeEdMode = GetEditorMode())
												{
													LandscapeEdMode->UpdateTargetList();
												}
											})
									]
									+ SHorizontalBox::Slot()
									.HAlign(HAlign_Right)
									[
										SNew(STextBlock)
											.Visibility_Lambda([=] { return (Target->LayerInfoObj.IsValid() && Target->LayerInfoObj->bNoWeightBlend) ? EVisibility::Visible : EVisibility::Collapsed; })
											.Font(IDetailLayoutBuilder::GetDetailFont())
											.Text(LOCTEXT("NoWeightBlend", "No Weight-Blend"))
											.ColorAndOpacity_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::GetTargetTextColor, Target)
									]
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.VAlign(VAlign_Center)
							[
								SNew(SHorizontalBox)
									.Visibility_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::GetTargetLayerInfoSelectorVisibility, Target)
									+ SHorizontalBox::Slot()
									.FillWidth(1)
									.VAlign(VAlign_Center)
									[
										SNew(SObjectPropertyEntryBox)
											.IsEnabled((bool)Target->bValid)
											.ObjectPath(Target->LayerInfoObj != nullptr ? Target->LayerInfoObj->GetPathName() : FString())
											.AllowedClass(ULandscapeLayerInfoObject::StaticClass())
											.OnObjectChanged_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::OnTargetLayerSetObject, Target)
											.OnShouldFilterAsset_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::ShouldFilterLayerInfo, Target->LayerName)
											.AllowCreate(false)
											.AllowClear(false)
									]
									+ SHorizontalBox::Slot()
									.AutoWidth()
									.VAlign(VAlign_Center)
									[
										SNew(SComboButton)
											.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
											.HasDownArrow(false)
											.ContentPadding(4.0f)
											.ForegroundColor(FSlateColor::UseForeground())
											.IsFocusable(false)
											.ToolTipText(LOCTEXT("Tooltip_Create", "Create Layer Info"))
											.IsEnabled_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::GetTargetLayerCreateEnabled, Target)
											.OnGetMenuContent_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::OnGetTargetLayerCreateMenu, Target)
											.ButtonContent()
											[
												SNew(SImage)
													.Image(FAppStyle::GetBrush("LandscapeEditor.Target_Create"))
											]
									]
									+ SHorizontalBox::Slot()
									.AutoWidth()
									.VAlign(VAlign_Center)
									[
										SNew(SButton)
											.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
											.ContentPadding(4.0f)
											.ForegroundColor(FSlateColor::UseForeground())
											.IsFocusable(false)
											.ToolTipText(LOCTEXT("Tooltip_MakePublic", "Make Layer Public (move layer info into asset file)"))
											.Visibility_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::GetTargetLayerMakePublicVisibility, Target)
											.OnClicked_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::OnTargetLayerMakePublicClicked, Target)
											[
												SNew(SImage)
													.Image(FAppStyle::GetBrush("LandscapeEditor.Target_MakePublic"))
											]
									]
									+ SHorizontalBox::Slot()
									.AutoWidth()
									.VAlign(VAlign_Center)
									[
										SNew(SButton)
											.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
											.ContentPadding(4.0f)
											.ForegroundColor(FSlateColor::UseForeground())
											.IsFocusable(false)
											.ToolTipText(LOCTEXT("Tooltip_Delete", "Delete Layer"))
											.OnClicked_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::OnTargetLayerDeleteClicked, Target)
											[
												SNew(SImage)
													.Image(FAppStyle::GetBrush("LandscapeEditor.Target_Delete"))
											]
									]
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SHorizontalBox)
									.Visibility_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::GetLayersSubstractiveBlendVisibility, Target)
									+ SHorizontalBox::Slot()
									.AutoWidth()
									.Padding(0, 2, 2, 2)
									[
										SNew(SCheckBox)
											.IsChecked_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::IsLayersSubstractiveBlendChecked, Target)
											.OnCheckStateChanged_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::OnLayersSubstractiveBlendChanged, Target)
											[
												SNew(STextBlock)
													.Text(LOCTEXT("SubtractiveBlend", "Subtractive Blend"))
													.ColorAndOpacity_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::GetTargetTextColor, Target)
											]
									]
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SHorizontalBox)
									.Visibility_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::GetDebugModeColorChannelVisibility, Target)
									+ SHorizontalBox::Slot()
									.AutoWidth()
									.Padding(0, 2, 2, 2)
									[
										SNew(SCheckBox)
											.IsChecked_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::DebugModeColorChannelIsChecked, Target, 0)
											.OnCheckStateChanged_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::OnDebugModeColorChannelChanged, Target, 0)
											[
												SNew(STextBlock)
													.Text(LOCTEXT("ViewMode.Debug_None", "None"))
											]
									]
									+ SHorizontalBox::Slot()
									.AutoWidth()
									.Padding(2)
									[
										SNew(SCheckBox)
											.IsChecked_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::DebugModeColorChannelIsChecked, Target, 1)
											.OnCheckStateChanged_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::OnDebugModeColorChannelChanged, Target, 1)
											[
												SNew(STextBlock)
													.Text(LOCTEXT("ViewMode.Debug_R", "R"))
											]
									]
									+ SHorizontalBox::Slot()
									.AutoWidth()
									.Padding(2)
									[
										SNew(SCheckBox)
											.IsChecked_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::DebugModeColorChannelIsChecked, Target, 2)
											.OnCheckStateChanged_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::OnDebugModeColorChannelChanged, Target, 2)
											[
												SNew(STextBlock)
													.Text(LOCTEXT("ViewMode.Debug_G", "G"))
											]
									]
									+ SHorizontalBox::Slot()
									.AutoWidth()
									.Padding(2)
									[
										SNew(SCheckBox)
											.IsChecked_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::DebugModeColorChannelIsChecked, Target, 4)
											.OnCheckStateChanged_Static(&FLandscapeEditorCustomNodeBuilder_TargetLayers::OnDebugModeColorChannelChanged, Target, 4)
											[
												SNew(STextBlock)
													.Text(LOCTEXT("ViewMode.Debug_B", "B"))
											]
									]
							]
					]
			];
	}

	return RowWidget;
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

FReply FLandscapeEditorCustomNodeBuilder_TargetLayers::HandleDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, int32 SlotIndex, SVerticalBox::FSlot* Slot)
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();

	if (LandscapeEdMode != nullptr)
	{
		// The slot index corresponds to what is actually shown, so we need to both sort and filter the target layer list here :
		TArray<TSharedRef<FLandscapeTargetListInfo>> TargetDisplayList = PrepareTargetLayerList(/*bInSort =*/ true, /*bInFilter = */true);
		if (TargetDisplayList.IsValidIndex(SlotIndex))
		{
			if (const TArray<FName>* TargetDisplayOrderList = LandscapeEdMode->GetTargetDisplayOrderList())
			{
				TSharedPtr<SWidget> Row = GenerateRow(TargetDisplayList[SlotIndex]);
				if (Row.IsValid())
				{
					return FReply::Handled().BeginDragDrop(FTargetLayerDragDropOp::New(SlotIndex, Slot, Row));
				}
			}
		}
	}

	return FReply::Unhandled();
}

TOptional<SDragAndDropVerticalBox::EItemDropZone> FLandscapeEditorCustomNodeBuilder_TargetLayers::HandleCanAcceptDrop(const FDragDropEvent& DragDropEvent, SDragAndDropVerticalBox::EItemDropZone DropZone, SVerticalBox::FSlot* Slot)
{
	TSharedPtr<FTargetLayerDragDropOp> DragDropOperation = DragDropEvent.GetOperationAs<FTargetLayerDragDropOp>();

	if (DragDropOperation.IsValid())
	{
		return DropZone;
	}

	return TOptional<SDragAndDropVerticalBox::EItemDropZone>();
}

FReply FLandscapeEditorCustomNodeBuilder_TargetLayers::HandleAcceptDrop(FDragDropEvent const& DragDropEvent, SDragAndDropVerticalBox::EItemDropZone DropZone, int32 SlotIndex, SVerticalBox::FSlot* Slot)
{
	TSharedPtr<FTargetLayerDragDropOp> DragDropOperation = DragDropEvent.GetOperationAs<FTargetLayerDragDropOp>();

	if (DragDropOperation.IsValid())
	{
		FEdModeLandscape* LandscapeEdMode = GetEditorMode();

		if (LandscapeEdMode != nullptr)
		{
			// The slot index corresponds to what is actually shown, so we need to both sort and filter the target layer list here :
			TArray<TSharedRef<FLandscapeTargetListInfo>> TargetDisplayList = PrepareTargetLayerList(/*bInSort =*/ true, /*bInFilter = */true);

			if (TargetDisplayList.IsValidIndex(DragDropOperation->SlotIndexBeingDragged) && TargetDisplayList.IsValidIndex(SlotIndex))
			{
				const FName TargetLayerNameBeingDragged = TargetDisplayList[DragDropOperation->SlotIndexBeingDragged]->GetLayerName();
				const FName DestinationTargetLayerName = TargetDisplayList[SlotIndex]->GetLayerName();
				if (const TArray<FName>* TargetDisplayOrderList = LandscapeEdMode->GetTargetDisplayOrderList())
				{
					int32 StartingLayerIndex = TargetDisplayOrderList->Find(TargetLayerNameBeingDragged);
					int32 DestinationLayerIndex = TargetDisplayOrderList->Find(DestinationTargetLayerName);
					if (StartingLayerIndex != INDEX_NONE && DestinationLayerIndex != INDEX_NONE)
					{
						LandscapeEdMode->MoveTargetLayerDisplayOrder(StartingLayerIndex, DestinationLayerIndex);
						return FReply::Handled();
					}
				}
			}
		}
	}

	return FReply::Unhandled();
}

bool FLandscapeEditorCustomNodeBuilder_TargetLayers::GetTargetLayerIsSelected(const TSharedRef<FLandscapeTargetListInfo> Target)
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode)
	{
		return
			LandscapeEdMode->CurrentToolTarget.TargetType == Target->TargetType &&
			LandscapeEdMode->CurrentToolTarget.LayerName == Target->LayerName &&
			LandscapeEdMode->CurrentToolTarget.LayerInfo == Target->LayerInfoObj; // may be null
	}

	return false;
}

void FLandscapeEditorCustomNodeBuilder_TargetLayers::OnTargetSelectionChanged(const TSharedRef<FLandscapeTargetListInfo> Target)
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode)
	{
		LandscapeEdMode->CurrentToolTarget.TargetType = Target->TargetType;
		if (Target->TargetType == ELandscapeToolTargetType::Heightmap)
		{
			checkSlow(Target->LayerInfoObj == nullptr);
			LandscapeEdMode->SetCurrentTargetLayer(NAME_None, nullptr);
		}
		else
		{
			LandscapeEdMode->SetCurrentTargetLayer(Target->LayerName, Target->LayerInfoObj);
		}
	}
}

TSharedPtr<SWidget> FLandscapeEditorCustomNodeBuilder_TargetLayers::OnTargetLayerContextMenuOpening(const TSharedRef<FLandscapeTargetListInfo> Target)
{
	if (Target->TargetType == ELandscapeToolTargetType::Heightmap || Target->LayerInfoObj != nullptr)
	{
		FMenuBuilder MenuBuilder(true, nullptr);
		
		MenuBuilder.BeginSection("LandscapeEditorLayerActions", LOCTEXT("LayerContextMenu.Heading", "Layer Actions"));
		{
			FEdModeLandscape* LandscapeEdMode = GetEditorMode();
			if (LandscapeEdMode)
			{
				FUIAction LandscapeHeightmapChangeToolsAction = FUIAction(FExecuteAction::CreateStatic(&FLandscapeEditorCustomNodeBuilder_TargetLayers::OnHeightmapLayerContextMenu, Target));
				MenuBuilder.AddMenuEntry(LOCTEXT("LayerContextMenu.Heightmap", "Import From/Export To File..."),
										LOCTEXT("LayerContextMenu.HeightmapToolTip", "Opens the Landscape Import tool in order to import / export heightmaps from / to external files."), FSlateIcon(), LandscapeHeightmapChangeToolsAction);
			}

			if (Target->TargetType == ELandscapeToolTargetType::Weightmap)
			{
				MenuBuilder.AddMenuSeparator();

				// Fill
				FUIAction FillAction = FUIAction(FExecuteAction::CreateStatic(&FLandscapeEditorCustomNodeBuilder_TargetLayers::OnFillLayer, Target));
				MenuBuilder.AddMenuEntry(LOCTEXT("LayerContextMenu.Fill", "Fill Layer"), LOCTEXT("LayerContextMenu.Fill_Tooltip", "Fills this layer to 100% across the entire landscape. If this is a weight-blended (normal) layer, all other weight-blended layers will be cleared."), FSlateIcon(), FillAction);

				// Clear
				FUIAction ClearAction = FUIAction(FExecuteAction::CreateStatic(&FLandscapeEditorCustomNodeBuilder_TargetLayers::OnClearLayer, Target));
				MenuBuilder.AddMenuEntry(LOCTEXT("LayerContextMenu.Clear", "Clear Layer"), LOCTEXT("LayerContextMenu.Clear_Tooltip", "Clears this layer to 0% across the entire landscape. If this is a weight-blended (normal) layer, other weight-blended layers will be adjusted to compensate."), FSlateIcon(), ClearAction);

				// Rebuild material instances
				FUIAction RebuildAction = FUIAction(FExecuteAction::CreateStatic(&FLandscapeEditorCustomNodeBuilder_TargetLayers::OnRebuildMICs, Target));
				MenuBuilder.AddMenuEntry(LOCTEXT("LayerContextMenu.Rebuild", "Rebuild Materials"), LOCTEXT("LayerContextMenu.Rebuild_Tooltip", "Rebuild material instances used for this landscape."), FSlateIcon(), RebuildAction);
			}
			else if (Target->TargetType == ELandscapeToolTargetType::Visibility)
			{
				MenuBuilder.AddMenuSeparator();

				// Clear
				FUIAction ClearAction = FUIAction(FExecuteAction::CreateStatic(&FLandscapeEditorCustomNodeBuilder_TargetLayers::OnClearLayer, Target));
				MenuBuilder.AddMenuEntry(LOCTEXT("LayerContextMenu.ClearHoles", "Remove all Holes"), FText(), FSlateIcon(), ClearAction);
			}
		}
		MenuBuilder.EndSection();

		return MenuBuilder.MakeWidget();
	}

	return nullptr;
}

void FLandscapeEditorCustomNodeBuilder_TargetLayers::OnExportLayer(const TSharedRef<FLandscapeTargetListInfo> Target)
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode)
	{
		check(!LandscapeEdMode->IsGridBased());
		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

		ULandscapeInfo* LandscapeInfo = Target->LandscapeInfo.Get();
		ULandscapeLayerInfoObject* LayerInfoObj = Target->LayerInfoObj.Get(); // nullptr for heightmaps

		// Prompt for filename
		FString SaveDialogTitle;
		FString DefaultFileName;
		const TCHAR* FileTypes = nullptr;

		ILandscapeEditorModule& LandscapeEditorModule = FModuleManager::GetModuleChecked<ILandscapeEditorModule>("LandscapeEditor");

		if (Target->TargetType == ELandscapeToolTargetType::Heightmap)
		{
			SaveDialogTitle = LOCTEXT("ExportHeightmap", "Export Landscape Heightmap").ToString();
			DefaultFileName = TEXT("Heightmap");
			FileTypes = LandscapeEditorModule.GetHeightmapExportDialogTypeString();
		}
		else //if (Target->TargetType == ELandscapeToolTargetType::Weightmap)
		{
			SaveDialogTitle = FText::Format(LOCTEXT("ExportLayer", "Export Landscape Layer: {0}"), FText::FromName(LayerInfoObj->LayerName)).ToString();
			DefaultFileName = LayerInfoObj->LayerName.ToString();
			FileTypes = LandscapeEditorModule.GetWeightmapExportDialogTypeString();
		}

		// Prompt the user for the filenames
		TArray<FString> SaveFilenames;
		bool bOpened = DesktopPlatform->SaveFileDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			*SaveDialogTitle,
			*LandscapeEdMode->UISettings->LastImportPath,
			*DefaultFileName,
			FileTypes,
			EFileDialogFlags::None,
			SaveFilenames
			);

		if (bOpened)
		{
			const FString& SaveFilename(SaveFilenames[0]);
			LandscapeEdMode->UISettings->LastImportPath = FPaths::GetPath(SaveFilename);

			// Actually do the export
			if (Target->TargetType == ELandscapeToolTargetType::Heightmap)
			{
				LandscapeInfo->ExportHeightmap(SaveFilename);
			}
			else //if (Target->TargetType == ELandscapeToolTargetType::Weightmap)
			{
				LandscapeInfo->ExportLayer(LayerInfoObj, SaveFilename);
			}

			Target->SetReimportFilePath(SaveFilename);
		}
	}
}

void FLandscapeEditorCustomNodeBuilder_TargetLayers::OnImportLayer(const TSharedRef<FLandscapeTargetListInfo> Target)
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode)
	{
		check(!LandscapeEdMode->IsGridBased());
		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

		ULandscapeInfo* LandscapeInfo = Target->LandscapeInfo.Get();
		ULandscapeLayerInfoObject* LayerInfoObj = Target->LayerInfoObj.Get(); // nullptr for heightmaps

		// Prompt for filename
		FString OpenDialogTitle;
		FString DefaultFileName;
		const TCHAR* FileTypes = nullptr;

		ILandscapeEditorModule& LandscapeEditorModule = FModuleManager::GetModuleChecked<ILandscapeEditorModule>("LandscapeEditor");

		if (Target->TargetType == ELandscapeToolTargetType::Heightmap)
		{
			OpenDialogTitle = *LOCTEXT("ImportHeightmap", "Import Landscape Heightmap").ToString();
			DefaultFileName = TEXT("Heightmap.png");
			FileTypes = LandscapeEditorModule.GetHeightmapImportDialogTypeString();
		}
		else //if (Target->TargetType == ELandscapeToolTargetType::Weightmap)
		{
			OpenDialogTitle = *FText::Format(LOCTEXT("ImportLayer", "Import Landscape Layer: {0}"), FText::FromName(LayerInfoObj->LayerName)).ToString();
			DefaultFileName = *FString::Printf(TEXT("%s.png"), *(LayerInfoObj->LayerName.ToString()));
			FileTypes = LandscapeEditorModule.GetWeightmapImportDialogTypeString();
		}

		// Prompt the user for the filenames
		TArray<FString> OpenFilenames;
		bool bOpened = DesktopPlatform->OpenFileDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			*OpenDialogTitle,
			*LandscapeEdMode->UISettings->LastImportPath,
			*DefaultFileName,
			FileTypes,
			EFileDialogFlags::None,
			OpenFilenames
			);

		if (bOpened)
		{
			const FString& OpenFilename(OpenFilenames[0]);
			LandscapeEdMode->UISettings->LastImportPath = FPaths::GetPath(OpenFilename);

			// Actually do the Import
			LandscapeEdMode->ImportData(*Target, OpenFilename);

			Target->SetReimportFilePath(OpenFilename);
		}
	}
}

void FLandscapeEditorCustomNodeBuilder_TargetLayers::OnReimportLayer(const TSharedRef<FLandscapeTargetListInfo> Target)
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode)
	{
		check(!LandscapeEdMode->IsGridBased());
		LandscapeEdMode->ReimportData(*Target);
	}
}

void FLandscapeEditorCustomNodeBuilder_TargetLayers::OnHeightmapLayerContextMenu(const TSharedRef<FLandscapeTargetListInfo> Target)
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode)
	{
		LandscapeEdMode->SetCurrentTool("ImportExport");
	}
}

void FLandscapeEditorCustomNodeBuilder_TargetLayers::OnFillLayer(const TSharedRef<FLandscapeTargetListInfo> Target)
{
	FScopedTransaction Transaction(LOCTEXT("Undo_FillLayer", "Filling Landscape Layer"));
	if (Target->LandscapeInfo.IsValid() && Target->LayerInfoObj.IsValid())
	{
		FLandscapeEditDataInterface LandscapeEdit(Target->LandscapeInfo.Get());

		FEdModeLandscape* LandscapeEdMode = GetEditorMode();
		if (LandscapeEdMode)
		{
			FScopedSetLandscapeEditingLayer Scope(LandscapeEdMode->GetLandscape(), LandscapeEdMode->GetCurrentLayerGuid(), [&] { LandscapeEdMode->RequestLayersContentUpdateForceAll(); });
			LandscapeEdit.FillLayer(Target->LayerInfoObj.Get());
		}
	}
}

void FLandscapeEditorCustomNodeBuilder_TargetLayers::FillEmptyLayers(ULandscapeInfo* LandscapeInfo, ULandscapeLayerInfoObject* LandscapeInfoObject)
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode)
	{
		FLandscapeEditDataInterface LandscapeEdit(LandscapeInfo);

		if (LandscapeEdMode->CanHaveLandscapeLayersContent())
		{
			if (LandscapeEdMode->NeedToFillEmptyMaterialLayers())
			{
				FScopedSetLandscapeEditingLayer Scope(LandscapeEdMode->GetLandscape(), LandscapeEdMode->GetCurrentLayerGuid());
				LandscapeEdit.FillEmptyLayers(LandscapeInfoObject);
			}
		}
		else
		{
			LandscapeEdit.FillEmptyLayers(LandscapeInfoObject);
		}
	}

}


void FLandscapeEditorCustomNodeBuilder_TargetLayers::OnClearLayer(const TSharedRef<FLandscapeTargetListInfo> Target)
{
	FScopedTransaction Transaction(LOCTEXT("Undo_ClearLayer", "Clearing Landscape Layer"));
	if (Target->LandscapeInfo.IsValid() && Target->LayerInfoObj.IsValid())
	{
		FEdModeLandscape* LandscapeEdMode = GetEditorMode();
		if (LandscapeEdMode)
		{
			FScopedSetLandscapeEditingLayer Scope(LandscapeEdMode->GetLandscape(), LandscapeEdMode->GetCurrentLayerGuid(), [&] { LandscapeEdMode->RequestLayersContentUpdateForceAll(); });
			FLandscapeEditDataInterface LandscapeEdit(Target->LandscapeInfo.Get());
			LandscapeEdit.DeleteLayer(Target->LayerInfoObj.Get());
			LandscapeEdMode->RequestUpdateLayerUsageInformation();
		}
	}
}

void FLandscapeEditorCustomNodeBuilder_TargetLayers::OnRebuildMICs(const TSharedRef<FLandscapeTargetListInfo> Target)
{
	if (Target->LandscapeInfo.IsValid())
	{
		Target->LandscapeInfo.Get()->UpdateAllComponentMaterialInstances(/*bInvalidateCombinationMaterials = */true);
	}
}


bool FLandscapeEditorCustomNodeBuilder_TargetLayers::ShouldFilterLayerInfo(const FAssetData& AssetData, FName LayerName)
{
	const FName LayerNameMetaData = AssetData.GetTagValueRef<FName>("LayerName");
	if (!LayerNameMetaData.IsNone())
	{
		return LayerNameMetaData != LayerName;
	}

	ULandscapeLayerInfoObject* LayerInfo = CastChecked<ULandscapeLayerInfoObject>(AssetData.GetAsset());
	return LayerInfo->LayerName != LayerName;
}

void FLandscapeEditorCustomNodeBuilder_TargetLayers::OnTargetLayerSetObject(const FAssetData& AssetData, const TSharedRef<FLandscapeTargetListInfo> Target)
{
	// Can't assign null to a layer
	UObject* Object = AssetData.GetAsset();
	if (Object == nullptr)
	{
		return;
	}

	FScopedTransaction Transaction(LOCTEXT("Undo_UseExisting", "Assigning Layer to Landscape"));

	ULandscapeLayerInfoObject* SelectedLayerInfo = const_cast<ULandscapeLayerInfoObject*>(CastChecked<ULandscapeLayerInfoObject>(Object));

	if (SelectedLayerInfo != Target->LayerInfoObj.Get())
	{
		if (ensure(SelectedLayerInfo->LayerName == Target->GetLayerName()))
		{
			ULandscapeInfo* LandscapeInfo = Target->LandscapeInfo.Get();
			ALandscape* LandscapeActor = LandscapeInfo->LandscapeActor.Get();
			
			if (!LandscapeActor->HasTargetLayer(Target->GetLayerName()))
			{
				LandscapeActor->AddTargetLayer(Target->GetLayerName(), FLandscapeTargetLayerSettings(SelectedLayerInfo));
			}
			
			if (Target->LayerInfoObj.IsValid())
			{
				int32 Index = LandscapeInfo->GetLayerInfoIndex(Target->LayerInfoObj.Get(), Target->Owner.Get());
				if (ensure(Index != INDEX_NONE))
				{
					FLandscapeInfoLayerSettings& LayerSettings = LandscapeInfo->Layers[Index];
					LandscapeInfo->ReplaceLayer(LayerSettings.LayerInfoObj, SelectedLayerInfo);
					// Important : don't use LayerSettings after the call to ReplaceLayer as it will have been reallocated. 
					//  Validate that the replacement happened as expected : 
					check(LandscapeInfo->GetLayerInfoIndex(SelectedLayerInfo, Target->Owner.Get()) != INDEX_NONE);
				}
			}
			else
			{
				int32 Index = LandscapeInfo->GetLayerInfoIndex(Target->LayerName, Target->Owner.Get());
				if (ensure(Index != INDEX_NONE))
				{
					FLandscapeInfoLayerSettings& LayerSettings = LandscapeInfo->Layers[Index];
					LayerSettings.LayerInfoObj = SelectedLayerInfo;

					Target->LandscapeInfo->CreateTargetLayerSettingsFor(SelectedLayerInfo);
				}
			}
						
			FEdModeLandscape* LandscapeEdMode = GetEditorMode();
			if (LandscapeEdMode)
			{
				LandscapeEdMode->CurrentToolTarget.TargetType = Target->TargetType;
				LandscapeEdMode->SetCurrentTargetLayer(Target->LayerName, SelectedLayerInfo);
				LandscapeEdMode->UpdateTargetList();
			}

			FillEmptyLayers(LandscapeInfo, SelectedLayerInfo);
		}
		else
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Error_LayerNameMismatch", "Can't use this layer info because the layer name does not match"));
		}
	}
}

EVisibility FLandscapeEditorCustomNodeBuilder_TargetLayers::GetTargetLayerInfoSelectorVisibility(const TSharedRef<FLandscapeTargetListInfo> Target)
{
	if (Target->TargetType == ELandscapeToolTargetType::Weightmap)
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}

bool FLandscapeEditorCustomNodeBuilder_TargetLayers::GetTargetLayerCreateEnabled(const TSharedRef<FLandscapeTargetListInfo> Target)
{
	if (!Target->LayerInfoObj.IsValid())
	{
		return true;
	}

	return false;
}

EVisibility FLandscapeEditorCustomNodeBuilder_TargetLayers::GetTargetLayerMakePublicVisibility(const TSharedRef<FLandscapeTargetListInfo> Target)
{
	if (Target->bValid && Target->LayerInfoObj.IsValid() && Target->LayerInfoObj->GetOutermost()->ContainsMap())
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}

EVisibility FLandscapeEditorCustomNodeBuilder_TargetLayers::GetTargetLayerDeleteVisibility(const TSharedRef<FLandscapeTargetListInfo> Target)
{
	if (!Target->bValid)
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}

TSharedRef<SWidget> FLandscapeEditorCustomNodeBuilder_TargetLayers::OnGetTargetLayerCreateMenu(const TSharedRef<FLandscapeTargetListInfo> Target)
{
	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.AddMenuEntry(LOCTEXT("Menu_Create_Blended", "Weight-Blended Layer (normal)"), FText(), FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic(&FLandscapeEditorCustomNodeBuilder_TargetLayers::OnTargetLayerCreateClicked, Target, false)));

	MenuBuilder.AddMenuEntry(LOCTEXT("Menu_Create_NoWeightBlend", "Non Weight-Blended Layer"), FText(), FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic(&FLandscapeEditorCustomNodeBuilder_TargetLayers::OnTargetLayerCreateClicked, Target, true)));

	return MenuBuilder.MakeWidget();
}

void FLandscapeEditorCustomNodeBuilder_TargetLayers::OnTargetLayerCreateClicked(const TSharedRef<FLandscapeTargetListInfo> Target, bool bNoWeightBlend)
{
	check(!Target->LayerInfoObj.IsValid());

	FScopedTransaction Transaction(LOCTEXT("Undo_Create", "Creating New Landscape Layer"));

	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode)
	{
		FName LayerName = Target->GetLayerName();
		ULevel* Level = Target->Owner->GetLevel();

		// Build default layer object name and package name
		FName LayerObjectName;
		FString PackageName = UE::Landscape::GetLayerInfoObjectPackageName(Level, LayerName, LayerObjectName);

		TSharedRef<SDlgPickAssetPath> NewLayerDlg =
			SNew(SDlgPickAssetPath)
			.Title(LOCTEXT("CreateNewLayerInfo", "Create New Landscape Layer Info Object"))
			.DefaultAssetPath(FText::FromString(PackageName));

		if (NewLayerDlg->ShowModal() != EAppReturnType::Cancel)
		{
			PackageName = NewLayerDlg->GetFullAssetPath().ToString();
			LayerObjectName = FName(*NewLayerDlg->GetAssetName().ToString());

			UPackage* Package = CreatePackage( *PackageName);

			// Do not pass RF_Transactional to NewObject, or the asset will mark itself as garbage on Undo (which is not a well-supported path, potentially causing crashes)
			ULandscapeLayerInfoObject* LayerInfo = NewObject<ULandscapeLayerInfoObject>(Package, LayerObjectName, RF_Public | RF_Standalone);
			LayerInfo->SetFlags(RF_Transactional);	// we add RF_Transactional after creation, so that future edits _are_ recorded in undo
			LayerInfo->LayerName = LayerName;
			LayerInfo->bNoWeightBlend = bNoWeightBlend;

			ULandscapeInfo* LandscapeInfo = Target->LandscapeInfo.Get();
			LandscapeInfo->Modify();
			int32 Index = LandscapeInfo->GetLayerInfoIndex(LayerName, Target->Owner.Get());
			if (Index == INDEX_NONE)
			{
				LandscapeInfo->Layers.Add(FLandscapeInfoLayerSettings(LayerInfo, Target->Owner.Get()));
			}
			else
			{
				LandscapeInfo->Layers[Index].LayerInfoObj = LayerInfo;
			}

			if (LandscapeEdMode->CurrentToolTarget.LayerName == Target->LayerName
				&& LandscapeEdMode->CurrentToolTarget.LayerInfo == Target->LayerInfoObj)
			{
				LandscapeEdMode->SetCurrentTargetLayer(Target->LayerName, Target->LayerInfoObj);
			}

			Target->LayerInfoObj = LayerInfo;
			Target->LandscapeInfo->CreateTargetLayerSettingsFor(LayerInfo);

			// Notify the asset registry
			FAssetRegistryModule::AssetCreated(LayerInfo);

			// Mark the package dirty...
			Package->MarkPackageDirty();

			// Show in the content browser
			TArray<UObject*> Objects;
			Objects.Add(LayerInfo);
			GEditor->SyncBrowserToObjects(Objects);

			ALandscape* LandscapeActor = Target->LandscapeInfo->LandscapeActor.Get();
			LandscapeActor->UpdateTargetLayer(FName(LayerName), FLandscapeTargetLayerSettings(LayerInfo));

			FillEmptyLayers(LandscapeInfo, LayerInfo);

			if (FEdModeLandscape* Mode = (FEdModeLandscape*)GLevelEditorModeTools().GetActiveMode(FBuiltinEditorModes::EM_Landscape))
			{
				Mode->UpdateTargetList();
			}
		}
	}
}

FReply FLandscapeEditorCustomNodeBuilder_TargetLayers::OnTargetLayerMakePublicClicked(const TSharedRef<FLandscapeTargetListInfo> Target)
{
	FScopedTransaction Transaction(LOCTEXT("Undo_MakePublic", "Make Layer Public"));
	TArray<UObject*> Objects;
	Objects.Add(Target->LayerInfoObj.Get());

	FString Path = Target->Owner->GetOutermost()->GetName() + TEXT("_sharedassets");
	bool bSucceed = ObjectTools::RenameObjects(Objects, false, TEXT(""), Path);
	if (bSucceed)
	{
		FEdModeLandscape* Mode = (FEdModeLandscape*)GLevelEditorModeTools().GetActiveMode(FBuiltinEditorModes::EM_Landscape);
		if (Mode)
		{
			Mode->UpdateTargetList();
		}
	}
	else
	{
		Transaction.Cancel();
	}

	return FReply::Handled();
}

FReply FLandscapeEditorCustomNodeBuilder_TargetLayers::OnTargetLayerDeleteClicked(const TSharedRef<FLandscapeTargetListInfo> Target)
{
	check(Target->LandscapeInfo.IsValid());

	if (FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("Prompt_DeleteLayer", "Are you sure you want to delete this layer?")) == EAppReturnType::Yes)
	{
		FScopedTransaction Transaction(LOCTEXT("Undo_Delete", "Delete Layer"));

		FEdModeLandscape* LandscapeEdMode = GetEditorMode();
		FScopedSetLandscapeEditingLayer Scope(LandscapeEdMode ? LandscapeEdMode->GetLandscape() : nullptr, LandscapeEdMode ? LandscapeEdMode->GetCurrentLayerGuid() : FGuid());

		Target->LandscapeInfo->DeleteLayer(Target->LayerInfoObj.Get(), Target->LayerName);

		if (LandscapeEdMode)
		{
			LandscapeEdMode->UpdateTargetList();
		}
	}

	return FReply::Handled();
}

FSlateColor FLandscapeEditorCustomNodeBuilder_TargetLayers::GetLayerUsageDebugColor(const TSharedRef<FLandscapeTargetListInfo> Target)
{
	if (GLandscapeViewMode == ELandscapeViewMode::LayerUsage && Target->TargetType != ELandscapeToolTargetType::Heightmap && ensure(Target->LayerInfoObj.IsValid()))
	{
		return FSlateColor(Target->LayerInfoObj->LayerUsageDebugColor);
	}
	return FSlateColor(FLinearColor(0, 0, 0, 0));
}

EVisibility FLandscapeEditorCustomNodeBuilder_TargetLayers::GetDebugModeLayerUsageVisibility(const TSharedRef<FLandscapeTargetListInfo> Target)
{
	if (GLandscapeViewMode == ELandscapeViewMode::LayerUsage && Target->TargetType != ELandscapeToolTargetType::Heightmap && Target->LayerInfoObj.IsValid())
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}

EVisibility FLandscapeEditorCustomNodeBuilder_TargetLayers::GetDebugModeLayerUsageVisibility_Invert(const TSharedRef<FLandscapeTargetListInfo> Target)
{
	if (GLandscapeViewMode == ELandscapeViewMode::LayerUsage && Target->TargetType != ELandscapeToolTargetType::Heightmap && Target->LayerInfoObj.IsValid())
	{
		return EVisibility::Collapsed;
	}

	return EVisibility::Visible;
}

EVisibility FLandscapeEditorCustomNodeBuilder_TargetLayers::GetLayersSubstractiveBlendVisibility(const TSharedRef<FLandscapeTargetListInfo> Target)
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode != nullptr && LandscapeEdMode->CanHaveLandscapeLayersContent() && Target->TargetType != ELandscapeToolTargetType::Heightmap && Target->LayerInfoObj.IsValid())
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}

ECheckBoxState FLandscapeEditorCustomNodeBuilder_TargetLayers::IsLayersSubstractiveBlendChecked(const TSharedRef<FLandscapeTargetListInfo> Target)
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode)
	{
		return LandscapeEdMode->IsCurrentLayerBlendSubstractive(Target.Get().LayerInfoObj) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}

	return ECheckBoxState::Unchecked;
}

void FLandscapeEditorCustomNodeBuilder_TargetLayers::OnLayersSubstractiveBlendChanged(ECheckBoxState NewCheckedState, const TSharedRef<FLandscapeTargetListInfo> Target)
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode)
	{
		FScopedTransaction Transaction(LOCTEXT("Undo_SubtractiveBlend", "Set Subtractive Blend Layer"));
		LandscapeEdMode->SetCurrentLayerSubstractiveBlendStatus(NewCheckedState == ECheckBoxState::Checked, Target.Get().LayerInfoObj);
	}
}

EVisibility FLandscapeEditorCustomNodeBuilder_TargetLayers::GetDebugModeColorChannelVisibility(const TSharedRef<FLandscapeTargetListInfo> Target)
{
	if (GLandscapeViewMode == ELandscapeViewMode::DebugLayer && Target->TargetType != ELandscapeToolTargetType::Heightmap && Target->LayerInfoObj.IsValid())
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}

ECheckBoxState FLandscapeEditorCustomNodeBuilder_TargetLayers::DebugModeColorChannelIsChecked(const TSharedRef<FLandscapeTargetListInfo> Target, int32 Channel)
{
	if (Target->DebugColorChannel == Channel)
	{
		return ECheckBoxState::Checked;
	}

	return ECheckBoxState::Unchecked;
}

void FLandscapeEditorCustomNodeBuilder_TargetLayers::OnDebugModeColorChannelChanged(ECheckBoxState NewCheckedState, const TSharedRef<FLandscapeTargetListInfo> Target, int32 Channel)
{
	if (NewCheckedState == ECheckBoxState::Checked)
	{
		// Enable on us and disable colour channel on other targets
		if (ensure(Target->LayerInfoObj != nullptr))
		{
			ULandscapeInfo* LandscapeInfo = Target->LandscapeInfo.Get();
			int32 Index = LandscapeInfo->GetLayerInfoIndex(Target->LayerInfoObj.Get(), Target->Owner.Get());
			if (ensure(Index != INDEX_NONE))
			{
				for (auto It = LandscapeInfo->Layers.CreateIterator(); It; It++)
				{
					FLandscapeInfoLayerSettings& LayerSettings = *It;
					if (It.GetIndex() == Index)
					{
						LayerSettings.DebugColorChannel = Channel;
					}
					else
					{
						LayerSettings.DebugColorChannel &= ~Channel;
					}
				}
				LandscapeInfo->UpdateDebugColorMaterial();

				if (FEdModeLandscape* LandscapeEdMode = GetEditorMode())
				{
					LandscapeEdMode->UpdateTargetList();
				}
			}
		}
	}
}

FSlateColor FLandscapeEditorCustomNodeBuilder_TargetLayers::GetTargetTextColor(const TSharedRef<FLandscapeTargetListInfo> InTarget)
{
	return FLandscapeEditorCustomNodeBuilder_TargetLayers::GetTargetLayerIsSelected(InTarget) ? FStyleColors::ForegroundHover : FSlateColor::UseForeground();
}

//////////////////////////////////////////////////////////////////////////

void SLandscapeEditorSelectableBorder::Construct(const FArguments& InArgs)
{
	SBorder::Construct(
		SBorder::FArguments()
		.HAlign(InArgs._HAlign)
		.VAlign(InArgs._VAlign)
		.Padding(InArgs._Padding)
		.BorderImage(this, &SLandscapeEditorSelectableBorder::GetBorder)
		.Content()
		[
			InArgs._Content.Widget
		]
	);

	OnContextMenuOpening = InArgs._OnContextMenuOpening;
	OnSelected = InArgs._OnSelected;
	IsSelected = InArgs._IsSelected;
}

FReply SLandscapeEditorSelectableBorder::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MyGeometry.IsUnderLocation(MouseEvent.GetScreenSpacePosition()))
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton &&
			OnSelected.IsBound())
		{
			OnSelected.Execute();

			return FReply::Handled().ReleaseMouseCapture();
		}
		else if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton &&
			OnContextMenuOpening.IsBound())
			{
				TSharedPtr<SWidget> Content = OnContextMenuOpening.Execute();
				if (Content.IsValid())
				{
					FWidgetPath WidgetPath = MouseEvent.GetEventPath() != nullptr ? *MouseEvent.GetEventPath() : FWidgetPath();

					FSlateApplication::Get().PushMenu(SharedThis(this), WidgetPath, Content.ToSharedRef(), MouseEvent.GetScreenSpacePosition(), FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
				}
			
			return FReply::Handled().ReleaseMouseCapture();
		}
	}

	return FReply::Unhandled();
}

const FSlateBrush* SLandscapeEditorSelectableBorder::GetBorder() const
{
	const bool bIsSelected = IsSelected.Get();
	const bool bHovered = IsHovered() && OnSelected.IsBound();

	if (bIsSelected)
	{
		return bHovered
			? FAppStyle::GetBrush("LandscapeEditor.TargetList", ".RowSelectedHovered")
			: FAppStyle::GetBrush("LandscapeEditor.TargetList", ".RowSelected");
	}
	else
	{
		return bHovered
			? FAppStyle::GetBrush("LandscapeEditor.TargetList", ".RowBackgroundHovered")
			: FAppStyle::GetBrush("LandscapeEditor.TargetList", ".RowBackground");
	}
}

TSharedRef<FTargetLayerDragDropOp> FTargetLayerDragDropOp::New(int32 InSlotIndexBeingDragged, SVerticalBox::FSlot* InSlotBeingDragged, TSharedPtr<SWidget> WidgetToShow)
{
	TSharedRef<FTargetLayerDragDropOp> Operation = MakeShareable(new FTargetLayerDragDropOp);

	Operation->MouseCursor = EMouseCursor::GrabHandClosed;
	Operation->SlotIndexBeingDragged = InSlotIndexBeingDragged;
	Operation->SlotBeingDragged = InSlotBeingDragged;
	Operation->WidgetToShow = WidgetToShow;

	Operation->Construct();

	return Operation;
}

FTargetLayerDragDropOp::~FTargetLayerDragDropOp()
{
}

TSharedPtr<SWidget> FTargetLayerDragDropOp::GetDefaultDecorator() const
{
	return SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ContentBrowser.AssetDragDropTooltipBackground"))
			.Content()
			[
				WidgetToShow.ToSharedRef()
			];
		
}

#undef LOCTEXT_NAMESPACE
