// Copyright 2018-2021 Mickael Daniel. All Rights Reserved.

#include "TargetSystemComponent.h"
#include "TargetSystemTargetableInterface.h"
#include "Components/WidgetComponent.h"
#include "EngineUtils.h"
#include "TargetSystemLog.h"
#include "Engine/Classes/Camera/CameraComponent.h"
#include "Engine/Classes/Kismet/GameplayStatics.h"
#include "Engine/Public/TimerManager.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

// Sets default values for this component's properties
UTargetSystemComponent::UTargetSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	LockedOnWidgetClass = StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/TargetSystem/UI/WBP_LockOn.WBP_LockOn_C"));
	TargetableActors = APawn::StaticClass();
	TargetableCollisionChannel = ECollisionChannel::ECC_Pawn;
}

// Called when the game starts
void UTargetSystemComponent::BeginPlay()
{

	// TS_LOG(Log, TEXT("TargetSystemComponent: 4.26.0"));

	Super::BeginPlay();
	CharacterReference = GetOwner();
	if (!CharacterReference)
	{
		TS_LOG(Error, TEXT("[%s] TargetSystemComponent: Cannot get Owner reference ..."), *GetName());
		return;
	}

	PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PlayerController)
	{
		TS_LOG(Error, TEXT("[%s] TargetSystemComponent: Cannot get PlayerController reference ..."), *CharacterReference->GetName());
		return;
	}
}

void UTargetSystemComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bTargetLocked && LockedOnTargetActor)
	{
		if (!TargetIsTargetable(LockedOnTargetActor))
		{
			TargetLockOff();
			return;
		}

		SetControlRotationOnTarget(LockedOnTargetActor);

		// Target Locked Off based on Distance
		if (GetDistanceFromCharacter(LockedOnTargetActor) > MinimumDistanceToEnable)
		{
			TargetLockOff();
		}

		if (ShouldBreakLineOfSight() && !bIsBreakingLineOfSight)
		{
			if (BreakLineOfSightDelay <= 0)
			{
				TargetLockOff();
			}
			else
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

	if (bTargetLocked)
	{
		TargetLockOff();
	}
	else
	{
		const TArray<AActor*> Actors = GetAllActorsOfClass(TargetableActors);
		LockedOnTargetActor = FindNearestTarget(Actors);
		TargetLockOn(LockedOnTargetActor);
	}
}

void UTargetSystemComponent::TargetActorWithAxisInput(const float AxisValue)
{
	// If we're not locked on, do nothing
	if (!bTargetLocked)
	{
		return;
	}

	if (!LockedOnTargetActor)
	{
		return;
	}

	// If we're not allowed to switch target, do nothing
	if (!ShouldSwitchTargetActor(AxisValue))
	{
		return;
	}

	// If we're switching target, do nothing for a set amount of time
	if (bIsSwitchingTarget)
	{
		return;
	}

	// Lock off target
	AActor* CurrentTarget = LockedOnTargetActor;

	// Depending on Axis Value negative / positive, set Direction to Look for (negative: left, positive: right)
	const float RangeMin = AxisValue < 0 ? 0 : 180;
	const float RangeMax = AxisValue < 0 ? 180 : 360;

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
		const bool bHit = LineTraceForActor(Actor, ActorsToIgnore);
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
		const float Distance = GetDistanceFromCharacter(Actor);
		if (Distance < MinimumDistanceToEnable)
		{
			const float RelativeActorsDistance = CurrentTarget->GetDistanceTo(Actor);
			if (RelativeActorsDistance < ClosestTargetDistance)
			{
				ClosestTargetDistance = RelativeActorsDistance;
				ActorToTarget = Actor;
			}
		}
	}

	if (ActorToTarget)
	{
		if (SwitchingTargetTimerHandle.IsValid())
		{
			SwitchingTargetTimerHandle.Invalidate();
		}

		TargetLockOff();
		LockedOnTargetActor = ActorToTarget;
		TargetLockOn(ActorToTarget);

		GetWorld()->GetTimerManager().SetTimer(
			SwitchingTargetTimerHandle,
			this,
			&UTargetSystemComponent::ResetIsSwitchingTarget,
			// Less sticky if still switching
			bIsSwitchingTarget ? 0.25f : 0.5f
		);

		bIsSwitchingTarget = true;
	}
}

bool UTargetSystemComponent::GetTargetLockedStatus(){
	return bTargetLocked;
}

AActor* UTargetSystemComponent::GetLockedOnTargetActor() const
{
	return LockedOnTargetActor;
}

bool UTargetSystemComponent::IsLocked() const
{
	return bTargetLocked && LockedOnTargetActor;
}

TArray<AActor*> UTargetSystemComponent::FindTargetsInRange(TArray<AActor*> ActorsToLook, const float RangeMin, const float RangeMax) const
{
	TArray<AActor*> ActorsInRange;

	for (AActor* Actor : ActorsToLook)
	{
		const float Angle = GetAngleUsingCameraRotation(Actor);
		if (Angle > RangeMin && Angle < RangeMax)
		{
			ActorsInRange.Add(Actor);
		}
	}

	return ActorsInRange;
}

