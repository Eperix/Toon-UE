// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Widgets/SCompoundWidget.h"
#include "Widgets/SWidget.h"
#include "Widgets/Views/STreeView.h"

#include "InsightsCore/Filter/ViewModels/FilterConfiguratorNode.h"
#include "InsightsCore/Filter/ViewModels/Filters.h"

class SDockTab;

namespace UE::Insights
{

class FFilterConfigurator;
class SFilterConfigurator;

////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * A custom widget used to configure custom filters.
 */
class TRACEINSIGHTSCORE_API SAdvancedFilter: public SCompoundWidget
{
public:
	/** Default constructor. */
	SAdvancedFilter();

	/** Virtual destructor. */
	virtual ~SAdvancedFilter();

	SLATE_BEGIN_ARGS(SAdvancedFilter) {}
	SLATE_END_ARGS()

	/**
	 * Construct this widget
	 * @param InArgs - The declaration data for this widget
	 */
	void Construct(const FArguments& InArgs, TSharedPtr<FFilterConfigurator> InFilterConfiguratorViewModel);

	void Reset();

	void SetParentTab(const TSharedPtr<SDockTab> InTab) { ParentTab = InTab; }
	const TWeakPtr<SDockTab> GetParentTab() { return ParentTab; };

	void RequestClose();

private:
	void InitCommandList();

	FReply OK_OnClicked();

	FReply Cancel_OnClicked();

private:
	TSharedPtr<SFilterConfigurator> FilterConfigurator;

	TWeakPtr<FFilterConfigurator> OriginalFilterConfiguratorViewModel;

	TSharedPtr<FFilterConfigurator> FilterConfiguratorViewModel;

	TWeakPtr<SDockTab> ParentTab;

	FDelegateHandle OnViewModelDestroyedHandle;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace UE::Insights
