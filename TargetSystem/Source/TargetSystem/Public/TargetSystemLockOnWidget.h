// Copyright 2018-2019 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "TargetSystemLockOnWidget.generated.h"

/**
 * 
 */
UCLASS()
class TARGETSYSTEM_API UTargetSystemLockOnWidget : public UUserWidget
{
	GENERATED_BODY()

private:
	UTextBlock* TextWidget;

public:
	UTargetSystemLockOnWidget(const FObjectInitializer& ObjectInitializer);

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
};
