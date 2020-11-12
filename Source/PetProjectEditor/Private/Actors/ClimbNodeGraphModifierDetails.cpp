// Fill out your copyright notice in the Description page of Project Settings.


#include "ClimbNodeGraphModifierDetails.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "ClimbNodeGraph.h"

TSharedRef<IDetailCustomization> FClimbNodeGraphModifierDetails::MakeInstance()
{
	return MakeShareable(new FClimbNodeGraphModifierDetails());
}

#define ADD_BUTTON_TO_CATEGORY(DetailLayout, CategoryName, NewRowFilterString, TextLeftToButton, ButtonText, ObjectPtr, FunctionPtr) \
	{ \
		DetailLayout.EditCategory((CategoryName)) \
			.AddCustomRow((NewRowFilterString)) \
			.NameContent() \
			[ \
				SNew(STextBlock) \
				.Font(IDetailLayoutBuilder::GetDetailFont()) \
				.Text((TextLeftToButton)) \
			] \
			.ValueContent() \
			.MaxDesiredWidth(125.f) \
			.MinDesiredWidth(125.f) \
			[ \
				SNew(SButton) \
				.ContentPadding(2) \
				.VAlign(VAlign_Center) \
				.HAlign(HAlign_Center) \
				.OnClicked((ObjectPtr), (FunctionPtr)) \
				[ \
					SNew(STextBlock) \
					.Font(IDetailLayoutBuilder::GetDetailFont()) \
					.Text((ButtonText)) \
				] \
			]; \
	}

void FClimbNodeGraphModifierDetails::CustomizeDetails(IDetailLayoutBuilder& detailBuilder)
{
	/*MyLayout = &detailBuilder;*/

	TArray< TWeakObjectPtr<UObject> > selectedObjects;
	detailBuilder.GetObjectsBeingCustomized(selectedObjects);
	// don't want to hanle mutiple selections, it's not necessarry for this data
	if (selectedObjects.Num() != 1 || !selectedObjects[0].IsValid())
	{
		return;
	}
	this->ReferencingObject = (selectedObjects[0]);
	// AClimbNodeGraph* climbNodeGraph = Cast<AClimbNodeGraph>(ReferencingObject.Get());

	ADD_BUTTON_TO_CATEGORY(detailBuilder,
		"Climb Node Graph",
		FText::FromString("Filter"),
		FText::FromString("Climb Node Creation"),
		FText::FromString("Create Climb Node"),
		this,
		&FClimbNodeGraphModifierDetails::GenerateClimbSegment);
}


FReply FClimbNodeGraphModifierDetails::GenerateClimbSegment()
{
	if (!this->ReferencingObject.IsValid())
	{
		return  FReply::Handled();
	}
	Cast<AClimbNodeGraph>(this->ReferencingObject.Get())->GenerateClimbSegment();
	return FReply::Handled();
}
