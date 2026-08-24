#include "OMCarryableActor.h"

#include "OMCarryComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "../Characters/OMMouseCharacter.h"
#include "../OperationMouse.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AOMCarryableActor::AOMCarryableActor()
{
	bReplicates = true;
	SetReplicateMovement(true);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	Mesh->SetSimulatePhysics(true);
	Mesh->SetRelativeScale3D(FVector(0.45f));

	ClientVisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ClientVisualMesh"));
	ClientVisualMesh->SetupAttachment(Mesh);
	ClientVisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ClientVisualMesh->SetGenerateOverlapEvents(false);
	ClientVisualMesh->SetCanEverAffectNavigation(false);
	ClientVisualMesh->SetSimulatePhysics(false);
	ClientVisualMesh->SetVisibility(false);
	ClientVisualMesh->SetHiddenInGame(true);
	ClientVisualMesh->SetIsReplicated(false);
	ClientVisualMesh->SetUsingAbsoluteLocation(true);
	ClientVisualMesh->SetUsingAbsoluteRotation(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
		ClientVisualMesh->SetStaticMesh(CubeMesh.Object);
	}

	StatusText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StatusText"));
	StatusText->SetupAttachment(Mesh);
	StatusText->SetRelativeLocation(FVector(0.0f, 0.0f, 75.0f));
	StatusText->SetHorizontalAlignment(EHTA_Center);
	StatusText->SetWorldSize(20.0f);
	StatusText->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	InteractionInfo.Prompt = NSLOCTEXT("OperationMouse", "GrabCarryablePrompt", "Grab");
	InteractionInfo.Type = EOMInteractionType::Instant;
	InteractionInfo.MaximumDistance = 220.0f;
	InteractionInfo.bExclusive = true;
}

void AOMCarryableActor::BeginPlay()
{
	Super::BeginPlay();
	HomeTransform = GetActorTransform();
	if (HasAuthority())
	{
		AuthoritativeWorldTransform = HomeTransform;
		WorldStateRevision = 1;
	}
	InitializeClientVisualMesh();
	UpdateStatusText();
}

void AOMCarryableActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority())
	{
		UpdateClientCarryPresentation(DeltaSeconds);
		return;
	}

	if (!IsValid(CurrentHolder))
	{
		return;
	}

	if (UOMCarryComponent* HolderCarryComponent = CurrentHolder->FindComponentByClass<UOMCarryComponent>())
	{
		if (IsValid(HolderCarryComponent->GetCarryPoint()))
		{
			UpdateCarriedTransform();
		}
	}
}

void AOMCarryableActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DeactivateClientCarryPresentation();
	ClearHolderCollisionIgnores();
	Super::EndPlay(EndPlayReason);
}

FOMInteractionInfo AOMCarryableActor::GetInteractionInfo_Implementation(AActor* Interactor) const
{
	return InteractionInfo;
}

FVector AOMCarryableActor::GetInteractionPoint_Implementation() const
{
	return Mesh ? Mesh->GetComponentLocation() : GetActorLocation();
}

bool AOMCarryableActor::CanInteract_Implementation(AActor* Interactor) const
{
	AOMMouseCharacter* Character = Cast<AOMMouseCharacter>(Interactor);
	UOMCarryComponent* CarryComponent = Character ? Character->FindComponentByClass<UOMCarryComponent>() : nullptr;
	return IsAvailableForGrab() && CarryComponent && CarryComponent->CanGrab(const_cast<AOMCarryableActor*>(this));
}

bool AOMCarryableActor::BeginInteraction_Implementation(AActor* Interactor)
{
	return HasAuthority() && CanInteract_Implementation(Interactor);
}

