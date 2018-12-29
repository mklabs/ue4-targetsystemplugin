// Fill out your copyright notice in the Description page of Project Settings.

#include "TargetSystemComponent.h"
#include "TargetSystemLockOnWidget.h"
#include "TargetSystemTargetableInterface.h"
#include "Engine/World.h"
#include "Engine/Public/TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"

// Sets default values for this component's properties
UTargetSystemComponent::UTargetSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	MinimumDistanceToEnable = 1200.0f;
	ClosestTargetDistance = 0.0f;
	TargetLocked = false;
	TargerLockedOnWidgetDrawSize = 32.0f;
	TargerLockedOnWidgetRelativeLocation = FVector(0.0f, 0.0f, 20.0f);
	BreakLineOfSightDelay = 2.0f;
	bIsBreakingLineOfSight = false;
	ShouldControlRotationWhenLockedOn = true;
	StartRotatingThreshold = 1.5f;

	TargetableActors = APawn::StaticClass();
}

// Called when the game starts
void UTargetSystemComponent::BeginPlay()
{
	Super::BeginPlay();
	CharacterReference = GetOwner();
	if (!CharacterReference)
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] TargetSystemComponent: Cannot get Owner reference ..."), *this->GetName());
	}

	CharacterController = CharacterReference->GetInstigatorController();
	if (!CharacterController)
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] TargetSystemComponent: Cannot get Controller reference ..."), *CharacterReference->GetName());
	}

	PlayerController = Cast<APlayerController>(CharacterController);
}

void UTargetSystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (TargetLocked && NearestTarget)
	{
		SetControlRotationOnTarget(NearestTarget);

		// Target Locked Off based on Distance
		if (GetDistanceFromCharacter(NearestTarget) > MinimumDistanceToEnable)
		{
			UE_LOG(LogTemp, Warning, TEXT("Target lock off"));
			TargetLockOff();
		}

		if (ShouldBreakLineOfSight() && !bIsBreakingLineOfSight)
		{
			UE_LOG(LogTemp, Warning, TEXT("Should break line of sight"));

			if (BreakLineOfSightDelay <= 0)
			{
				TargetLockOff();
			} else
			{
				bIsBreakingLineOfSight = true;
				GetWorld()->GetTimerManager().SetTimer(
					LineOfSightBreakTimerHandle,
					this,
					&UTargetSystemComponent::BreakLineOfSight,
					BreakLineOfSightDelay
				);
			}
		}
	}
}

void UTargetSystemComponent::TargetActor()
{
	UE_LOG(LogTemp, Warning, TEXT("Target Actor ..."));

	ClosestTargetDistance = MinimumDistanceToEnable;

	if (TargetLocked)
	{
		UE_LOG(LogTemp, Warning, TEXT("Target is locked already ..."));
		TargetLockOff();
	} else
	{
		TargetLockOn();
	}
}

void UTargetSystemComponent::TargetActorWithAxisInput(float AxisValue)
{
	if (FMath::Abs(AxisValue) >= StartRotatingThreshold)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Target Actor with Axis Input: %f..."), *CharacterReference->GetName(), AxisValue);
	}
}

void UTargetSystemComponent::TargetLockOn()
{
	TArray<AActor*> Actors = GetAllActorsOfClass(TargetableActors);
	NearestTarget = FindNearestTarget(Actors);
	if (NearestTarget)
	{
		TargetLocked = true;
		CreateAndAttachTargetLockedOnWidgetComponent(NearestTarget);

		if (ShouldControlRotationWhenLockedOn)
		{
			ControlRotation(true);
		}

		OnTargetLockedOn.Broadcast(NearestTarget);
	}
}

void UTargetSystemComponent::TargetLockOff()
{
	TargetLocked = false;
	if (TargetLockedOnWidgetComponent)
	{
		TargetLockedOnWidgetComponent->DestroyComponent();
	}

	if (NearestTarget)
	{
		if (ShouldControlRotationWhenLockedOn)
		{
			ControlRotation(false);
		}

		OnTargetLockedOff.Broadcast(NearestTarget);
	}

	NearestTarget = nullptr;
}

void UTargetSystemComponent::CreateAndAttachTargetLockedOnWidgetComponent(AActor* TargetActor)
{
	TargetLockedOnWidgetComponent = NewObject<UWidgetComponent>(TargetActor, FName("TargetLockOn"));
	if (TargetLockedOnWidgetClass)
	{
		TargetLockedOnWidgetComponent->SetWidgetClass(TargetLockedOnWidgetClass);
	} else
	{
		TargetLockedOnWidgetComponent->SetWidgetClass(UTargetSystemLockOnWidget::StaticClass());
	}

	TargetLockedOnWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	TargetLockedOnWidgetComponent->SetupAttachment(TargetActor->GetRootComponent());
	TargetLockedOnWidgetComponent->SetRelativeLocation(TargerLockedOnWidgetRelativeLocation);
	TargetLockedOnWidgetComponent->SetDrawSize(FVector2D(TargerLockedOnWidgetDrawSize, TargerLockedOnWidgetDrawSize));
	TargetLockedOnWidgetComponent->SetVisibility(true);
	TargetLockedOnWidgetComponent->RegisterComponent();
}

