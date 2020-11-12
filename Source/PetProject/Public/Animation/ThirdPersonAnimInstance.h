// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/RootMotionSource.h"
#include "ThirdPersonAnimInstance.generated.h"

UENUM()
enum EStrafeType
{
	Invalid		UMETA(DisplayName = "Invalid"),
	StrafeForward		UMETA(DisplayName = "StrafeForward"),
	StrafeBackward	UMETA(DisplayName = "StrafeBackward")
};

struct FRootMotionSource;
/**
 * 
 */
UCLASS()
class PETPROJECT_API UThirdPersonAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	UThirdPersonAnimInstance(const FObjectInitializer& objectInitializer);

	virtual void NativeInitializeAnimation() override;

	virtual void NativeUpdateAnimation(float deltaSeconds) override;

	virtual void NativePostEvaluateAnimation() override;

	bool IsStrafing() const { return StrafeType != EStrafeType::Invalid; }

	float GetClampedJogSpeed(bool isRunning);

	// function that usually called before you want to let animation take over owner transform
	UFUNCTION(BlueprintCallable)
	void UpdateMotionDetailStatus();
	   
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector LastStandingLocation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector ForwardDirection;
		
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector Acceleration;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector Velocity;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector LeftHandEffectorLocation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector NextLeftHandEffectorLocation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector LeftHandJointTargetLocation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector RightHandEffectorLocation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector NextRightHandEffectorLocation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector RightHandJointTargetLocation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TEnumAsByte<EStrafeType> StrafeType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MaxSpeed = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float AimingYaw = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float AimingVSVelocityYaw = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float VelocityYaw = 0.f;

	// angle between the owner facing direction and desired moving direction (acceleration) in degree
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float AccelerationYaw = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TurnToAimingLocationRate = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LeftHandIKWeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RightHandIKWeight = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float StandingLocationZOffset = 0.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool IsOnGround = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool IsCrouched = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool IsAiming = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool IsHanging = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool ShouldUpdateMotionDetailStatus = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName NextFootMarkerName;

	// this is the state before evaluation, so it's like a previous motion state to anim bp
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName CurrentMotionState = FName("Idle State");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName PreviousMotionState = FName("Idle State");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UPPCharacterMovementComponent* PPCharacterMovement;

	FAnimNode_StateMachine* MotionStateMachine = nullptr;

private:
	static float CalculateYaw(const FVector& va, const FVector& vb);
};
