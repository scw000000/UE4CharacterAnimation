// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "ModuleManager.h"
#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(PetProjectEditor, All, All);
 
class FPetProjectEditorModule : public IModuleInterface
{
	public:
 
	/* This will get called when the editor loads the module */
	virtual void StartupModule() override;
 
	/* This will get called when the editor unloads the module */
	virtual void ShutdownModule() override;
};