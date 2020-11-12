// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "Input/Reply.h"
#include "IPropertyTypeCustomization.h"
#include "GameFramework/Actor.h"
#include "GameFramework/RootMotionSource.h"
#include "ClimbNodeGraph.generated.h"

class UClimbNodeGraphRenderingComponent;
class UBillboardComponent;
class APPCharacter;

UCLASS(BlueprintType)
class PETPROJECT_API AClimbSegment : public AActor
{
	GENERATED_BODY()
public:
	AClimbSegment();

	virtual void OnConstruction(const FTransform& transform) override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	class USplineComponent* LedgeClimbingSpline;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	bool MovingRightIsForward;

	UPROPERTY()
	class USphereComponent* InteractionCollider;
	
	// for now assuming only one segment is connected
	UPROPERTY(EditAnywhere)
	AClimbSegment* FrontConnectedSegment;
	
	UPROPERTY(EditAnywhere)
	bool FrontConnectedToSegmentStart;

	UPROPERTY(EditAnywhere)
	AClimbSegment* RearConnectedSegment;

	UPROPERTY(EditAnywhere)
	bool RearConnectedToSegmentStart;

	UPROPERTY(EditAnywhere)
	bool Climbable;

	UPROPERTY(EditAnywhere)
	AClimbSegment* UpConnectedSegment;

	UPROPERTY(EditAnywhere)
	float EntryDistanceAlongSpline;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1000.0", UIMin = "0.0", UIMax = "1000.0"))
	float EntryRadius;

#if WITH_EDITOR
public:
	/*virtual void PostEditChangeProperty(struct FPropertyChangedEvent& propertyChangedEvent) override;

	virtual void PostEditMove(bool bFinished) override;*/

private:
	//UPROPERTY()
	//UBillboardComponent* BillboardComponent;
#endif // WITH_EDITORONLY_DATA

};

UCLASS()
class PETPROJECT_API AClimbNodeGraph : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AClimbNodeGraph();
	
	UFUNCTION(BlueprintCallable)
	void GenerateClimbSegment();
	
	void OnSegementConstruction(AClimbSegment* segmentConstruction);

#if WITH_EDITOR

	virtual void OnConstruction(const FTransform& transform) override;
#endif

	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<AClimbSegment*> ClimbSegments;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	class UAnimSequence* IdleToHangAnim;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UAnimSequence* HangIdleAnim;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UAnimSequence* ShimmyLeftAnim;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UAnimSequence* ShimmyRightAnim;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UAnimSequence* HopLeftAnim;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UAnimSequence* HopRightAnim;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UAnimSequence* HopUpAnim;

	UPROPERTY(EditAnywhere)
	FTransform IdleToHangAnimTransform;

	UPROPERTY(EditAnywhere)
	FTransform HopLeftAnimTransform;

	UPROPERTY(EditAnywhere)
	FTransform HopRightAnimTransform;

	UPROPERTY(EditAnywhere)
	FTransform HopUpAnimTransform;

	UPROPERTY(EditAnywhere)
	FTransform MeshComponentToHandCenterTransform;

	UPROPERTY(EditAnywhere)
	FTransform LedgeToLeftHandTransform;
	
	UPROPERTY(EditAnywhere)
	FTransform LedgeToRightHandTransform;

	float CharacterDistanceAlongSpline;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnBeginOverlapCollider(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnEndOverlapCollider(UPrimitiveComponent* overlappedComp, AActor* other, UPrimitiveComponent* otherComp, int32 otherBodyIndex);

	UFUNCTION()
	void OnBeginInteraction();

	void PerformHop(AClimbSegment* segmenHopTo,
		UAnimSequence* animToPlay,
		const FTransform& animTransform,
		const FVector2D& deltaCorrectionWindow,
		float splineDestination,
		float splineDirScalar);

	// helper function for getting anim curve
	static struct FFloatCurve* GetAnimCurve(class UAnimSequence* anim, const FName& curveName);

	static float GetDistanceAlongSplineFromInputKey(float inputKey, USplineComponent* inSplineComponent);

	UPROPERTY()
	class  APPCharacter* InteractingCharacter;

	UPROPERTY()
	class UThirdPersonAnimInstance* InteractingCharacterAnimInstance;

	UPROPERTY()
	class UPPCharacterMovementComponent* InteractingCharacterMovement;

	UPROPERTY()
	AClimbSegment* InteractingClimbSegment;

	int32 CharacterInputActionBindingIdx = -1;

	FTransform TargetEffectorTransform;

	FVector RightHandEffectorLocation;

	FVector LeftHandEffectorLocation;

	
#if WITH_EDITOR
private:

	/** Editor Preview */
	UPROPERTY()
	UClimbNodeGraphRenderingComponent* EdRenderComp;
	//UPROPERTY()
	//UBillboardComponent* SpriteComponent;

#endif // WITH_EDITORONLY_DATA
};

USTRUCT()
struct PETPROJECT_API FRootMotionSource_Spline : public FRootMotionSource
{
	GENERATED_USTRUCT_BODY()

		virtual void PrepareRootMotion(
			float simulationTime,
			float movementTickTime,
			const ACharacter& character,
			const UCharacterMovementComponent& moveComponent
		) override;

	UPROPERTY()
	class USplineComponent* RootFollowingSpline;

	UPROPERTY()
	AClimbNodeGraph* InteractingNodeGraph;

	FTransform EffectorToRootTransform;

	TArray<FFloatCurve*, TFixedAllocator<3>> TranslationCurves;
	
	float InitialDistanceAlongSpline;

	float LastDistanceAlongSpline;

	bool MovingRightIsForward;
};