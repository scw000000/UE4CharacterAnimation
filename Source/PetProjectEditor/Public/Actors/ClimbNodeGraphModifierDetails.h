// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "IPropertyTypeCustomization.h"
#include "Input/Reply.h"
#include "GameFramework/Actor.h"
// #include "ClimbNodeGraphModifierDetails.generated.h"

// class AClimbNodeGraph;
class UClimbNodeGraphRenderingComponent;

class PETPROJECTEDITOR_API FClimbNodeGraphModifierDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	FClimbNodeGraphModifierDetails() = default;

private:
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;

	FReply GenerateClimbSegment();

	TWeakObjectPtr<UObject> ReferencingObject;
private:
};
