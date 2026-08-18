#include "OMMouseCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "../OperationMouse.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "../Interaction/OMInteractionComponent.h"
#include "../Carry/OMCarryComponent.h"
#include "TimerManager.h"
#include "../Traversal/OMTraversalComponent.h"
#include "UObject/ConstructorHelpers.h"

AOMMouseCharacter::AOMMouseCharacter()
{
	bReplicates = true;
	SetReplicateMovement(true);
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0, 500.0, 0.0);
	GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed;
	GetCharacterMovement()->AirControl = AirControlStrength;
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;

	PlaceholderVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderVisual"));
	PlaceholderVisual->SetupAttachment(GetCapsuleComponent());
	PlaceholderVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaceholderVisual->SetRelativeScale3D(FVector(0.45, 0.45, 0.85));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaceholderMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (PlaceholderMesh.Succeeded())
	{
		PlaceholderVisual->SetStaticMesh(PlaceholderMesh.Object);
	}

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	CarryPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CarryPoint"));
	CarryPoint->SetupAttachment(GetCapsuleComponent());
	CarryPoint->SetRelativeLocation(FVector(100.0f, 0.0f, 35.0f));

	TraversalComponent = CreateDefaultSubobject<UOMTraversalComponent>(TEXT("TraversalComponent"));
	InteractionComponent = CreateDefaultSubobject<UOMInteractionComponent>(TEXT("InteractionComponent"));
	CarryComponent = CreateDefaultSubobject<UOMCarryComponent>(TEXT("CarryComponent"));

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> GameplayMappingContextAsset(
		TEXT("/Game/OperationMouse/Input/IMC_Gameplay.IMC_Gameplay"));
	if (GameplayMappingContextAsset.Succeeded())
	{
		GameplayMappingContext = GameplayMappingContextAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> MoveActionAsset(
		TEXT("/Game/OperationMouse/Input/IA_Move.IA_Move"));
	if (MoveActionAsset.Succeeded())
	{
		MoveAction = MoveActionAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> LookActionAsset(
		TEXT("/Game/OperationMouse/Input/IA_Look.IA_Look"));
	if (LookActionAsset.Succeeded())
	{
		LookAction = LookActionAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> JumpActionAsset(
		TEXT("/Game/OperationMouse/Input/IA_Jump.IA_Jump"));
	if (JumpActionAsset.Succeeded())
	{
		JumpAction = JumpActionAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> SprintActionAsset(
		TEXT("/Game/OperationMouse/Input/IA_Sprint.IA_Sprint"));
	if (SprintActionAsset.Succeeded())
	{
		SprintAction = SprintActionAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> CrouchActionAsset(
		TEXT("/Game/OperationMouse/Input/IA_Crouch.IA_Crouch"));
	if (CrouchActionAsset.Succeeded())
	{
		CrouchAction = CrouchActionAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> InteractActionAsset(
		TEXT("/Game/OperationMouse/Input/IA_Interact.IA_Interact"));
	if (InteractActionAsset.Succeeded())
	{
		InteractAction = InteractActionAsset.Object;
	}
}

void AOMMouseCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (CarryComponent)
	{
		CarryComponent->SetCarryPoint(CarryPoint);
	}
	SetSprinting(false);
}

void AOMMouseCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	const APlayerController* PlayerController = Cast<APlayerController>(Controller);
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
	{
		if (GameplayMappingContext)
		{
			InputSubsystem->AddMappingContext(GameplayMappingContext, 0);
			UE_LOG(
				LogOperationMouse,
				Log,
				TEXT("[Local] Applied input mapping context: Pawn=%s Context=%s"),
				*GetName(),
				*GameplayMappingContext->GetName());
		}
		else
		{
			UE_LOG(LogOperationMouse, Warning, TEXT("IA mapping context is missing on %s"), *GetName());
		}
	}
}

void AOMMouseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent)
	{
		UE_LOG(LogOperationMouse, Error, TEXT("Enhanced Input component is missing on %s"), *GetName());
		return;
	}

	if (MoveAction)
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AOMMouseCharacter::Move);
	}

	if (LookAction)
	{
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AOMMouseCharacter::Look);
	}

	if (JumpAction)
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AOMMouseCharacter::StartJump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AOMMouseCharacter::StopJump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Canceled, this, &AOMMouseCharacter::StopJump);
	}

	if (SprintAction)
	{
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AOMMouseCharacter::StartSprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AOMMouseCharacter::StopSprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &AOMMouseCharacter::StopSprint);
	}

	if (CrouchAction)
	{
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &AOMMouseCharacter::StartCrouch);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AOMMouseCharacter::StopCrouch);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Canceled, this, &AOMMouseCharacter::StopCrouch);
	}

	if (InteractAction)
	{
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AOMMouseCharacter::StartInteraction);
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Completed, this, &AOMMouseCharacter::StopInteraction);
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Canceled, this, &AOMMouseCharacter::StopInteraction);
	}

	if (!MoveAction || !LookAction || !JumpAction || !SprintAction || !CrouchAction || !InteractAction)
	{
		UE_LOG(LogOperationMouse, Warning, TEXT("Enhanced Input actions are incomplete on %s"), *GetName());
	}
}

