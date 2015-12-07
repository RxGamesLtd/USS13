// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SS13Remake.h"
#include "SS13RemakeGameMode.h"
#include "SS13RemakePlayerController.h"
#include "SS13RemakeCharacter.h"

ASS13RemakeGameMode::ASS13RemakeGameMode()
{
	// use our custom PlayerController class
	PlayerControllerClass = ASS13RemakePlayerController::StaticClass();

	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/TopDownCPP/Blueprints/TopDownCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}