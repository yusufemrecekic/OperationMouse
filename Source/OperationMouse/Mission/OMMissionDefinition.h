#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "OMMissionDefinition.generated.h"

/** Small designer-facing definition shared by reusable mission instances. */
UCLASS(BlueprintType)
class OPERATIONMOUSE_API UOMMissionDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Operation Mouse|Mission")
	FName MissionId = TEXT("Mission_Prototype");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Operation Mouse|Mission", meta = (ClampMin = "1"))
	int32 ObjectiveTarget = 1;
};