void AOMCarryableActor::CompleteInteraction_Implementation(AActor* Interactor)
{
	if (!HasAuthority())
	{
		UE_LOG(LogOperationMouse, Warning, TEXT("[Carry][Rejected] Carrier=%s Target=%s Reason=NotAuthority"), *GetNameSafe(Interactor), *GetName());
		return;
	}

	AOMMouseCharacter* Character = Cast<AOMMouseCharacter>(Interactor);
	UOMCarryComponent* CarryComponent = Character ? Character->FindComponentByClass<UOMCarryComponent>() : nullptr;
	if (!CarryComponent || !CarryComponent->TryGrab(this))
	{
		UE_LOG(
			LogOperationMouse,
			Log,
			TEXT("[Carry][Rejected] Carrier=%s Target=%s Reason=GameplayHandoffRejected"),
			*GetNameSafe(Interactor),
			*GetName());
	}
}

bool AOMCarryableActor::BeginCarry(UOMCarryComponent* NewCarrier, USceneComponent* NewCarryPoint)
{
	if (!HasAuthority() || !IsAvailableForGrab() || !IsValid(NewCarrier) || !IsValid(NewCarryPoint))
	{
		return false;
	}

	AOMMouseCharacter* NewHolder = Cast<AOMMouseCharacter>(NewCarrier->GetOwner());
	if (!IsValid(NewHolder))
	{
		return false;
	}

	CurrentHolder = NewHolder;
	ApplyCarryPresentation(NewCarryPoint);
	SetActorTickEnabled(HasAuthority());
	ForceNetUpdate();
	return true;
}

bool AOMCarryableActor::EndCarry(UOMCarryComponent* RequestingCarrier, const FVector& DropLocation)
{
	if (!HasAuthority() || !IsValid(RequestingCarrier) || CurrentHolder != RequestingCarrier->GetOwner())
	{
		return false;
	}

	SetActorTickEnabled(false);
	ClearHolderCollisionIgnores();
	CurrentHolder = nullptr;
	FTransform DropTransform = GetActorTransform();
	DropTransform.SetLocation(DropLocation);
	DropTransform.SetRotation(FQuat::Identity);
	PublishAuthoritativeWorldState(DropTransform);
	return true;
}

bool AOMCarryableActor::IsAvailableForGrab() const
{
	return IsValid(this) && !IsActorBeingDestroyed() && !IsValid(CurrentHolder);
}

bool AOMCarryableActor::IsHeldBy(const AOMMouseCharacter* Character) const
{
	return IsValid(Character) && CurrentHolder == Character;
}

FVector AOMCarryableActor::ConstrainHolderMovement(
	const AOMMouseCharacter* Holder,
	const FVector& DesiredWorldMovement) const
{
	if (!IsValid(Holder) || !IsHeldBy(Holder) || CarryObstructionNormal.IsNearlyZero())
	{
		return DesiredWorldMovement;
	}

	const FVector ObstructionNormal = FVector(CarryObstructionNormal).GetSafeNormal();
	return FVector::DotProduct(DesiredWorldMovement, ObstructionNormal) < 0.0f
		? FVector::VectorPlaneProject(DesiredWorldMovement, ObstructionNormal)
		: DesiredWorldMovement;
}

void AOMCarryableActor::ResetToHome()
{
	if (!HasAuthority())
	{
		return;
	}

	if (IsValid(CurrentHolder))
	{
		if (UOMCarryComponent* HolderCarryComponent = CurrentHolder->FindComponentByClass<UOMCarryComponent>())
		{
			HolderCarryComponent->ReleaseForRecovery(this);
		}
	}

	SetActorTickEnabled(false);
	ClearHolderCollisionIgnores();
	CurrentHolder = nullptr;
	PublishAuthoritativeWorldState(HomeTransform);
	UE_LOG(LogOperationMouse, Log, TEXT("[Carry][Reset] Target=%s Authority=Server"), *GetName());
}

