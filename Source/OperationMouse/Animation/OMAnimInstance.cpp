#include "OMAnimInstance.h"

#include "OMAnimInstanceProxy.h"
#include "../OperationMouse.h"
#include "../Traversal/OMTraversalComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UOMAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	UE_LOG(
		LogOperationMouse,
		Log,
		TEXT("[Animation] AnimInstance initialized: Instance=%s Class=%s Pawn=%s"),
		*GetName(),
		*GetClass()->GetName(),
		*GetNameSafe(TryGetPawnOwner()));
}

void UOMAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const ACharacter* Character = Cast<ACharacter>(TryGetPawnOwner());
	if (!Character)
	{
		GroundSpeed = 0.0f;
		MovementDirection = 0.0f;
		VerticalSpeed = 0.0f;
		bIsGrounded = true;
		bIsFalling = false;
		bIsAscending = false;
		bIsCrouched = false;
		bIsSprinting = false;
		bIsMantling = false;
		return;
	}

	const FVector Velocity = Character->GetVelocity();
	GroundSpeed = Velocity.Size2D();
	VerticalSpeed = Velocity.Z;

	const FVector LocalVelocity = Character->GetActorTransform().InverseTransformVectorNoScale(Velocity);
	MovementDirection = GroundSpeed > UE_KINDA_SMALL_NUMBER
		? FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y, LocalVelocity.X))
		: 0.0f;

	const UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();
	bIsFalling = MovementComponent && MovementComponent->IsFalling();
	if (bWasFalling && !bIsFalling)
	{
		LandingTimeRemaining = LandingVisualSeconds;
	}
	else
	{
		LandingTimeRemaining = FMath::Max(0.0f, LandingTimeRemaining - DeltaSeconds);
	}
	bWasFalling = bIsFalling;
	bIsGrounded = !bIsFalling;
	bIsAscending = bIsFalling && VerticalSpeed > 0.0f;
	bIsCrouched = Character->bIsCrouched;
	bIsSprinting = bIsGrounded && !bIsCrouched && GroundSpeed >= SprintSpeedThreshold;

	const UOMTraversalComponent* TraversalComponent = Character->FindComponentByClass<UOMTraversalComponent>();
	bIsMantling = TraversalComponent && TraversalComponent->IsMantling();

	if (FParse::Param(FCommandLine::Get(), TEXT("OMAnimDiagnostics")))
	{
		FName CurrentState = TEXT("Idle");
		if (bIsMantling)
		{
			CurrentState = TEXT("Mantle");
		}
		else if (bIsFalling)
		{
			CurrentState = bIsAscending ? TEXT("Jump") : TEXT("Fall");
		}
		else if (LandingTimeRemaining > 0.0f)
		{
			CurrentState = TEXT("Land");
		}
		else if (bIsCrouched)
		{
			CurrentState = GroundSpeed > 5.0f ? TEXT("CrouchWalk") : TEXT("CrouchIdle");
		}
		else if (GroundSpeed > 5.0f)
		{
			CurrentState = bIsSprinting ? TEXT("Run") : TEXT("Walk");
		}

		if (CurrentState != LastDiagnosticState)
		{
			LastDiagnosticState = CurrentState;
			UE_LOG(
				LogOperationMouse,
				Log,
				TEXT("[Animation] State=%s GroundSpeed=%.1f VerticalSpeed=%.1f Falling=%s Crouched=%s Sprinting=%s Mantling=%s"),
				*CurrentState.ToString(),
				GroundSpeed,
				VerticalSpeed,
				bIsFalling ? TEXT("true") : TEXT("false"),
				bIsCrouched ? TEXT("true") : TEXT("false"),
				bIsSprinting ? TEXT("true") : TEXT("false"),
				bIsMantling ? TEXT("true") : TEXT("false"));
		}
	}
}

FAnimInstanceProxy* UOMAnimInstance::CreateAnimInstanceProxy()
{
	return new FOMAnimInstanceProxy(this);
}

void UOMAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
	delete InProxy;
}
