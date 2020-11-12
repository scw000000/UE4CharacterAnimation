// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PPCharacterMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class PETPROJECT_API UPPCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
public:
	UPPCharacterMovementComponent(const FObjectInitializer& objectInitializer);

	UFUNCTION(BlueprintCallable)
	void StartApplyAnimCurveRootMotionSource();

	UFUNCTION(BlueprintCallable)
	void StopApplyRootMotionSource();

	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
	bool IsApplyingRotMotion() const { return RootMotionSourceID != (uint16)ERootMotionSourceID::Invalid; }
	
	void StartApplyRootMotionSource(FRootMotionSource* newMotionSource);

	uint16 GetRootMotionSourceID() const { return RootMotionSourceID; }
	/*void StartApplyFloatCurveRootMotionSource(
		const TArray<FFloatCurve*, TFixedAllocator<3>>& tranlationCurves, 
		const FVector2D& deltaCorrectionWindow,
		const FTransform& deltaCorrectionTransform);*/
	// todo: set max speed to PredefinedMaxRunSpeed if it's walking to running
	// decrease speed overtime till PredefinedMaxWalkSpeed otherwise

	/*UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1000.0", UIMin = "0.0", UIMax = "1000.0"))
	float PredefinedMaxWalkSpeed = 160.f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1000.0", UIMin = "0.0", UIMax = "1000.0"))
	float PredefinedMaxRunSpeed = 645.f;*/

	/*void SetupWalkMode();

	void SetupRunMode();*/
protected:
	uint16 RootMotionSourceID = (uint16)ERootMotionSourceID::Invalid;

	virtual void PerformMovement(float DeltaTime) override;
};

USTRUCT()
struct PETPROJECT_API FRootMotionSource_AnimationCurve : public FRootMotionSource
{
	GENERATED_USTRUCT_BODY()

	FRootMotionSource_AnimationCurve() = default;

	virtual ~FRootMotionSource_AnimationCurve() {}

	//UPROPERTY()
	//	FVector Force;

	//UPROPERTY()
	//	UCurveFloat* StrengthOverTime;

	//virtual FRootMotionSource* Clone() const override;

	//virtual bool Matches(const FRootMotionSource* Other) const override;

	//virtual bool MatchesAndHasSameState(const FRootMotionSource* Other) const override;

	virtual void PrepareRootMotion(
		float simulationTime,
		float movementTickTime,
		const ACharacter& character,
		const UCharacterMovementComponent& moveComponent
	) override;

	/*virtual UScriptStruct* GetScriptStruct() const override;

	virtual FString ToSimpleString() const override;
*/
	
	FTransform CachedMeshComponentTransform;
	FVector PreviousTranslation = FVector::ZeroVector;

	FQuat PreviousRotation = FQuat::Identity;
	
	/*virtual void AddReferencedObjects(class FReferenceCollector& Collector) override;*/
};

USTRUCT()
struct PETPROJECT_API FRootMotionSource_FloatCurve : public FRootMotionSource
{
	GENERATED_USTRUCT_BODY()

	FRootMotionSource_FloatCurve() = default;

	virtual void PrepareRootMotion(
		float simulationTime,
		float movementTickTime,
		const ACharacter& character,
		const UCharacterMovementComponent& moveComponent
	) override;

	FTransform InitialTransform;
	TArray<FFloatCurve*, TFixedAllocator<3>> TranslationCurves;
	// for applying delta transform
	FTransform DeltaCorrectionTransform;

	FVector2D DeltaCorrectionWindow;
};
