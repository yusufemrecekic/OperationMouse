#include "OMCarryableActor.h"

#include "OMCarryComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "../Characters/OMMouseCharacter.h"
#include "../OperationMouse.h"
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
	return CanInteract_Implementation(Interactor);
}

void AOMCarryableActor::CompleteInteraction_Implementation(AActor* Interactor)
{
	AOMMouseCharacter* Character = Cast<AOMMouseCharacter>(Interactor);
	UOMCarryComponent* CarryComponent = Character ? Character->FindComponentByClass<UOMCarryComponent>() : nullptr;
	if (!CarryComponent || !CarryComponent->TryGrab(this))
	{
		UE_LOG(
			LogOperationMouse,
			Log,
			TEXT("[Carry][Rejected] Carrier=%s Target=%s Reason=GameplayHandoffRejected Authority=PENDING"),
			*GetNameSafe(Interactor),
			*GetName());
	}
}

bool AOMCarryableActor::BeginCarry(UOMCarryComponent* NewCarrier, USceneComponent* NewCarryPoint)
{
	if (!IsAvailableForGrab() || !IsValid(NewCarrier) || !IsValid(NewCarryPoint))
	{
		return false;
	}

	CurrentCarrier = NewCarrier;
	SavedCollisionEnabled = Mesh->GetCollisionEnabled();
	bSavedSimulatePhysics = Mesh->IsSimulatingPhysics();
	Mesh->SetSimulatePhysics(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttachToComponent(NewCarryPoint, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	SetActorRelativeLocation(FVector::ZeroVector);
	SetActorRelativeRotation(FRotator::ZeroRotator);
	UpdateStatusText();
	return true;
}

void AOMCarryableActor::EndCarry(UOMCarryComponent* RequestingCarrier, const FVector& DropLocation)
{
	if (CurrentCarrier != RequestingCarrier)
	{
		return;
	}

	CurrentCarrier = nullptr;
	FTransform DropTransform = GetActorTransform();
	DropTransform.SetLocation(DropLocation);
	DropTransform.SetRotation(FQuat::Identity);
	RestoreWorldState(DropTransform);
}

bool AOMCarryableActor::IsAvailableForGrab() const
{
	return IsValid(this) && !IsActorBeingDestroyed() && !IsValid(CurrentCarrier);
}

void AOMCarryableActor::ResetToHome()
{
	if (IsValid(CurrentCarrier))
	{
		CurrentCarrier->ReleaseForRecovery(this);
	}

	CurrentCarrier = nullptr;
	RestoreWorldState(HomeTransform);
	UE_LOG(LogOperationMouse, Log, TEXT("[Carry][Reset] Target=%s Authority=PENDING"), *GetName());
}

void AOMCarryableActor::RestoreWorldState(const FTransform& TargetTransform)
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	Mesh->SetSimulatePhysics(false);
	SetActorTransform(TargetTransform, false, nullptr, ETeleportType::TeleportPhysics);
	Mesh->SetCollisionEnabled(SavedCollisionEnabled);
	Mesh->SetSimulatePhysics(bSavedSimulatePhysics);
	UpdateStatusText();
}

void AOMCarryableActor::UpdateStatusText()
{
	if (!StatusText)
	{
		return;
	}

	const bool bCarried = IsValid(CurrentCarrier);
	StatusText->SetText(bCarried
		? NSLOCTEXT("OperationMouse", "CarryableCarriedState", "CARRYING")
		: NSLOCTEXT("OperationMouse", "CarryableReadyState", "CARRYABLE: READY"));
	StatusText->SetTextRenderColor(bCarried ? FColor::Yellow : FColor::Cyan);
}
