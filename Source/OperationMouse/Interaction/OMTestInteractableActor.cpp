#include "OMTestInteractableActor.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AOMTestInteractableActor::AOMTestInteractableActor()
{
	bReplicates = true;
	SetReplicateMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(SceneRoot);
	Mesh->SetRelativeScale3D(FVector(0.75f, 0.75f, 0.75f));
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}

	StatusText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StatusText"));
	StatusText->SetupAttachment(SceneRoot);
	StatusText->SetRelativeLocation(FVector(0.0f, 0.0f, 105.0f));
	StatusText->SetHorizontalAlignment(EHTA_Center);
	StatusText->SetWorldSize(24.0f);

	StatusLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StatusLight"));
	StatusLight->SetupAttachment(SceneRoot);
	StatusLight->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
	StatusLight->SetIntensity(1800.0f);
	StatusLight->SetAttenuationRadius(220.0f);

	InteractionInfo.Prompt = NSLOCTEXT("OperationMouse", "TestInteractPrompt", "Activate");
	InteractionInfo.Type = EOMInteractionType::Instant;
	InteractionInfo.HoldDuration = 2.0f;
	InteractionInfo.MaximumDistance = 200.0f;
	InteractionInfo.bExclusive = true;
}

void AOMTestInteractableActor::BeginPlay()
{
	Super::BeginPlay();
	UpdateVisualState();
}

FOMInteractionInfo AOMTestInteractableActor::GetInteractionInfo_Implementation(AActor* Interactor) const
{
	return InteractionInfo;
}

FVector AOMTestInteractableActor::GetInteractionPoint_Implementation() const
{
	return GetActorLocation() + FVector::UpVector * 45.0f;
}

bool AOMTestInteractableActor::CanInteract_Implementation(AActor* Interactor) const
{
	if (!Interactor || bActivated)
	{
		return false;
	}

	return !InteractionInfo.bExclusive || !ActiveInteractor || ActiveInteractor == Interactor;
}

bool AOMTestInteractableActor::BeginInteraction_Implementation(AActor* Interactor)
{
	if (!HasAuthority() || !CanInteract_Implementation(Interactor))
	{
		return false;
	}

	if (InteractionInfo.bExclusive)
	{
		ActiveInteractor = Interactor;
		UpdateVisualState();
		ForceNetUpdate();
	}

	return true;
}

void AOMTestInteractableActor::CancelInteraction_Implementation(AActor* Interactor)
{
	if (!HasAuthority() || ActiveInteractor != Interactor || bActivated)
	{
		return;
	}

	ActiveInteractor = nullptr;
	UpdateVisualState();
	ForceNetUpdate();
}

void AOMTestInteractableActor::CompleteInteraction_Implementation(AActor* Interactor)
{
	if (!HasAuthority() || bActivated || (InteractionInfo.bExclusive && ActiveInteractor != Interactor))
	{
		return;
	}

	bActivated = true;
	ActiveInteractor = nullptr;
	++CompletionCount;
	UpdateVisualState();
	ForceNetUpdate();
}

void AOMTestInteractableActor::OnRep_InteractionState()
{
	UpdateVisualState();
}

void AOMTestInteractableActor::UpdateVisualState()
{
	if (bActivated)
	{
		StatusText->SetText(NSLOCTEXT("OperationMouse", "InteractionActivatedState", "ACTIVATED"));
		StatusText->SetTextRenderColor(FColor::Green);
		StatusLight->SetLightColor(FLinearColor::Green);
		Mesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 25.0f));
	}
	else if (ActiveInteractor)
	{
		StatusText->SetText(NSLOCTEXT("OperationMouse", "InteractionInUseState", "IN USE"));
		StatusText->SetTextRenderColor(FColor::Yellow);
		StatusLight->SetLightColor(FLinearColor::Yellow);
		Mesh->SetRelativeRotation(FRotator::ZeroRotator);
	}
	else
	{
		StatusText->SetText(NSLOCTEXT("OperationMouse", "InteractionReadyState", "READY"));
		StatusText->SetTextRenderColor(FColor::Cyan);
		StatusLight->SetLightColor(FLinearColor(0.0f, 0.5f, 1.0f));
		Mesh->SetRelativeRotation(FRotator::ZeroRotator);
	}
}

void AOMTestInteractableActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AOMTestInteractableActor, bActivated);
	DOREPLIFETIME(AOMTestInteractableActor, ActiveInteractor);
	DOREPLIFETIME(AOMTestInteractableActor, CompletionCount);
}
