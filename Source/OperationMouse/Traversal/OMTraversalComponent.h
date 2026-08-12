#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OMTraversalComponent.generated.h"

class ACharacter;

UENUM(BlueprintType)
enum class EOMMantleType : uint8
{
	Low,
	High
};

USTRUCT()
struct FOMReplicatedMantleState
{
	GENERATED_BODY()

	UPROPERTY()
	bool bIsActive = false;

	UPROPERTY()
	FVector_NetQuantize10 StartLocation = FVector::ZeroVector;

	UPROPERTY()
	FVector_NetQuantize10 TargetLocation = FVector::ZeroVector;

	UPROPERTY()
	float ServerStartTime = 0.0f;

	UPROPERTY()
	float Duration = 0.0f;

	UPROPERTY()
	EOMMantleType MantleType = EOMMantleType::Low;
};

/** Detects and performs short, server-validated mantle transitions on static geometry. */
UCLASS(ClassGroup = (OperationMouse), meta = (BlueprintSpawnableComponent))
class OPERATIONMOUSE_API UOMTraversalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOMTraversalComponent();

	/** Returns true when mantle consumed the input or a server request was sent. */
	bool TryMantle();

	bool IsMantling() const { return MantleState.bIsActive; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	struct FMantleCandidate
	{
		FVector TargetLocation = FVector::ZeroVector;
		float Height = 0.0f;
		EOMMantleType MantleType = EOMMantleType::Low;
	};

	bool CanStartMantle() const;
	bool FindMantleCandidate(FMantleCandidate& OutCandidate) const;
	void StartValidatedMantle(const FMantleCandidate& Candidate);
	void FinishMantle();
	void ApplyMantlePosition(float Alpha);
	float GetSynchronizedTimeSeconds() const;

	UFUNCTION(Server, Reliable)
	void ServerTryMantle();

	UFUNCTION(Client, Reliable)
	void ClientRejectMantle();

	UFUNCTION()
	void OnRep_MantleState();

	UPROPERTY(ReplicatedUsing = OnRep_MantleState)
	FOMReplicatedMantleState MantleState;

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> OwnerCharacter;

	/** Horizontal distance used to find a wall in front of the character. */
	UPROPERTY(EditAnywhere, Category = "Mantle|Detection", meta = (ClampMin = "1.0"))
	float ForwardDetectionDistance = 85.0f;

	UPROPERTY(EditAnywhere, Category = "Mantle|Detection", meta = (ClampMin = "1.0"))
	float MinimumMantleHeight = 45.0f;

	UPROPERTY(EditAnywhere, Category = "Mantle|Detection", meta = (ClampMin = "1.0"))
	float MaximumMantleHeight = 210.0f;

	/** Heights at or above this value use the high mantle profile. */
	UPROPERTY(EditAnywhere, Category = "Mantle|Detection", meta = (ClampMin = "1.0"))
	float HighMantleThreshold = 125.0f;

	/** Radius of the forward obstacle sweep. */
	UPROPERTY(EditAnywhere, Category = "Mantle|Detection", meta = (ClampMin = "1.0"))
	float ObstacleTraceRadius = 12.0f;

	/** How far beyond the detected wall face to search for its top surface. */
	UPROPERTY(EditAnywhere, Category = "Mantle|Detection", meta = (ClampMin = "0.0"))
	float LandingForwardOffset = 12.0f;

	/** Small vertical separation used by destination and path clearance checks. */
	UPROPERTY(EditAnywhere, Category = "Mantle|Detection", meta = (ClampMin = "0.0"))
	float ClearanceMargin = 3.0f;

	/** Minimum upward normal accepted as a walkable top surface. */
	UPROPERTY(EditAnywhere, Category = "Mantle|Detection", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinimumTopSurfaceNormalZ = 0.7f;

	UPROPERTY(EditAnywhere, Category = "Mantle|Transition", meta = (ClampMin = "0.05"))
	float LowMantleDuration = 0.28f;

	UPROPERTY(EditAnywhere, Category = "Mantle|Transition", meta = (ClampMin = "0.05"))
	float HighMantleDuration = 0.45f;

	UPROPERTY(EditAnywhere, Category = "Mantle|Debug")
	bool bDrawDebug = false;
};
