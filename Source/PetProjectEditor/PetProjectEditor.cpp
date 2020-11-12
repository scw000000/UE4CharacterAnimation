// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "PetProjectEditor.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Animation/FootAnimationModifier.h"
#include "Actors/ClimbNodeGraphModifierDetails.h"

IMPLEMENT_GAME_MODULE(FPetProjectEditorModule, PetProjectEditor );

 
DEFINE_LOG_CATEGORY(PetProjectEditor);
 
#define LOCTEXT_NAMESPACE "PetProjectEditor"
 
void FPetProjectEditorModule::StartupModule()
{
	UE_LOG(PetProjectEditor, Warning, TEXT("Orfeas module has started!"));
	FPropertyEditorModule& propertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	propertyModule.RegisterCustomClassLayout("FootAnimationModifier", FOnGetDetailCustomizationInstance::CreateStatic(&FFootAnimationModifierDetails::MakeInstance));
	propertyModule.RegisterCustomClassLayout("ClimbNodeGraph", FOnGetDetailCustomizationInstance::CreateStatic(&FClimbNodeGraphModifierDetails::MakeInstance));
}
 
void FPetProjectEditorModule::ShutdownModule()
{
	UE_LOG(PetProjectEditor, Warning, TEXT("Orfeas module has shut down"));
}
#undef LOCTEXT_NAMESPACE