float UTargetSystemComponent::GetAngleUsingCameraRotation(AActor* ActorToLook) const
{
	UCameraComponent* CameraComponent = CharacterReference->FindComponentByClass<UCameraComponent>();
	if (!CameraComponent)
	{
		// Fallback to CharacterRotation if no CameraComponent can be found
		return GetAngleUsingCharacterRotation(ActorToLook);
	}

	const FRotator CameraWorldRotation = CameraComponent->GetComponentRotation();
	const FRotator LookAtRotation = FindLookAtRotation(CameraComponent->GetComponentLocation(), ActorToLook->GetActorLocation());

	float YawAngle = CameraWorldRotation.Yaw - LookAtRotation.Yaw;
	if (YawAngle < 0)
	{
		YawAngle = YawAngle + 360;
	}

	return YawAngle;
}

float UTargetSystemComponent::GetAngleUsingCharacterRotation(AActor* ActorToLook) const
{
	const FRotator CharacterRotation = CharacterReference->GetActorRotation();
	const FRotator LookAtRotation = FindLookAtRotation(CharacterReference->GetActorLocation(), ActorToLook->GetActorLocation());

	float YawAngle = CharacterRotation.Yaw - LookAtRotation.Yaw;
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
	bDesireToSwitch = false;
}

bool UTargetSystemComponent::ShouldSwitchTargetActor(const float AxisValue)
{
	// Sticky feeling computation
	if (bEnableStickyTarget)
	{
		StartRotatingStack += (AxisValue != 0) ?  AxisValue * AxisMultiplier : (StartRotatingStack > 0 ? -AxisMultiplier : AxisMultiplier);

		if (AxisValue == 0 && FMath::Abs(StartRotatingStack) <= AxisMultiplier)
		{

			StartRotatingStack = 0.0f;
		}

		// If Axis value does not exceeds configured threshold, do nothing
		if (FMath::Abs(StartRotatingStack) < StickyRotationThreshold)
		{
			bDesireToSwitch = false;
			return false;
		}

		//Sticky when switching target.
		if (StartRotatingStack * AxisValue > 0)
		{
			StartRotatingStack = StartRotatingStack > 0 ? StickyRotationThreshold : -StickyRotationThreshold;
		}
		else if (StartRotatingStack * AxisValue < 0)
		{
			StartRotatingStack = StartRotatingStack * -1.0f;

		}

		bDesireToSwitch = true;

		return true;
	}

	// Non Sticky feeling, check Axis value exceeds threshold
	return FMath::Abs(AxisValue) > StartRotatingThreshold;
}

void UTargetSystemComponent::TargetLockOn(AActor* TargetToLockOn)
{
	if (TargetToLockOn)
	{

		bTargetLocked = true;
		if (bShouldDrawLockedOnWidget)
		{
			CreateAndAttachTargetLockedOnWidgetComponent(TargetToLockOn);
		}

		if (bShouldControlRotation)
		{
			ControlRotation(true);
		}

		if (bAdjustPitchBasedOnDistanceToTarget || bIgnoreLookInput)
		{
			PlayerController->SetIgnoreLookInput(true);
		}

		if (OnTargetLockedOn.IsBound())
		{
			OnTargetLockedOn.Broadcast(TargetToLockOn);
		}
	}
}

void UTargetSystemComponent::TargetLockOff()
{
	bTargetLocked = false;
	if (TargetLockedOnWidgetComponent)
	{
		TargetLockedOnWidgetComponent->DestroyComponent();
	}

	if (LockedOnTargetActor)
	{
		if (bShouldControlRotation)
		{
			ControlRotation(false);
		}

		PlayerController->ResetIgnoreLookInput();

		if (OnTargetLockedOff.IsBound())
		{
			OnTargetLockedOff.Broadcast(LockedOnTargetActor);
		}
	}

	LockedOnTargetActor = nullptr;
}

void UTargetSystemComponent::CreateAndAttachTargetLockedOnWidgetComponent(AActor* TargetActor)
{
	if (!LockedOnWidgetClass)
	{
		TS_LOG(Error, TEXT("TargetSystemComponent: Cannot get LockedOnWidgetClass, please ensure it is a valid reference in the Component Properties."));
		return;
	}

	TargetLockedOnWidgetComponent = NewObject<UWidgetComponent>(TargetActor, MakeUniqueObjectName( TargetActor, UWidgetComponent::StaticClass(), FName("TargetLockOn") ) );
	TargetLockedOnWidgetComponent->SetWidgetClass(LockedOnWidgetClass);

	UMeshComponent* MeshComponent = TargetActor->FindComponentByClass<UMeshComponent>();
	USceneComponent* ParentComponent = MeshComponent && LockedOnWidgetParentSocket != NAME_None ? MeshComponent : TargetActor->GetRootComponent();

	TargetLockedOnWidgetComponent->ComponentTags.Add(FName("TargetSystem.LockOnWidget"));
	TargetLockedOnWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	TargetLockedOnWidgetComponent->SetupAttachment(ParentComponent, LockedOnWidgetParentSocket);
	TargetLockedOnWidgetComponent->SetRelativeLocation(LockedOnWidgetRelativeLocation);
	TargetLockedOnWidgetComponent->SetDrawSize(FVector2D(LockedOnWidgetDrawSize, LockedOnWidgetDrawSize));
	TargetLockedOnWidgetComponent->SetVisibility(true);
	TargetLockedOnWidgetComponent->RegisterComponent();
}

