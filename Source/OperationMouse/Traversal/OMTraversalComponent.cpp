#include "OMTraversalComponent.h"

#include "../Characters/OMMouseCharacter.h"
#include "../OperationMouse.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"

UOMTraversalComponent::UOMTraversalComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	SetIsReplicatedByDefault(true);
}

void UOMTraversalComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<ACharacter>(GetOwner());

	if (!OwnerCharacter)
	{
		UE_LOG(LogOperationMouse, Error, TEXT("TraversalComponent requires an ACharacter owner: %s"), *GetNameSafe(GetOwner()));
	}
}

bool UOMTraversalComponent::TryMantle()
{
	FMantleCandidate LocalCandidate;
	if (!CanStartMantle() || !FindMantleCandidate(LocalCandidate))
	{
		return false;
	}

	if (GetOwner()->HasAuthority())
	{
		StartValidatedMantle(LocalCandidate);
	}
	else
	{
		ServerTryMantle();
	}

	return true;
}

void UOMTraversalComponent::ServerTryMantle_Implementation()
{
	FMantleCandidate ServerCandidate;
	if (!CanStartMantle() || !FindMantleCandidate(ServerCandidate))
	{
		ClientRejectMantle();
		return;
	}

	StartValidatedMantle(ServerCandidate);
}

void UOMTraversalComponent::ClientRejectMantle_Implementation()
{
	if (AOMMouseCharacter* MouseCharacter = Cast<AOMMouseCharacter>(OwnerCharacter))
	{
		MouseCharacter->HandleMantleRejected();
	}
}

bool UOMTraversalComponent::CanStartMantle() const
{
	if (!OwnerCharacter || MantleState.bIsActive || OwnerCharacter->bIsCrouched)
	{
		return false;
	}

	const UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement();
	return MovementComponent && MovementComponent->IsMovingOnGround();
}

bool UOMTraversalComponent::FindMantleCandidate(FMantleCandidate& OutCandidate) const
{
	if (!OwnerCharacter || MinimumMantleHeight >= MaximumMantleHeight)
	{
		return false;
	}

	const FVector CharacterForward = OwnerCharacter->GetActorForwardVector().GetSafeNormal2D();
	if (CharacterForward.IsNearlyZero())
	{
		return false;
	}

	if (FindMantleCandidateInDirection(CharacterForward, OutCandidate))
	{
		return true;
	}

	if (DetectionHalfAngleDegrees <= 0.0f)
	{
		return false;
	}

	const FVector LeftDirection = CharacterForward.RotateAngleAxis(-DetectionHalfAngleDegrees, FVector::UpVector);
	if (FindMantleCandidateInDirection(LeftDirection, OutCandidate))
	{
		return true;
	}

	const FVector RightDirection = CharacterForward.RotateAngleAxis(DetectionHalfAngleDegrees, FVector::UpVector);
	return FindMantleCandidateInDirection(RightDirection, OutCandidate);
}

