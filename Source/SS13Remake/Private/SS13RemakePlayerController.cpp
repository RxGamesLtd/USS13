#include "SS13RemakePlayerController.h"
#include "AI/Navigation/NavigationSystem.h"
#include "Engine/World.h"

ASS13RemakePlayerController::ASS13RemakePlayerController() : bMoveToMouseCursor(false) {
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void ASS13RemakePlayerController::PlayerTick(float DeltaTime) {
	Super::PlayerTick(DeltaTime);

	// keep updating the destination every tick while desired
	if (bMoveToMouseCursor) {
		MoveToMouseCursor();
	}
}

void ASS13RemakePlayerController::SetupInputComponent() {
	// set up gameplay key bindings
	Super::SetupInputComponent();

	InputComponent->BindAction(FName(TEXT("SetDestination")), IE_Pressed, this,
	                           &ASS13RemakePlayerController::OnSetDestinationPressed);
	InputComponent->BindAction(FName(TEXT("SetDestination")), IE_Released, this,
	                           &ASS13RemakePlayerController::OnSetDestinationReleased);

	// support touch devices
	InputComponent->BindTouch(EInputEvent::IE_Pressed, this, &ASS13RemakePlayerController::MoveToTouchLocation);
	InputComponent->BindTouch(EInputEvent::IE_Repeat, this, &ASS13RemakePlayerController::MoveToTouchLocation);
}

void ASS13RemakePlayerController::MoveToMouseCursor() {
	// Trace to see what is under the mouse cursor
	FHitResult Hit;
	GetHitResultUnderCursor(ECC_Visibility, false, Hit);

	if (Hit.bBlockingHit) {
		// We hit something, move there
		SetNewMoveDestination(Hit.ImpactPoint);
	}
}

void ASS13RemakePlayerController::MoveToTouchLocation(const ETouchIndex::Type FingerIndex, const FVector Location) {
	FVector2D ScreenSpaceLocation(Location);

	// Trace to see what is under the touch location
	FHitResult HitResult;
	GetHitResultAtScreenPosition(ScreenSpaceLocation, ECollisionChannel::ECC_GameTraceChannel1, true, HitResult);
	if (HitResult.bBlockingHit) {
		// We hit something, move there
		SetNewMoveDestination(HitResult.ImpactPoint);
	}
}

void ASS13RemakePlayerController::SetNewMoveDestination(const FVector DestLocation) {
	const APawn* controlledPawn = GetPawn();
	if (controlledPawn) {
		const UNavigationSystem* NavSys = GetWorld()->GetNavigationSystem();
		const float Distance = FVector::Dist(DestLocation, controlledPawn->GetActorLocation());

		// We need to issue move command only if far enough in order for walk animation to play correctly
		if (NavSys && (Distance > 220.0f)) {
			NavSys->SimpleMoveToLocation(this, DestLocation);
		}
		else {
			auto rot = (DestLocation - controlledPawn->GetActorLocation()).Rotation();
			rot.Pitch = 0.0f;
			SetControlRotation(rot);
		}
	}
}

void ASS13RemakePlayerController::OnSetDestinationPressed() {
	// set flag to keep updating destination until released
	bMoveToMouseCursor = true;
}

void ASS13RemakePlayerController::OnSetDestinationReleased() {
	// clear flag to indicate we should stop updating the destination
	bMoveToMouseCursor = false;
}