TArray<AActor*> UTargetSystemComponent::GetAllActorsOfClass(const TSubclassOf<AActor> ActorClass) const
{
	TArray<AActor*> Actors;
	for (TActorIterator<AActor> ActorIterator(GetWorld(), ActorClass); ActorIterator; ++ActorIterator)
	{
		AActor* Actor = *ActorIterator;
		const bool bIsTargetable = TargetIsTargetable(Actor);
		if (bIsTargetable)
		{
			Actors.Add(Actor);
		}
	}

	return Actors;
}

bool UTargetSystemComponent::TargetIsTargetable(AActor* Actor)
{
	const bool bIsImplemented = Actor->GetClass()->ImplementsInterface(UTargetSystemTargetableInterface::StaticClass());
	if (bIsImplemented)
	{
		return ITargetSystemTargetableInterface::Execute_IsTargetable(Actor);
	}

	return true;
}

AActor* UTargetSystemComponent::FindNearestTarget(TArray<AActor*> Actors) const
{
	TArray<AActor*> ActorsHit;

	// Find all actors we can line trace to
	for (AActor* Actor : Actors)
	{
		const bool bHit = LineTraceForActor(Actor);
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
		const float Distance = GetDistanceFromCharacter(Actor);
		if (Distance < ClosestDistance)
		{
			ClosestDistance = Distance;
			Target = Actor;
		}
	}

	return Target;
}


bool UTargetSystemComponent::LineTraceForActor(AActor* OtherActor, const TArray<AActor*> ActorsToIgnore) const
{
	FHitResult HitResult;
	const bool bHit = LineTrace(HitResult, OtherActor, ActorsToIgnore);
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

bool UTargetSystemComponent::LineTrace(FHitResult& HitResult, AActor* OtherActor, const TArray<AActor*> ActorsToIgnore) const
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
		(ECollisionChannel) TargetableCollisionChannel,
		Params
	);
}

FRotator UTargetSystemComponent::GetControlRotationOnTarget(AActor* OtherActor) const
{
	const FRotator ControlRotation = PlayerController->GetControlRotation();

	const FVector CharacterLocation = CharacterReference->GetActorLocation();
	const FVector OtherActorLocation = OtherActor->GetActorLocation();

	// Find look at rotation
	const FRotator LookRotation = FRotationMatrix::MakeFromX(OtherActorLocation - CharacterLocation).Rotator();
	float Pitch = LookRotation.Pitch;
	FRotator TargetRotation;
	if (bAdjustPitchBasedOnDistanceToTarget)
	{
		const float DistanceToTarget = GetDistanceFromCharacter(OtherActor);
		const float PitchInRange = (DistanceToTarget * PitchDistanceCoefficient + PitchDistanceOffset) * -1.0f;
		const float PitchOffset = FMath::Clamp(PitchInRange, PitchMin, PitchMax);

		Pitch = Pitch + PitchOffset;
		TargetRotation = FRotator(Pitch, LookRotation.Yaw, ControlRotation.Roll);
	}
	else
	{
		if (bIgnoreLookInput) {
			TargetRotation = FRotator(Pitch, LookRotation.Yaw, ControlRotation.Roll);
		} else {
			TargetRotation = FRotator(ControlRotation.Pitch, LookRotation.Yaw, ControlRotation.Roll);
		}
	}

	return FMath::RInterpTo( ControlRotation, TargetRotation, GetWorld()->GetDeltaSeconds(), 9.0f );

}

void UTargetSystemComponent::SetControlRotationOnTarget(AActor* TargetActor) const
{
	if (!PlayerController)
	{
		return;
	}

	const FRotator ControlRotation = GetControlRotationOnTarget(TargetActor);
    if (OnTargetSetRotation.IsBound())
    {
        OnTargetSetRotation.Broadcast(TargetActor, ControlRotation);
    }
    else
    {
        PlayerController->SetControlRotation(ControlRotation);
    }
}

float UTargetSystemComponent::GetDistanceFromCharacter(AActor* OtherActor) const
{
	return CharacterReference->GetDistanceTo(OtherActor);
}

bool UTargetSystemComponent::ShouldBreakLineOfSight() const
{
	if (!LockedOnTargetActor)
	{
		return true;
	}

	TArray<AActor*> ActorsToIgnore = GetAllActorsOfClass(TargetableActors);
	ActorsToIgnore.Remove(LockedOnTargetActor);

	FHitResult HitResult;
	const bool bHit = LineTrace(HitResult, LockedOnTargetActor, ActorsToIgnore);
	if (bHit && HitResult.GetActor() != LockedOnTargetActor)
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

void UTargetSystemComponent::ControlRotation(const bool ShouldControlRotation) const
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

bool UTargetSystemComponent::IsInViewport(AActor* TargetActor) const
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