TArray<AActor*> UTargetSystemComponent::GetAllActorsOfClass(TSubclassOf<AActor> ActorClass)
{
	TArray<AActor*> Actors;
	for (TActorIterator<AActor> ActorIterator(GetWorld(), ActorClass); ActorIterator; ++ActorIterator)
	{
		AActor* Actor = *ActorIterator;
		bool bIsImplemented = Actor->GetClass()->ImplementsInterface(UTargetSystemTargetableInterface::StaticClass());
		if (bIsImplemented)
		{
			bool bIsTargetable = ITargetSystemTargetableInterface::Execute_IsTargetable(Actor);
			if (bIsTargetable)
			{
				Actors.Add(Actor);
			}
		} else
		{
			Actors.Add(Actor);
		}
	}

	return Actors;
}

AActor* UTargetSystemComponent::FindNearestTarget(TArray<AActor*> Actors)
{
	TArray<AActor*> ActorsHit;

	// Find all actors we can line trace to
	for (AActor* Actor : Actors)
	{
		bool bHit = LineTraceForActor(Actor);
		if (bHit && IsInViewport(Actor))
		{
			ActorsHit.Add(Actor);
		}
	}

	// From the hit actors, check distance and return the nearest
	if (ActorsHit.Num() == 0)
	{
		return nullptr;
	}

	float ClosestDistance = ClosestTargetDistance;
	AActor* Target = nullptr;
	for (AActor* Actor : ActorsHit)
	{
		float Distance = GetDistanceFromCharacter(Actor);
		if (Distance < ClosestDistance)
		{
			ClosestDistance = Distance;
			Target = Actor;
		}
	}

	return Target;
}


bool UTargetSystemComponent::LineTraceForActor(AActor* OtherActor, TArray<AActor*> ActorsToIgnore)
{
	FHitResult HitResult;
	bool bHit = LineTrace(HitResult, OtherActor, ActorsToIgnore);
	if (bHit)
	{
		AActor* HitActor = HitResult.GetActor();
		if (HitActor == OtherActor)
		{
			return true;
		}
	}

	return false;
}

bool UTargetSystemComponent::LineTrace(FHitResult& HitResult, AActor* OtherActor, TArray<AActor*> ActorsToIgnore)
{
	FCollisionQueryParams Params = FCollisionQueryParams(FName("LineTraceSingle"));

	TArray<AActor*> IgnoredActors;
	IgnoredActors.Init(CharacterReference, 1);
	IgnoredActors += ActorsToIgnore;

	Params.AddIgnoredActors(IgnoredActors);

	return GetWorld()->LineTraceSingleByChannel(
		HitResult,
		CharacterReference->GetActorLocation(),
		OtherActor->GetActorLocation(),
		ECollisionChannel::ECC_Camera,
		Params
	);
}

FRotator UTargetSystemComponent::GetControlRotationOnTarget(AActor* OtherActor)
{
	FRotator ControlRotation = CharacterController->GetControlRotation();

	FVector CharacterLocation = CharacterReference->GetActorLocation();
	FVector OtherActorLocation = OtherActor->GetActorLocation();

	// Find look at rotation
	FRotator LookRotation = FRotationMatrix::MakeFromX(OtherActorLocation - CharacterLocation).Rotator();

	FRotator TargetRotation = FRotator(ControlRotation.Pitch, LookRotation.Yaw, ControlRotation.Roll);

	return FMath::RInterpTo(ControlRotation, TargetRotation, GetWorld()->GetDeltaSeconds(), 5.0f);
}

void UTargetSystemComponent::SetControlRotationOnTarget(AActor* TargetActor)
{
	if (!CharacterController)
	{
		return;
	}

	CharacterController->SetControlRotation(GetControlRotationOnTarget(TargetActor));
}

float UTargetSystemComponent::GetDistanceFromCharacter(AActor* OtherActor)
{
	return CharacterReference->GetDistanceTo(OtherActor);
}


bool UTargetSystemComponent::ShouldBreakLineOfSight()
{
	if (!NearestTarget)
	{
		return true;
	}

	TArray<AActor*> ActorsToIgnore = GetAllActorsOfClass(TargetableActors);
	ActorsToIgnore.Remove(NearestTarget);

	FHitResult HitResult;
	bool bHit = LineTrace(HitResult, NearestTarget, ActorsToIgnore);
	if (bHit && HitResult.GetActor() != NearestTarget)
	{
		return true;
	}

	return false;
}

void UTargetSystemComponent::BreakLineOfSight()
{
	bIsBreakingLineOfSight = false;
	if (ShouldBreakLineOfSight())
	{
		UE_LOG(LogTemp, Warning, TEXT("Break line of Sight"));
		TargetLockOff();
	}
}

void UTargetSystemComponent::ControlRotation(bool ShouldControlRotation)
{
	APawn* Pawn = Cast<APawn>(CharacterReference);
	if (Pawn)
	{
		Pawn->bUseControllerRotationYaw = ShouldControlRotation;
	}

	UCharacterMovementComponent* CharacterMovementComponent = CharacterReference->FindComponentByClass<UCharacterMovementComponent>();
	if (CharacterMovementComponent)
	{
		CharacterMovementComponent->bOrientRotationToMovement = !ShouldControlRotation;
	}
}

bool UTargetSystemComponent::IsInViewport(AActor* TargetActor)
{
	UE_LOG(LogTemp, Warning, TEXT("Check if is in Viewport for: %s"), *TargetActor->GetName());
	if (!PlayerController)
	{
		return true;
	}

	FVector2D ScreenLocation;
	PlayerController->ProjectWorldLocationToScreen(TargetActor->GetActorLocation(), ScreenLocation);

	FVector2D ViewportSize;
	GetWorld()->GetGameViewport()->GetViewportSize(ViewportSize);

	return ScreenLocation.X > 0 && ScreenLocation.Y > 0 && ScreenLocation.X < ViewportSize.X && ScreenLocation.Y < ViewportSize.Y;
}
