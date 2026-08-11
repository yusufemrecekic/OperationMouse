#include "OMMouseCharacter.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "../OperationMouse.h"
#include "UObject/ConstructorHelpers.h"

AOMMouseCharacter::AOMMouseCharacter()
{
	bReplicates = true;
	SetReplicateMovement(true);

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
