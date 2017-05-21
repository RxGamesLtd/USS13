#include "SS13RemakeGameMode.h"
#include "SS13RemakePlayerController.h"
#include "SS13RemakeCharacter.h"

ASS13RemakeGameMode::ASS13RemakeGameMode() {
	// use our custom PlayerController class
	PlayerControllerClass = ASS13RemakePlayerController::StaticClass();
	DefaultPawnClass = ASS13RemakeCharacter::StaticClass();
}
