#pragma once

#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "OMAnimInstanceProxy.generated.h"

class UOMAnimInstance;

/** Small native pose graph used by the prototype Animation Blueprint. */
USTRUCT()
struct FOMAnimInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

	FOMAnimInstanceProxy() = default;
	explicit FOMAnimInstanceProxy(UAnimInstance* InAnimInstance);

	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual void Initialize(UAnimInstance* InAnimInstance) override;
	virtual void UpdateAnimationNode(const FAnimationUpdateContext& InContext) override;
	virtual bool Evaluate(FPoseContext& Output) override;
	virtual FAnimNode_Base* GetCustomRootNode() override;
	virtual void GetCustomNodes(TArray<FAnimNode_Base*>& OutNodes) override;

private:
	void SelectAnimations(const UOMAnimInstance& Instance);
	void ConfigurePlayer(FAnimNode_SequencePlayer_Standalone& Player, UAnimSequenceBase* Sequence, bool bLoop, float PlayRate);

	UPROPERTY(Transient)
	FAnimNode_SequencePlayer_Standalone PrimaryPlayer;

	UPROPERTY(Transient)
	FAnimNode_SequencePlayer_Standalone SecondaryPlayer;

	float PrimaryWeight = 1.0f;
};
