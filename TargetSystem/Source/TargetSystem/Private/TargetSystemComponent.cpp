// Fill out your copyright notice in the Description page of Project Settings.

#include "TargetSystemComponent.h"
#include "TargetSystemLockOnWidget.h"
#include "TargetSystemTargetableInterface.h"
#include "Engine/World.h"
#include "Engine/Public/TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/Classes/Camera/CameraComponent.h"
#include "EngineUtils.h"

// Sets default values for this component's properties
UTargetSystemComponent::UTargetSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	MinimumDistanceToEnable = 1200.0f;
	ClosestTargetDistance = 0.0f;
	TargetLocked = false;
	TargetLockedOnWidgetDrawSize = 32.0f;
	TargetLockedOnWidgetRelativeLocation = FVector(0.0f, 0.0f, 20.0f);
	BreakLineOfSightDelay = 2.0f;
	bIsBreakingLineOfSight = false;
	ShouldControlRotationWhenLockedOn = true;
	StartRotatingThreshold = 1.5f;
	bIsSwitchingTarget = false;

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
			TargetLockOff();
		}

		if (ShouldBreakLineOfSight() && !bIsBreakingLineOfSight)
		{
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
	ClosestTargetDistance = MinimumDistanceToEnable;

	if (TargetLocked)
	{
		TargetLockOff();
	} else
	{
		TArray<AActor*> Actors = GetAllActorsOfClass(TargetableActors);
		NearestTarget = FindNearestTarget(Actors);
		TargetLockOn(NearestTarget);
	}
}

void UTargetSystemComponent::TargetActorWithAxisInput(float AxisValue)
{
	// If Axis value does not exceeds configured threshold, do nothing
	if (FMath::Abs(AxisValue) < StartRotatingThreshold)
	{
		return;
	}

	// If we're not locked on, do nothing
	if (!TargetLocked)
	{
		return;
	}

	// If we're switching target, do nothing for a set amount of time
	if (bIsSwitchingTarget)
	{
		return;
	}
	
	// Lock off target
	AActor* CurrentTarget = NearestTarget;

	// Depending on Axis Value negative / positive, set Direction to Look for (negative: left, positive: right)
	float RangeMin = AxisValue < 0 ? 0 : 180;
	float RangeMax = AxisValue < 0 ? 180 : 360;

	// Reset Closest Target Distance to Minimum Distance to Enable
	ClosestTargetDistance = MinimumDistanceToEnable;

	// Get All Actors of Class
	TArray<AActor*> Actors = GetAllActorsOfClass(TargetableActors);
	
	// For each of these actors, check line trace and ignore Current Target and build the list of actors to look from
	TArray<AActor*> ActorsToLook;

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(CurrentTarget);
	for (AActor* Actor : Actors)
	{
		bool bHit = LineTraceForActor(Actor, ActorsToIgnore);
		if (bHit && IsInViewport(Actor))
		{
			ActorsToLook.Add(Actor);
		}
	}

	// Find Targets in Range (left or right, based on Character and CurrentTarget)
	TArray<AActor*> TargetsInRange = FindTargetsInRange(ActorsToLook, RangeMin, RangeMax);

	// For each of these targets in range, get the closest one to current target
	AActor* ActorToTarget = nullptr;
	for (AActor* Actor : TargetsInRange)
	{
		// and filter out any character too distant from minimum distance to enable
		float Distance = GetDistanceFromCharacter(Actor);
		if (Distance < MinimumDistanceToEnable)
		{
			float RelativeActorsDistance = CurrentTarget->GetDistanceTo(Actor);
			if (RelativeActorsDistance < ClosestTargetDistance)
			{
				ClosestTargetDistance = RelativeActorsDistance;
				ActorToTarget = Actor;
			}
		}
	}

	if (ActorToTarget)
	{
		bIsSwitchingTarget = true;
		TargetLockOff();
		NearestTarget = ActorToTarget;
		TargetLockOn(ActorToTarget);
		GetWorld()->GetTimerManager().SetTimer(
			SwitchingTargetTimerHandle,
			this,
			&UTargetSystemComponent::ResetIsSwitchingTarget,
			0.5f
		);
	}
}

TArray<AActor*> UTargetSystemComponent::FindTargetsInRange(TArray<AActor*> ActorsToLook, float RangeMin, float RangeMax)
{
	TArray<AActor*> ActorsInRange;

	for (AActor* Actor : ActorsToLook)
	{
		float Angle = GetAngleUsingCameraRotation(Actor);
		if (Angle > RangeMin && Angle < RangeMax)
		{
			ActorsInRange.Add(Actor);
		}
	}

	return ActorsInRange;
}

float UTargetSystemComponent::GetAngleUsingCameraRotation(AActor* ActorToLook)
{
	UCameraComponent* CameraComponent = CharacterReference->FindComponentByClass<UCameraComponent>();
	if (!CameraComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("TargetSystem::GetAngleUsingCameraRotation() Cannot get reference to camera component"));
		return 0.0f;
	}

	FRotator CameraWorldRotation = CameraComponent->GetComponentRotation();
	FRotator LookAtRotation = FindLookAtRotation(CameraComponent->GetComponentLocation(), ActorToLook->GetActorLocation());

	float YawAngle = CameraWorldRotation.Yaw - LookAtRotation.Yaw;
	if (YawAngle < 0)
	{
		YawAngle = YawAngle + 360;
	}

	return YawAngle;
}

FRotator UTargetSystemComponent::FindLookAtRotation(const FVector Start, const FVector Target)
{
	return FRotationMatrix::MakeFromX(Target - Start).Rotator();
}

void UTargetSystemComponent::ResetIsSwitchingTarget()
{
	bIsSwitchingTarget = false;
}

void UTargetSystemComponent::TargetLockOn(AActor* TargetToLockOn)
{
	if (TargetToLockOn)
	{
		TargetLocked = true;
		CreateAndAttachTargetLockedOnWidgetComponent(TargetToLockOn);

		if (ShouldControlRotationWhenLockedOn)
		{
			ControlRotation(true);
		}

		CharacterController->SetIgnoreLookInput(true);

		OnTargetLockedOn.Broadcast(TargetToLockOn);
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

		CharacterController->ResetIgnoreLookInput();

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
	TargetLockedOnWidgetComponent->SetRelativeLocation(TargetLockedOnWidgetRelativeLocation);
	TargetLockedOnWidgetComponent->SetDrawSize(FVector2D(TargetLockedOnWidgetDrawSize, TargetLockedOnWidgetDrawSize));
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

	FRotator TargetRotation = FRotator(LookRotation.Pitch, LookRotation.Yaw, ControlRotation.Roll);

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
