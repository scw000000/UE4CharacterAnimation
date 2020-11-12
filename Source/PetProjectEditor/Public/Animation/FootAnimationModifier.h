// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimCurveTypes.h"
#include "Engine/DataAsset.h"
#include "IDetailCustomization.h"
#include "Input/Reply.h"
#include "IPropertyTypeCustomization.h"
#include "FootAnimationModifier.generated.h"

class UAnimSequence;

USTRUCT()
struct PETPROJECTEDITOR_API FMarkerGroup
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere)
	FName NotifyTrackName;

	UPROPERTY(EditAnywhere)
	FName MarkerName;

	UPROPERTY(EditAnywhere)
	TArray<FName> SocketNames;
};



UCLASS(Abstract)
class PETPROJECTEDITOR_API UCurveGenerator : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool Enabled;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool ResetCurveBeforeGeneration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> TrackingBoneNames;
	
	virtual void TryInitCurve(UAnimSequence* animSequence) {};

	// virtual ERawCurveTrackTypes GetCurveType() const { return ERawCurveTrackTypes::RCT_Float; };
	virtual void CreateKey(UAnimSequence* outputAnimSequence, 
		UAnimSequence* referenceAnimSequence, 
		int frame,
		const TArray<FTransform>& initialBoneTransforms, 
		const TArray<FTransform>& previousBoneTransforms, 
		const TArray<FTransform>& currentBoneTransforms) {};
};


UCLASS(Abstract)
class PETPROJECTEDITOR_API UFloatCurveGenerator : public UCurveGenerator
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName CurveName;

	virtual void TryInitCurve(UAnimSequence* animSequence) override;

	// virtual void CreateKey(UAnimSequence* outputAnimSequence, UAnimSequence* referenceAnimSequence, int frame, const FTransform& initialBoneTransform, const FTransform& previousBoneTransform, const FTransform& currentBoneTransform) override;
};


UCLASS(BlueprintType)
class PETPROJECTEDITOR_API UVelocityCurveGenerator : public UFloatCurveGenerator
{
	GENERATED_BODY()
public:
	
	virtual void CreateKey(UAnimSequence* outputAnimSequence,
		UAnimSequence* referenceAnimSequence,
		int frame,
		const TArray<FTransform>& initialBoneTransforms,
		const TArray<FTransform>& previousBoneTransforms,
		const TArray<FTransform>& currentBoneTransforms) override;
};


UCLASS(Abstract)
class PETPROJECTEDITOR_API UVectorCurveGenerator : public UCurveGenerator
{
	GENERATED_BODY()
public:
	UVectorCurveGenerator();

	UPROPERTY(EditAnywhere, EditFixedSize)
	TArray<bool> ChannelEnabled;
	
	UPROPERTY(EditAnywhere, EditFixedSize)
	TArray<FName> CurveChannelNames;

	virtual void TryInitCurve(UAnimSequence* animSequence) override;
};


UCLASS(BlueprintType)
class PETPROJECTEDITOR_API UTranslationCurveGenerator : public UVectorCurveGenerator
{
	GENERATED_BODY()
public:
	virtual void CreateKey(UAnimSequence* outputAnimSequence,
		UAnimSequence* referenceAnimSequence,
		int frame,
		const TArray<FTransform>& initialBoneTransforms,
		const TArray<FTransform>& previousBoneTransforms,
		const TArray<FTransform>& currentBoneTransforms) override;
};


UCLASS(BlueprintType)
class PETPROJECTEDITOR_API URotationCurveGenerator : public UVectorCurveGenerator
{
	GENERATED_BODY()
public:
	virtual void CreateKey(UAnimSequence* outputAnimSequence,
		UAnimSequence* referenceAnimSequence,
		int frame,
		const TArray<FTransform>& initialBoneTransforms,
		const TArray<FTransform>& previousBoneTransforms,
		const TArray<FTransform>& currentBoneTransforms) override;
};

/**
 *
 */
UCLASS()
class PETPROJECTEDITOR_API UFootAnimationModifier : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Asset")
	UAnimSequence* ReferenceAnimSequence;

	UPROPERTY(EditAnywhere, Category = "Asset")
	UAnimSequence* OutputAnimSequence;

	UPROPERTY(EditAnywhere, Category = "Marker Generation")
	bool EnableMarkerGeneration;

	UPROPERTY(EditAnywhere, Category = "Marker Generation")
	TArray<FMarkerGroup> MarkerGroups;

	UPROPERTY(EditAnywhere, Category = "Marker Generation")
	float ZThreshold = 0.f;

	UPROPERTY(EditAnywhere, Category = "Moevement Information Extraction")
	FName RootBoneName;

	UPROPERTY(EditAnywhere, Category = "Moevement Information Extraction")
	FName ChestBoneName;

	UPROPERTY(EditAnywhere, Category = "Curve Generation")
	TSubclassOf<UCurveGenerator> CurveGeneratorClass;

	UPROPERTY(EditAnywhere, Category = "Curve Generation")
	TArray<UCurveGenerator*> CurveGenerators;
};

class PETPROJECTEDITOR_API FFootAnimationModifierDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	FFootAnimationModifierDetails() = default;

private:
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;

	static FTransform GetBoneTransformInWorldSpace(const FReferenceSkeleton& refSkeleton, const TArray<FTransform>& poseTransforms, TMap<FName, FTransform>& cache, const FName& boneName);
	static TArray<FTransform> GetBoneTransformsInWorldSpace(const FReferenceSkeleton& refSkeleton, const TArray<FTransform>& poseTransforms, TMap<FName, FTransform>& cache, TArray<FName> boneNames);


	FReply GenerateMarkers();
	FReply GenerateCurveGenerator();

	TWeakObjectPtr<UFootAnimationModifier> ReferencingObject;
private:
};