bool UOMTraversalComponent::FindMantleCandidateInDirection(
	const FVector& DetectionDirection,
	FMantleCandidate& OutCandidate) const
{
	const FVector ForwardDirection = DetectionDirection.GetSafeNormal2D();
	if (ForwardDirection.IsNearlyZero())
	{
		return false;
	}

	const UWorld* World = GetWorld();
	const UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent();
	if (!World || !Capsule)
	{
		return false;
	}

	const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
	const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	const float CharacterBaseZ = CharacterLocation.Z - CapsuleHalfHeight;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OMMantleDetection), false, OwnerCharacter);
	QueryParams.bReturnPhysicalMaterial = false;

	const float ObstacleHalfHeight = FMath::Max((MaximumMantleHeight - MinimumMantleHeight) * 0.5f, ObstacleTraceRadius);
	const float ObstacleCenterZ = CharacterBaseZ + MinimumMantleHeight + ObstacleHalfHeight;
	const FVector ObstacleTraceStart(CharacterLocation.X, CharacterLocation.Y, ObstacleCenterZ);
	const FVector ObstacleTraceEnd = ObstacleTraceStart + ForwardDirection * ForwardDetectionDistance;

	FHitResult ObstacleHit;
	const bool bHitObstacle = World->SweepSingleByChannel(
		ObstacleHit,
		ObstacleTraceStart,
		ObstacleTraceEnd,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeCapsule(ObstacleTraceRadius, ObstacleHalfHeight),
		QueryParams);

	if (bDrawDebug)
	{
		DrawDebugLine(World, ObstacleTraceStart, ObstacleTraceEnd, bHitObstacle ? FColor::Yellow : FColor::Red, false, 1.5f, 0, 2.0f);
	}

	if (!bHitObstacle || ObstacleHit.GetActor() == nullptr || ObstacleHit.GetActor()->IsRootComponentMovable())
	{
		return false;
	}

	if (FVector::DotProduct(ObstacleHit.ImpactNormal.GetSafeNormal2D(), ForwardDirection) > -0.2f)
	{
		return false;
	}

	const FVector TopTraceHorizontal = ObstacleHit.ImpactPoint + ForwardDirection * (CapsuleRadius + LandingForwardOffset);
	const FVector TopTraceStart(TopTraceHorizontal.X, TopTraceHorizontal.Y, CharacterBaseZ + MaximumMantleHeight + CapsuleHalfHeight);
	const FVector TopTraceEnd(TopTraceHorizontal.X, TopTraceHorizontal.Y, CharacterBaseZ + MinimumMantleHeight);

	FHitResult TopHit;
	const bool bHitTop = World->LineTraceSingleByChannel(
		TopHit,
		TopTraceStart,
		TopTraceEnd,
		ECC_Visibility,
		QueryParams);

	if (bDrawDebug)
	{
		DrawDebugLine(World, TopTraceStart, TopTraceEnd, bHitTop ? FColor::Green : FColor::Red, false, 1.5f, 0, 2.0f);
	}

	if (!bHitTop || TopHit.GetActor() != ObstacleHit.GetActor() || TopHit.ImpactNormal.Z < MinimumTopSurfaceNormalZ)
	{
		return false;
	}

	const float MantleHeight = TopHit.ImpactPoint.Z - CharacterBaseZ;
	if (MantleHeight < MinimumMantleHeight || MantleHeight > MaximumMantleHeight)
	{
		return false;
	}

	const FVector TargetLocation = TopHit.ImpactPoint + FVector::UpVector * (CapsuleHalfHeight + LandingFloorOffset);
	const float MaximumHorizontalReach = ForwardDetectionDistance + CapsuleRadius + LandingForwardOffset;
	if (FVector::Dist2D(CharacterLocation, TargetLocation) > MaximumHorizontalReach)
	{
		return false;
	}

	const FCollisionShape DestinationCapsule = FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight);
	if (World->OverlapBlockingTestByChannel(TargetLocation, FQuat::Identity, ECC_Pawn, DestinationCapsule, QueryParams))
	{
		if (bDrawDebug)
		{
			DrawDebugCapsule(World, TargetLocation, CapsuleHalfHeight, CapsuleRadius, FQuat::Identity, FColor::Red, false, 1.5f);
		}
		return false;
	}

	const FCollisionShape PathCapsule = FCollisionShape::MakeCapsule(
		FMath::Max(1.0f, CapsuleRadius - ClearanceMargin),
		FMath::Max(1.0f, CapsuleHalfHeight - ClearanceMargin));

	const FVector RaisedStart(CharacterLocation.X, CharacterLocation.Y, TargetLocation.Z);
	FHitResult PathHit;
	const bool bVerticalPathBlocked = World->SweepSingleByChannel(
		PathHit,
		CharacterLocation,
		RaisedStart,
		FQuat::Identity,
		ECC_Pawn,
		PathCapsule,
		QueryParams);
	const bool bHorizontalPathBlocked = !bVerticalPathBlocked && World->SweepSingleByChannel(
		PathHit,
		RaisedStart,
		TargetLocation,
		FQuat::Identity,
		ECC_Pawn,
		PathCapsule,
		QueryParams);

	if (bVerticalPathBlocked || bHorizontalPathBlocked)
	{
		return false;
	}

	OutCandidate.TargetLocation = TargetLocation;
	OutCandidate.Height = MantleHeight;
	OutCandidate.MantleType = MantleHeight >= HighMantleThreshold ? EOMMantleType::High : EOMMantleType::Low;
	return true;
}

