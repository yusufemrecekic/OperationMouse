#include "OMAnimInstance.h"

#include "OMAnimInstanceProxy.h"
#include "../Traversal/OMTraversalComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UOMAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
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
}

FAnimInstanceProxy* UOMAnimInstance::CreateAnimInstanceProxy()
{
	return new FOMAnimInstanceProxy(this);
}

void UOMAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
	delete InProxy;
}