void AOMCarryableActor::OnRep_CurrentHolder(AOMMouseCharacter* PreviousHolder)
{
	ReconcileReplicatedPresentation();

	UE_LOG(
		LogOperationMouse,
		Log,
		TEXT("[Carry][Replicated] Target=%s PreviousHolder=%s CurrentHolder=%s"),
		*GetName(),
		*GetNameSafe(PreviousHolder),
		*GetNameSafe(CurrentHolder));
}

void AOMCarryableActor::OnRep_WorldStateRevision()
{
	ReconcileReplicatedPresentation();
	UE_LOG(
		LogOperationMouse,
		Log,
		TEXT("[Carry][WorldStateReplicated] Target=%s Revision=%u Holder=%s Location=%s"),
		*GetName(),
		WorldStateRevision,
		*GetNameSafe(CurrentHolder),
		*AuthoritativeWorldTransform.GetLocation().ToCompactString());
}

void AOMCarryableActor::SaveWorldStateIfNeeded()
{
	if (bHasSavedWorldState || !Mesh)
	{
		return;
	}

	SavedCollisionEnabled = Mesh->GetCollisionEnabled();
	SavedPawnCollisionResponse = Mesh->GetCollisionResponseToChannel(ECC_Pawn);
	bSavedSimulatePhysics = Mesh->IsSimulatingPhysics();
	bHasSavedWorldState = true;
}

