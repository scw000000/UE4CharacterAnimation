// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_HandIKWeightModifier.generated.h"

/**
 * 
 */
UCLASS()
class PETPROJECT_API UAnimNotifyState_HandIKWeightModifier : public UAnimNotifyState
{
	GENERATED_BODY()
public:
	
	virtual void NotifyBegin(class USkeletalMeshComponent * meshComp, class UAnimSequenceBase * animation, float totalDuration) override;
	virtual void NotifyTick(class USkeletalMeshComponent * meshComp, class UAnimSequenceBase * animation, float frameDeltaTime) override;
	virtual void NotifyEnd(class USkeletalMeshComponent * meshComp, class UAnimSequenceBase * animation) override;

	UPROPERTY()
	float TotalDuration;

	UPROPERTY()
	float ElapsedTime;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float BeginIKWeight;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float EndIKWeight;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool ControlLeftHand;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool ControlRightHand;

	UPROPERTY()
	class UThirdPersonAnimInstance* AnimInstance;

	float UThirdPersonAnimInstance::*WeightToModify;
};
