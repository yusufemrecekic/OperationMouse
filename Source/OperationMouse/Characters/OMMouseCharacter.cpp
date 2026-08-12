#include "OMMouseCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "../OperationMouse.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
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

	if (!MoveAction || !LookAction)
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