void AOMCarryableActor::ApplyCarryPresentation(USceneComponent* NewCarryPoint)
{
	if (!IsValid(NewCarryPoint) || !Mesh)
	{
		return;
	}

	SaveWorldStateIfNeeded();

	ClearHolderCollisionIgnores();
	Mesh->SetSimulatePhysics(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	ApplyHolderCollisionIgnores(Cast<AOMMouseCharacter>(NewCarryPoint->GetOwner()));
	if (HasAuthority())
	{
		UpdateCarriedTransform();
	}
	else
	{
		ActivateClientCarryPresentation(NewCarryPoint);
	}
	UpdateStatusText();
}

FTransform AOMCarryableActor::BuildCarryTargetTransform(USceneComponent* CarryPoint) const
{
	FTransform TargetTransform = GetActorTransform();
	if (!IsValid(CarryPoint) || !Mesh)
	{
		return TargetTransform;
	}

	const AOMMouseCharacter* Holder = Cast<AOMMouseCharacter>(CarryPoint->GetOwner());
	if (!IsValid(Holder))
	{
		return TargetTransform;
	}

	const UCapsuleComponent* Capsule = Holder->GetCapsuleComponent();
	const float CapsuleRadius = IsValid(Capsule) ? Capsule->GetScaledCapsuleRadius() : 0.0f;
	const float ObjectRadius = Mesh->Bounds.SphereRadius;
	const float SafeCenterDistance = FMath::Max(
		MinimumCarryDistance,
		CapsuleRadius + ObjectRadius + CarryClearance);
	const float CarryPointForwardDistance = FVector::DotProduct(
		CarryPoint->GetComponentLocation() - Holder->GetActorLocation(),
		Holder->GetActorForwardVector());

	FVector LocalOffset = CarryOffset;
	LocalOffset.X += SafeCenterDistance - CarryPointForwardDistance;
	TargetTransform.SetLocation(
		CarryPoint->GetComponentLocation()
		+ CarryPoint->GetComponentQuat().RotateVector(LocalOffset));
	TargetTransform.SetRotation(CarryPoint->GetComponentQuat());
	return TargetTransform;
}

void AOMCarryableActor::UpdateCarriedTransform()
{
	if (!HasAuthority() || !IsValid(CurrentHolder) || !Mesh)
	{
		return;
	}

	UOMCarryComponent* HolderCarryComponent = CurrentHolder->FindComponentByClass<UOMCarryComponent>();
	USceneComponent* CarryPoint = HolderCarryComponent ? HolderCarryComponent->GetCarryPoint() : nullptr;
	if (!IsValid(CarryPoint))
	{
		return;
	}

	FHitResult Hit;
	const FTransform TargetTransform = BuildCarryTargetTransform(CarryPoint);
	SetActorLocationAndRotation(
		TargetTransform.GetLocation(),
		TargetTransform.GetRotation(),
		true,
		&Hit,
		ETeleportType::None);
	SetCarryObstructed(Hit.bBlockingHit, Hit);
}

void AOMCarryableActor::InitializeClientVisualMesh()
{
	if (!Mesh || !ClientVisualMesh)
	{
		return;
	}

	ClientVisualMesh->SetStaticMesh(Mesh->GetStaticMesh());
	ClientVisualMesh->SetRelativeScale3D(FVector::OneVector);
	for (int32 MaterialIndex = 0; MaterialIndex < Mesh->GetNumMaterials(); ++MaterialIndex)
	{
		ClientVisualMesh->SetMaterial(MaterialIndex, Mesh->GetMaterial(MaterialIndex));
	}
	ClientVisualMesh->SetVisibility(false);
	ClientVisualMesh->SetHiddenInGame(true);
}

void AOMCarryableActor::ActivateClientCarryPresentation(USceneComponent* CarryPoint)
{
	if (HasAuthority() || !Mesh || !ClientVisualMesh || !IsValid(CarryPoint))
	{
		return;
	}

	InitializeClientVisualMesh();
	if (!bClientCarryPresentationActive)
	{
		bAuthoritativeMeshWasVisible = Mesh->IsVisible();
		bAuthoritativeMeshWasHiddenInGame = Mesh->bHiddenInGame;
		ClientCarryStartWorldStateRevision = WorldStateRevision;
		ClientVisualMesh->SetWorldLocationAndRotation(Mesh->GetComponentLocation(), Mesh->GetComponentQuat());
		bClientCarryPresentationActive = true;
	}

	if (UCharacterMovementComponent* Movement = IsValid(CurrentHolder)
		? CurrentHolder->GetCharacterMovement()
		: nullptr)
	{
		if (ClientPresentationTickPrerequisite.Get() != Movement)
		{
			if (UCharacterMovementComponent* PreviousMovement = ClientPresentationTickPrerequisite.Get())
			{
				RemoveTickPrerequisiteComponent(PreviousMovement);
			}
			AddTickPrerequisiteComponent(Movement);
			ClientPresentationTickPrerequisite = Movement;
		}
	}

	Mesh->SetVisibility(false, false);
	Mesh->SetHiddenInGame(true, false);
	ClientVisualMesh->SetVisibility(true);
	ClientVisualMesh->SetHiddenInGame(false);
	SetActorTickEnabled(true);
}

void AOMCarryableActor::UpdateClientCarryPresentation(float DeltaSeconds)
{
	if (!bClientCarryPresentationActive || !ClientVisualMesh || !IsValid(CurrentHolder))
	{
		return;
	}

	const UOMCarryComponent* HolderCarryComponent = CurrentHolder->FindComponentByClass<UOMCarryComponent>();
	USceneComponent* CarryPoint = HolderCarryComponent ? HolderCarryComponent->GetCarryPoint() : nullptr;
	if (!IsValid(CarryPoint))
	{
		return;
	}

	const FTransform LocalTarget = BuildCarryTargetTransform(CarryPoint);
	const FVector AuthoritativeLocation = GetActorLocation();
	FVector VisualOffset = LocalTarget.GetLocation() - AuthoritativeLocation;
	const FVector ObstructionNormal = FVector(CarryObstructionNormal).GetSafeNormal();
	if (!ObstructionNormal.IsNearlyZero() && FVector::DotProduct(VisualOffset, ObstructionNormal) < 0.0f)
	{
		VisualOffset = FVector::VectorPlaneProject(VisualOffset, ObstructionNormal);
	}
	VisualOffset = VisualOffset.GetClampedToMaxSize(ClientVisualMaxOffset);
	const FVector VisualTargetLocation = AuthoritativeLocation + VisualOffset;

	FVector CurrentVisualLocation = ClientVisualMesh->GetComponentLocation();
	FQuat CurrentVisualRotation = ClientVisualMesh->GetComponentQuat();
	if (FVector::DistSquared(CurrentVisualLocation, AuthoritativeLocation)
		> FMath::Square(ClientVisualHardCorrectionDistance))
	{
		CurrentVisualLocation = AuthoritativeLocation;
		CurrentVisualRotation = GetActorQuat();
	}

	ClientVisualMesh->SetWorldLocationAndRotation(
		FMath::VInterpTo(CurrentVisualLocation, VisualTargetLocation, DeltaSeconds, ClientVisualSmoothingSpeed),
		FMath::QInterpTo(CurrentVisualRotation, LocalTarget.GetRotation(), DeltaSeconds, ClientVisualSmoothingSpeed));
}

void AOMCarryableActor::DeactivateClientCarryPresentation()
{
	if (!bClientCarryPresentationActive)
	{
		return;
	}

	if (UCharacterMovementComponent* Movement = ClientPresentationTickPrerequisite.Get())
	{
		RemoveTickPrerequisiteComponent(Movement);
	}
	ClientPresentationTickPrerequisite = nullptr;

	if (ClientVisualMesh)
	{
		ClientVisualMesh->SetVisibility(false);
		ClientVisualMesh->SetHiddenInGame(true);
	}
	if (Mesh)
	{
		Mesh->SetVisibility(bAuthoritativeMeshWasVisible, false);
		Mesh->SetHiddenInGame(bAuthoritativeMeshWasHiddenInGame, false);
	}

	bClientCarryPresentationActive = false;
	SetActorTickEnabled(false);
}

void AOMCarryableActor::ApplyHolderCollisionIgnores(AOMMouseCharacter* Holder)
{
	if (!IsValid(Holder) || !Mesh)
	{
		return;
	}

	CollisionIgnoredHolder = Holder;
	if (!Mesh->GetMoveIgnoreActors().Contains(Holder))
	{
		Mesh->IgnoreActorWhenMoving(Holder, true);
		bAddedMeshIgnoreForHolder = true;
	}
	if (UCapsuleComponent* Capsule = Holder->GetCapsuleComponent();
		IsValid(Capsule) && !Capsule->GetMoveIgnoreActors().Contains(this))
	{
		Holder->MoveIgnoreActorAdd(this);
		bAddedHolderIgnoreForCargo = true;
	}
}

void AOMCarryableActor::ClearHolderCollisionIgnores()
{
	AOMMouseCharacter* Holder = CollisionIgnoredHolder.Get();
	if (IsValid(Holder) && Mesh && bAddedMeshIgnoreForHolder)
	{
		Mesh->IgnoreActorWhenMoving(Holder, false);
	}
	if (IsValid(Holder) && bAddedHolderIgnoreForCargo)
	{
		Holder->MoveIgnoreActorRemove(this);
	}
	CollisionIgnoredHolder = nullptr;
	bAddedMeshIgnoreForHolder = false;
	bAddedHolderIgnoreForCargo = false;
	bCarryObstructed = false;
	CarryObstructionNormal = FVector::ZeroVector;
}

void AOMCarryableActor::SetCarryObstructed(bool bNewObstructed, const FHitResult& Hit)
{
	const FVector NewNormal = bNewObstructed ? Hit.ImpactNormal.GetSafeNormal() : FVector::ZeroVector;
	if (bCarryObstructed == bNewObstructed && FVector(CarryObstructionNormal).Equals(NewNormal, 0.01f))
	{
		return;
	}

	bCarryObstructed = bNewObstructed;
	CarryObstructionNormal = NewNormal;
	const UCharacterMovementComponent* Movement = IsValid(CurrentHolder)
		? CurrentHolder->GetCharacterMovement()
		: nullptr;
	UE_LOG(LogOperationMouse, Log,
		TEXT("[Carry][Obstruction] Target=%s State=%s Hit=%s Role=%s Holder=%s MovementMode=%d Normal=%s"),
		*GetName(), bCarryObstructed ? TEXT("Blocked") : TEXT("Clear"), *GetNameSafe(Hit.GetActor()),
		*UEnum::GetValueAsString(GetLocalRole()), *GetNameSafe(CurrentHolder),
		Movement ? static_cast<int32>(Movement->MovementMode) : -1, *NewNormal.ToCompactString());
	ForceNetUpdate();
}

void AOMCarryableActor::ApplyReplicatedWorldPresentation()
{
	if (!Mesh)
	{
		return;
	}

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	ClearHolderCollisionIgnores();
	Mesh->SetCollisionEnabled(SavedCollisionEnabled);
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, SavedPawnCollisionResponse);
	SetActorTransform(AuthoritativeWorldTransform, false, nullptr, ETeleportType::TeleportPhysics);
	DeactivateClientCarryPresentation();
	bHasSavedWorldState = false;
	UpdateStatusText();
}

