// Copyright 2018-2019 Mickael Daniel. All Rights Reserved.

#include "TargetSystemLockOnWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"


UTargetSystemLockOnWidget::UTargetSystemLockOnWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UTargetSystemLockOnWidget::RebuildWidget()
{

	UPanelWidget* RootWidget = Cast<UPanelWidget>(GetRootWidget());
	if (!RootWidget)
	{
		// Construct the root widget. Root widgets are UCanvasPanels by default in UMG
		RootWidget = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootWidget"));

		// Set it as the root widget
		WidgetTree->RootWidget = RootWidget;
	}

	// The root widget needs to be set before claling Super::RebuildWidget()
	TSharedRef<SWidget> Widget = Super::RebuildWidget();
	if (WidgetTree && RootWidget)
	{
		TextWidget = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TextBox"));
		TextWidget->Font.Size = 24;
		TextWidget->SetText(FText::FromString("O"));
		RootWidget->AddChild(TextWidget);
	}

	return Widget;
}
