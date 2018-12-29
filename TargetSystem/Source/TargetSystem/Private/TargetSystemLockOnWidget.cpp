// Fill out your copyright notice in the Description page of Project Settings.

#include "TargetSystemLockOnWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"


UTargetSystemLockOnWidget::UTargetSystemLockOnWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	UE_LOG(LogTemp, Warning, TEXT("UTargetLockOnWidget initialization"));
}

TSharedRef<SWidget> UTargetSystemLockOnWidget::RebuildWidget()
{

	UE_LOG(LogTemp, Warning, TEXT("UTargetLockOnWidget RebuildWidget()"));

	UPanelWidget* RootWidget = Cast<UPanelWidget>(GetRootWidget());
	if (!RootWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("UTargetLockOnWidget RebuildWidget() No root widget create it"));
		// Construct the root widget. Root widgets are UCanvasPanels by default in UMG
		RootWidget = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootWidget"));

		// Set it as the root widget
		WidgetTree->RootWidget = RootWidget;
	}

	// The root widget needs to be set before claling Super::RebuildWidget()
	TSharedRef<SWidget> Widget = Super::RebuildWidget();
	if (WidgetTree && RootWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("WidgetTree && RootWidget"));

		TextWidget = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TextBox"));
		TextWidget->Font.Size = 24;
		TextWidget->SetText(FText::FromString("O"));
		RootWidget->AddChild(TextWidget);
	}

	return Widget;
}