void AOMCarryableActor::ReconcileReplicatedPresentation()
{
	if (IsValid(CurrentHolder))
	{
		UOMCarryComponent* HolderCarryComponent = CurrentHolder->FindComponentByClass<UOMCarryComponent>();
		if (HolderCarryComponent && IsValid(HolderCarryComponent->GetCarryPoint()))
		{
			ApplyCarryPresentation(HolderCarryComponent->GetCarryPoint());
			return;
		}

		UE_LOG(
			LogOperationMouse,
			Warning,
			TEXT("[Carry][Replicated] Target=%s Holder=%s Result=PendingPresentation Reason=MissingCarryPoint"),
			*GetName(),
			*GetNameSafe(CurrentHolder));
		return;
	}

	if (!HasAuthority() && bClientCarryPresentationActive
		&& WorldStateRevision == ClientCarryStartWorldStateRevision)
	{
		return;
	}

	ApplyReplicatedWorldPresentation();
}

void AOMCarryableActor::PublishAuthoritativeWorldState(const FTransform& TargetTransform)
{
	if (!HasAuthority())
	{
		return;
	}

	RestoreWorldState(TargetTransform);
	AuthoritativeWorldTransform = GetActorTransform();
	++WorldStateRevision;
	ForceNetUpdate();
}