void UOMTraversalComponent::StartValidatedMantle(const FMantleCandidate& Candidate)
{
	check(GetOwner()->HasAuthority());

	UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement();
	FVector ExitVelocity = MovementComponent->Velocity;
	ExitVelocity.Z = 0.0f;
	ExitVelocity = ExitVelocity.GetClampedToMaxSize(MovementComponent->MaxWalkSpeed);
	MovementComponent->StopMovementImmediately();
	MovementComponent->DisableMovement();

	MantleState.bIsActive = true;
	MantleState.StartLocation = OwnerCharacter->GetActorLocation();
	MantleState.TargetLocation = Candidate.TargetLocation;
	MantleState.ServerStartTime = GetSynchronizedTimeSeconds();
	MantleState.Duration = Candidate.MantleType == EOMMantleType::High ? HighMantleDuration : LowMantleDuration;
	MantleState.MantleType = Candidate.MantleType;
	MantleState.ExitVelocity = ExitVelocity;

	SetComponentTickEnabled(true);
	GetOwner()->ForceNetUpdate();

	UE_LOG(
		LogOperationMouse,
		Log,
		TEXT("[Server] Started %s mantle: Pawn=%s Height=%.1f Target=%s"),
		Candidate.MantleType == EOMMantleType::High ? TEXT("high") : TEXT("low"),
		*GetNameSafe(OwnerCharacter),
		Candidate.Height,
		*Candidate.TargetLocation.ToCompactString());
}

void UOMTraversalComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!MantleState.bIsActive || !OwnerCharacter || MantleState.Duration <= 0.0f)
	{
		SetComponentTickEnabled(false);
		return;
	}

	const float Alpha = FMath::Clamp(
		(GetSynchronizedTimeSeconds() - MantleState.ServerStartTime) / MantleState.Duration,
		0.0f,
		1.0f);
	ApplyMantlePosition(Alpha);

	if (GetOwner()->HasAuthority() && Alpha >= 1.0f)
	{
		FinishMantle();
	}
}

void UOMTraversalComponent::ApplyMantlePosition(float Alpha)
{
	const float SmoothedAlpha = FMath::SmoothStep(0.0f, 1.0f, Alpha);
	const FVector StartLocation = MantleState.StartLocation;
	const FVector TargetLocation = MantleState.TargetLocation;
	const FVector RaisedStart(StartLocation.X, StartLocation.Y, TargetLocation.Z);

	FVector NewLocation;
	if (SmoothedAlpha < 0.5f)
	{
		NewLocation = FMath::Lerp(StartLocation, RaisedStart, SmoothedAlpha * 2.0f);
	}
	else
	{
		NewLocation = FMath::Lerp(RaisedStart, TargetLocation, (SmoothedAlpha - 0.5f) * 2.0f);
	}

	OwnerCharacter->SetActorLocation(NewLocation, false, nullptr, ETeleportType::None);
}

void UOMTraversalComponent::FinishMantle()
{
	check(GetOwner()->HasAuthority());

	OwnerCharacter->SetActorLocation(MantleState.TargetLocation, false, nullptr, ETeleportType::None);
	UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement();
	MovementComponent->SetMovementMode(MOVE_Walking);
	MovementComponent->Velocity = MantleState.ExitVelocity;
	MantleState.bIsActive = false;
	SetComponentTickEnabled(false);
	GetOwner()->ForceNetUpdate();

	UE_LOG(LogOperationMouse, Log, TEXT("[Server] Finished mantle: Pawn=%s"), *GetNameSafe(OwnerCharacter));
}

void UOMTraversalComponent::OnRep_MantleState()
{
	if (MantleState.bIsActive)
	{
		if (OwnerCharacter)
		{
			OwnerCharacter->GetCharacterMovement()->StopMovementImmediately();
			OwnerCharacter->GetCharacterMovement()->DisableMovement();
		}
		SetComponentTickEnabled(true);
	}
	else
	{
		if (OwnerCharacter)
		{
			OwnerCharacter->SetActorLocation(MantleState.TargetLocation, false, nullptr, ETeleportType::None);
			if (OwnerCharacter->GetCharacterMovement()->MovementMode == MOVE_None)
			{
				OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			}
			OwnerCharacter->GetCharacterMovement()->Velocity = MantleState.ExitVelocity;
		}
		SetComponentTickEnabled(false);
	}
}

float UOMTraversalComponent::GetSynchronizedTimeSeconds() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}

	const AGameStateBase* GameState = World->GetGameState();
	return GameState ? GameState->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
}

void UOMTraversalComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UOMTraversalComponent, MantleState);
}
