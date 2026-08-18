#include "OMCarryableActor.h"

#include "OMCarryComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "../Characters/OMMouseCharacter.h"
#include "../OperationMouse.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AOMCarryableActor::AOMCarryableActor()
{
	bReplicates = true;
	SetReplicateMovement(true);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	Mesh->SetSimulatePhysics(true);
	Mesh->SetRelativeScale3D(FVector(0.45f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
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
	UpdateStatusText();
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
	ForceNetUpdate();
	return true;
}

bool AOMCarryableActor::EndCarry(UOMCarryComponent* RequestingCarrier, const FVector& DropLocation)
{
	if (!HasAuthority() || !IsValid(RequestingCarrier) || CurrentHolder != RequestingCarrier->GetOwner())
	{
		return false;
	}

	CurrentHolder = nullptr;
	FTransform DropTransform = GetActorTransform();
	DropTransform.SetLocation(DropLocation);
	DropTransform.SetRotation(FQuat::Identity);
	RestoreWorldState(DropTransform);
	ForceNetUpdate();
	return true;
}

bool AOMCarryableActor::IsAvailableForGrab() const
{
	return IsValid(this) && !IsActorBeingDestroyed() && !IsValid(CurrentHolder);
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

	CurrentHolder = nullptr;
	RestoreWorldState(HomeTransform);
	ForceNetUpdate();
	UE_LOG(LogOperationMouse, Log, TEXT("[Carry][Reset] Target=%s Authority=Server"), *GetName());
}

void AOMCarryableActor::OnRep_CurrentHolder(AOMMouseCharacter* PreviousHolder)
{
	if (IsValid(CurrentHolder))
	{
		UOMCarryComponent* HolderCarryComponent = CurrentHolder->FindComponentByClass<UOMCarryComponent>();
		if (HolderCarryComponent && IsValid(HolderCarryComponent->GetCarryPoint()))
		{
			ApplyCarryPresentation(HolderCarryComponent->GetCarryPoint());
		}
		else
		{
			UE_LOG(
				LogOperationMouse,
				Warning,
				TEXT("[Carry][Replicated] Target=%s Holder=%s Result=PendingPresentation Reason=MissingCarryPoint"),
				*GetName(),
				*GetNameSafe(CurrentHolder));
		}
	}
	else
	{
		ApplyDroppedPresentation();
	}

	UE_LOG(
		LogOperationMouse,
		Log,
		TEXT("[Carry][Replicated] Target=%s PreviousHolder=%s CurrentHolder=%s"),
		*GetName(),
		*GetNameSafe(PreviousHolder),
		*GetNameSafe(CurrentHolder));
}

void AOMCarryableActor::SaveWorldStateIfNeeded()
{
	if (bHasSavedWorldState || !Mesh)
	{
		return;
	}

	SavedCollisionEnabled = Mesh->GetCollisionEnabled();
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
	Mesh->SetSimulatePhysics(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttachToComponent(NewCarryPoint, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	SetActorRelativeLocation(FVector::ZeroVector);
	SetActorRelativeRotation(FRotator::ZeroRotator);
	UpdateStatusText();
}

void AOMCarryableActor::ApplyDroppedPresentation()
{
	if (!Mesh)
	{
		return;
	}

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	Mesh->SetSimulatePhysics(false);
	Mesh->SetCollisionEnabled(SavedCollisionEnabled);
	Mesh->SetSimulatePhysics(bSavedSimulatePhysics);
	bHasSavedWorldState = false;
	UpdateStatusText();
}

void AOMCarryableActor::RestoreWorldState(const FTransform& TargetTransform)
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	Mesh->SetSimulatePhysics(false);
	SetActorTransform(TargetTransform, false, nullptr, ETeleportType::TeleportPhysics);
	Mesh->SetCollisionEnabled(SavedCollisionEnabled);
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
}