void AOMCarryableActor::RestoreWorldState(const FTransform& TargetTransform)
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	ClearHolderCollisionIgnores();
	if (Mesh->IsSimulatingPhysics())
	{
		Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
		Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	}
	Mesh->SetSimulatePhysics(false);
	SetActorTransform(TargetTransform, false, nullptr, ETeleportType::TeleportPhysics);
	Mesh->SetCollisionEnabled(SavedCollisionEnabled);
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, SavedPawnCollisionResponse);
	Mesh->SetSimulatePhysics(bSavedSimulatePhysics);
	bHasSavedWorldState = false;
	UpdateStatusText();
}

void AOMCarryableActor::UpdateStatusText()
{
	if (!StatusText)
	{
		return;
	}

	const bool bCarried = IsValid(CurrentHolder);
	StatusText->SetText(bCarried
		? NSLOCTEXT("OperationMouse", "CarryableCarriedState", "CARRYING")
		: NSLOCTEXT("OperationMouse", "CarryableReadyState", "CARRYABLE: READY"));
	StatusText->SetTextRenderColor(bCarried ? FColor::Yellow : FColor::Cyan);
}

void AOMCarryableActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AOMCarryableActor, CurrentHolder);
	DOREPLIFETIME(AOMCarryableActor, AuthoritativeWorldTransform);
	DOREPLIFETIME(AOMCarryableActor, WorldStateRevision);
	DOREPLIFETIME(AOMCarryableActor, CarryObstructionNormal);
}