void AOMMouseCharacter::Move(const FInputActionValue& Value)
{
	if (!Controller)
	{
		return;
	}

	const FVector2D MovementInput = Value.Get<FVector2D>();
	const FRotator YawRotation(0.0, Controller->GetControlRotation().Yaw, 0.0);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, MovementInput.Y);
	AddMovementInput(RightDirection, MovementInput.X);
}

void AOMMouseCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookInput = Value.Get<FVector2D>();

	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(-LookInput.Y);
}

void AOMMouseCharacter::StartJump()
{
	bJumpInputHeld = true;
	if (TraversalComponent && TraversalComponent->IsMantling())
	{
		return;
	}

	if (TraversalComponent && TraversalComponent->TryMantle())
	{
		BufferedJumpExpiration = -1.0;
		return;
	}

	AttemptJumpOrBuffer();
}

void AOMMouseCharacter::AttemptJumpOrBuffer()
{

	if (CanJump())
	{
		BufferedJumpExpiration = -1.0;
		Jump();
		return;
	}

	if (const UWorld* World = GetWorld())
	{
		BufferedJumpExpiration = World->GetTimeSeconds() + JumpInputBufferSeconds;
	}
}

void AOMMouseCharacter::HandleMantleRejected()
{
	AttemptJumpOrBuffer();
}

void AOMMouseCharacter::StopJump()
{
	bJumpInputHeld = false;
	StopJumping();
}

void AOMMouseCharacter::StartSprint()
{
	SetSprinting(true);

	if (!HasAuthority())
	{
		ServerSetSprinting(true);
	}
}

void AOMMouseCharacter::StopSprint()
{
	SetSprinting(false);

	if (!HasAuthority())
	{
		ServerSetSprinting(false);
	}
}

void AOMMouseCharacter::StartCrouch()
{
	Crouch();
}

void AOMMouseCharacter::StopCrouch()
{
	UnCrouch();
}

void AOMMouseCharacter::StartInteraction()
{
	if (CarryComponent && CarryComponent->IsCarrying())
	{
		CarryComponent->Drop();
		return;
	}

	if (InteractionComponent)
	{
		InteractionComponent->BeginInteractionInput();
	}
}

void AOMMouseCharacter::StopInteraction()
{
	if (InteractionComponent)
	{
		InteractionComponent->EndInteractionInput();
	}
}

void AOMMouseCharacter::SetSprinting(bool bNewSprinting)
{
	bIsSprinting = bNewSprinting;
	GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? SprintSpeed : NormalWalkSpeed;
}

void AOMMouseCharacter::ServerSetSprinting_Implementation(bool bNewSprinting)
{
	SetSprinting(bNewSprinting);
}

void AOMMouseCharacter::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	const bool bWasGrounded = PreviousMovementMode == MOVE_Walking || PreviousMovementMode == MOVE_NavWalking;

	if (bWasGrounded && MovementComponent->IsFalling())
	{
		if (const UWorld* World = GetWorld())
		{
			CoyoteTimeExpiration = World->GetTimeSeconds() + CoyoteTimeSeconds;
		}
	}
	else if (MovementComponent->IsMovingOnGround())
	{
		CoyoteTimeExpiration = -1.0;
		TryConsumeBufferedJump();
	}
}

bool AOMMouseCharacter::CanJumpWhileFalling() const
{
	const UWorld* World = GetWorld();
	return World && World->GetTimeSeconds() <= CoyoteTimeExpiration;
}

void AOMMouseCharacter::TryConsumeBufferedJump()
{
	const UWorld* World = GetWorld();
	if (!World || World->GetTimeSeconds() > BufferedJumpExpiration || !CanJump())
	{
		BufferedJumpExpiration = -1.0;
		return;
	}

	BufferedJumpExpiration = -1.0;
	Jump();

	if (!bJumpInputHeld)
	{
		World->GetTimerManager().SetTimerForNextTick(this, &AOMMouseCharacter::StopJumping);
	}
}

void AOMMouseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	UE_LOG(
		LogOperationMouse,
		Log,
		TEXT("[Server] Character possessed: Pawn=%s Controller=%s"),
		*GetName(),
		*GetNameSafe(NewController));
}

void AOMMouseCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	UE_LOG(
		LogOperationMouse,
		Log,
		TEXT("[Client] Character controller replicated: Pawn=%s Controller=%s"),
		*GetName(),
		*GetNameSafe(GetController()));
}
