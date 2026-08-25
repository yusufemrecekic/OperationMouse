#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OMCloseSpaceCameraComponent.generated.h"

class ACharacter;
class USpringArmComponent;

/**
 * Local-only camera obstruction resolver for mouse-scale third-person spaces.
 * It separates the desired open-space arm from the collision-limited arm and
 * leaves server gameplay, replication, and camera ownership untouched.
 */
UCLASS(ClassGroup = (Camera), meta = (BlueprintSpawnableComponent))
class OPERATIONMOUSE_API UOMCloseSpaceCameraComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOMCloseSpaceCameraComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Enables the custom local resolver. Disabled by default to preserve existing production camera behavior. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Camera Foundation")
	bool bCloseSpaceCameraEnabled = false;

	/** Unobstructed third-person distance; calibration value, not final production tuning. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Camera Foundation", meta = (ClampMin = "1.0"))
	float DesiredArmLength = 170.0f;

	/** Radius used by the local camera-channel sphere sweep. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Camera Foundation", meta = (ClampMin = "0.1"))
	float CameraProbeRadius = 5.0f;

	/** Extra clearance kept behind a sweep contact so the perspective near plane cannot expose geometry beyond thin wall edges. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Camera Foundation", meta = (ClampMin = "0.0"))
	float CameraCollisionPadding = 2.0f;

	/** Interpolation rate used while obstruction shortens the desired arm. Collision safety remains a hard ceiling. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Camera Foundation", meta = (ClampMin = "0.0"))
	float CameraRetractSpeed = 30.0f;

	/** Slower interpolation rate used when returning to open-space distance. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Camera Foundation", meta = (ClampMin = "0.0"))
	float CameraExtendSpeed = 5.0f;

	/** Minimum readable distance equals capsule radius multiplied by this scale-resilient factor. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Camera Foundation", meta = (ClampMin = "1.0"))
	float MinSafeDistanceRadiusMultiplier = 2.0f;

	/** Compression ratio below which the pivot begins moving upward. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Camera Foundation", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float CloseSpaceThreshold = 0.55f;

	/** Maximum extra pivot height equals capsule half-height multiplied by this factor. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Camera Foundation", meta = (ClampMin = "0.0"))
	float CloseSpaceVerticalOffsetHalfHeightMultiplier = 1.0f;

	/** Frame-rate-independent blend rate for the compression-driven pivot offset. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Camera Foundation", meta = (ClampMin = "0.0"))
	float CloseSpaceBlendSpeed = 8.0f;

	/** Open-space camera pivot offset in world axes. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Camera Foundation")
	FVector OpenSpaceTargetOffset = FVector(0.0f, 0.0f, 18.0f);

private:
	void InitializeLocalResolver();
	float FindObstructionLimit(const FVector& Pivot, const FRotator& CameraRotation) const;
	void SetOwnerMeshFallback(bool bShouldHide);

	TObjectPtr<ACharacter> CharacterOwner;
	TObjectPtr<USpringArmComponent> CameraBoom;
	float CurrentResolvedDistance = 0.0f;
	float CurrentCloseSpaceAlpha = 0.0f;
	bool bOriginalSpringArmCollision = true;
	bool bOriginalOwnerNoSee = false;
	bool bResolverInitialized = false;
	bool bOwnerMeshFallbackActive = false;
};
