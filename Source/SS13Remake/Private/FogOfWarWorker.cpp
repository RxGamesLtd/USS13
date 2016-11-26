// The MIT License (MIT)
// Copyright (c) 2016 RxCompile
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions
// of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
// OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "SS13Remake.h"
#include "FogOfWarManager.h"

FogOfWarWorker::FogOfWarWorker(AFogOfWarManager* manager) {
	Manager = manager;
	Thread = FRunnableThread::Create(this, TEXT("AFogOfWarWorker"), 0U, TPri_BelowNormal);
	TimeTillLastTick = 0.0f;
}

FogOfWarWorker::~FogOfWarWorker() {
	UE_LOG(LogSS13Remake, Log, TEXT("Fog of War worker destroyed!"));
	delete Thread;
	Thread = nullptr;
}

void FogOfWarWorker::ShutDown() {
	Stop();
	Thread->WaitForCompletion();
}

bool FogOfWarWorker::Init() {
	if (Manager) {
		UE_LOG(LogSS13Remake, Log, TEXT("Fog of War worker thread started"));
		return true;
	}
	return false;
}

uint32 FogOfWarWorker::Run() {
	FPlatformProcess::Sleep(0.03f);

	while (StopTaskCounter.GetValue() == 0) {
		if (!Manager || !Manager->GetWorld()) {
			return 0;
		}

		if (!Manager->bHasFOWTextureUpdate) {
			auto time = Manager->GetWorld()->TimeSeconds;
			UpdateFowTextureFromCamera(Manager->GetWorld()->TimeSince(TimeTillLastTick));
			Manager->fowUpdateTime = Manager->GetWorld()->TimeSince(time);
		}

		TimeTillLastTick = Manager->GetWorld()->TimeSeconds;

		FPlatformProcess::Sleep(0.1f);
	}
	return 0;
}

void FogOfWarWorker::Stop() {
	UE_LOG(LogSS13Remake, Log, TEXT("Fog of War worker thread stoped."));
	StopTaskCounter.Increment();
}

void FogOfWarWorker::UpdateFowTexture(float time) const {
	Manager->LastFrameTextureData = TArray<FColor>(Manager->TextureData);
	int32 halfTextureSize = Manager->TextureSize / 2;
	int32 signedSize = static_cast<int32>(Manager->TextureSize); //For convenience....
	TSet<FVector2D> currentlyInSight;
	TSet<FVector2D> texelsToBlur;
	int32 sightTexels = FMath::TruncToInt(Manager->SightRange * Manager->SamplesPerMeter);
	float dividend = 100.0f / Manager->SamplesPerMeter;
	const FName TraceTag(TEXT("FOW trace"));

	const auto PC = Manager->GetWorld()->GetFirstPlayerController();

	//const FVector cameraPos = Manager->GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetCameraLocation();

	if (!Manager->GetWorld())
		return;

	Manager->GetWorld()->DebugDrawTraceTag = TraceTag;

	for (auto Itr(Manager->Observers.CreateIterator()); Itr; ++Itr) {
		//Find actor position
		if (!(*Itr)->IsValidLowLevel()) continue;
		FVector position = (*Itr)->GetActorLocation();
		FVector2D screenPos;
		PC->ProjectWorldLocationToScreen(position, screenPos);

		//We divide by 100.0 because 1 texel equals 1 meter of visibility-data.
		int32 posX = FMath::TruncToInt(position.X / dividend) + halfTextureSize;
		int32 posY = FMath::TruncToInt(position.Y / dividend) + halfTextureSize;
		//float integerX;
		//float integerY;

		//FVector2D fractions = FVector2D(FMath::Modf(position.X / dividend, &integerX), FMath::Modf(position.Y / dividend, &integerY));
		auto textureSpacePos = FIntVector(posX, posY, 0);
		int32 size = static_cast<int32>(Manager->TextureSize);

		FCollisionQueryParams queryParams(TraceTag, false, (*Itr));
		int32 halfKernelSize = (Manager->blurKernelSize - 1) / 2;

		//Store the positions we want to blur
		for (int32 y = textureSpacePos.Y - sightTexels - halfKernelSize;
		     y <= textureSpacePos.Y + sightTexels + halfKernelSize; y++) {
			for (int32 x = textureSpacePos.X - sightTexels - halfKernelSize;
			     x <= textureSpacePos.X + sightTexels + halfKernelSize; x++) {
				if (x > 0 && x < size && y > 0 && y < size) {
					texelsToBlur.Add(FIntPoint(x, y));
				}
			}
		}

		//forget about old positions
		for (int32 i = 0; i < size * size; ++i) {
			float secondsToForget = Manager->SecondsToForget;
			Manager->UnfoggedData[i] -= 1.0f / secondsToForget * time;

			if (Manager->UnfoggedData[i] < 0.0f)
				Manager->UnfoggedData[i] = 0.0f;
		}

		//Unveil the positions our actors are currently looking at
		for (int32 y = textureSpacePos.Y - sightTexels; y <= textureSpacePos.Y + sightTexels; y++) {
			for (int32 x = textureSpacePos.X - sightTexels; x <= textureSpacePos.X + sightTexels; x++) {
				//Kernel for radial sight
				if (x > 0 && x < size && y > 0 && y < size) {
					auto currentTextureSpacePos = FIntVector(x, y, 0);
					auto length = (textureSpacePos - currentTextureSpacePos).Size();
					if (length <= sightTexels) {
						FVector currentWorldSpacePos = FVector(
							((x - static_cast<int32>(halfTextureSize))) * dividend,
							((y - static_cast<int32>(halfTextureSize))) * dividend,
							position.Z);

						//CONSIDER: This is NOT the most efficient way to do conditional unfogging. With long view distances and/or a lot of actors affecting the FOW-data
						//it would be preferrable to not trace against all the boundary points and internal texels/positions of the circle, but create and cache "rasterizations" of
						//viewing circles (using Bresenham's midpoint circle algorithm) for the needed sightranges, shift the circles to the actor's location
						//and just trace against the boundaries.
						//We would then use Manager->GetWorld()->LineTraceSingle() and find the first collision texel. Having found the nearest collision
						//for every ray we would unveil all the points between the collision and origo using Bresenham's Line-drawing algorithm.
						//However, the tracing doesn't seem like it takes much time at all (~0.02ms with four actors tracing circles of 18 texels each),
						//it's the blurring that chews CPU..
						if (!Manager->GetWorld()->LineTraceTestByChannel(position, currentWorldSpacePos,
						                                                 ECC_SightStatic, queryParams)) {
							//Unveil the positions we are currently seeing
							Manager->UnfoggedData[x + y * Manager->TextureSize] = 1.0f;
							//Store the positions we are currently seeing.
							currentlyInSight.Add(FVector2D(x, y));
						}
					}
				}
			}
		}
	}

	if (Manager->IsBlurEnabled()) {
		//Horizontal blur pass
		int offset = FMath::TruncToInt(Manager->blurKernelSize / 2.0f);
		for (auto Itr(texelsToBlur.CreateIterator()); Itr; ++Itr) {
			int32 x = (Itr)->IntPoint().X;
			int32 y = (Itr)->IntPoint().Y;
			float sum = 0;
			for (int32 i = 0; i < Manager->blurKernelSize; i++) {
				int32 shiftedIndex = i - offset;
				if (x + shiftedIndex >= 0 && x + shiftedIndex <= signedSize - 1) {
					if (Manager->UnfoggedData[x + shiftedIndex + (y * signedSize)] > 0.0f) {
						//If we are currently looking at a position, unveil it completely
						if (currentlyInSight.Contains(FVector2D(x + shiftedIndex, y))) {
							sum += (Manager->blurKernel[i] * 255);
						}
						//If this is a previously discovered position that we're not currently looking at, put it into a "shroud of darkness".
						else {
							sum += (Manager->blurKernel[i] * 100);
						}
					}
				}
			}
			Manager->HorizontalBlurData[x + y * signedSize] = static_cast<uint8>(sum);
		}


		//Vertical blur pass
		for (auto Itr(texelsToBlur.CreateIterator()); Itr; ++Itr) {
			int32 x = (Itr)->IntPoint().X;
			int32 y = (Itr)->IntPoint().Y;
			float sum = 0;
			for (int32 i = 0; i < Manager->blurKernelSize; i++) {
				int32 shiftedIndex = i - offset;
				if (y + shiftedIndex >= 0 && y + shiftedIndex <= signedSize - 1) {
					sum += (Manager->blurKernel[i] * Manager->HorizontalBlurData[x + (y + shiftedIndex) * signedSize]);
				}
			}
			Manager->TextureData[x + y * signedSize] = FColor(static_cast<uint8>(sum), static_cast<uint8>(sum),
			                                                  static_cast<uint8>(sum), 255);
		}
	}
	else {
		for (int32 y = 0; y < signedSize; y++) {
			for (int32 x = 0; x < signedSize; x++) {
				if (Manager->UnfoggedData[x + (y * signedSize)] > 0.0f) {
					if (currentlyInSight.Contains(FVector2D(x, y))) {
						Manager->TextureData[x + y * signedSize] = FColor(static_cast<uint8>(255),
						                                                  static_cast<uint8>(255),
						                                                  static_cast<uint8>(255), 255);
					}
					else {
						Manager->TextureData[x + y * signedSize] = FColor(static_cast<uint8>(100),
						                                                  static_cast<uint8>(100),
						                                                  static_cast<uint8>(100), 255);
					}
				}
				else {
					Manager->TextureData[x + y * signedSize] = FColor(static_cast<uint8>(0), static_cast<uint8>(0),
					                                                  static_cast<uint8>(0), 255);
				}
			}
		}
	}

	Manager->bHasFOWTextureUpdate = true;
}

void FogOfWarWorker::UpdateFowTextureFromCamera(float time) const {

	Manager->LastFrameTextureData = TArray<FColor>(Manager->TextureData);

	TSet<FVector2D> currentlyInSight;

	if (!Manager->GetWorld())
		return;

	// First PC is local player on client
	const auto PC = Manager->GetWorld()->GetFirstPlayerController();

	// Get viewport size
	int32 viewSizeX, viewSizeY;
	PC->GetViewportSize(viewSizeX, viewSizeY);

	auto textureSize = static_cast<int32>(Manager->TextureSize);

	const FName TraceTag(TEXT("FOWTrace"));

	//forget about old positions
	for (auto& dta : Manager->UnfoggedData) {
		dta -= 1.0f / Manager->SecondsToForget * time;
		if (dta < 0.0f)
			dta = 0.0f;
	}

	// iterate through observers to unveil fog
	for (auto Itr(Manager->Observers.CreateIterator()); Itr; ++Itr) {
		// check ptr
		if (!(*Itr)->IsValidLowLevel()) continue;

		//Find actor position
		FVector observerLocation = (*Itr)->GetActorLocation();

		FVector2D observerTexLoc;
		// skip actors too far
		if (!PC->ProjectWorldLocationToScreen(observerLocation, observerTexLoc)) continue;

		// Translate to Texture space
		observerTexLoc.X /= viewSizeX;
		observerTexLoc.Y /= viewSizeY;
		observerTexLoc *= textureSize;

		FCollisionQueryParams queryParams(TraceTag, false, (*Itr));

		TSet<FVector2D> sightShape;

		for (float i = 0; i < 2 * PI; i += HALF_PI / 8.0f) {
			auto x = Manager->SightRange * FMath::Cos(i) + observerLocation.X;
			auto y = Manager->SightRange * FMath::Sin(i) + observerLocation.Y;

			FVector sightLoc = FVector(x, y, observerLocation.Z);

			FHitResult hit;
			Manager->GetWorld()->LineTraceSingleByChannel(hit, observerLocation, sightLoc, ECC_SightStatic, queryParams);

			FVector2D hitTexLoc;
			PC->ProjectWorldLocationToScreen(hit.Location, hitTexLoc);

			// Translate to Texture space
			hitTexLoc.X /= viewSizeX;
			hitTexLoc.Y /= viewSizeY;
			hitTexLoc *= textureSize;

			sightShape.Add(hitTexLoc);
		}

		// draw a unveil shape
		DrawUnveilShape(observerTexLoc, sightShape);
	}

	for (int32 y = 0; y < textureSize; ++y) {
		for (int32 x = 0; x < textureSize; ++x) {
			auto visibility = Manager->UnfoggedData[x + y * textureSize];
			if (visibility > 0.0f) {
				if (visibility > 0.9f) {
					Manager->TextureData[x + y * textureSize] = FColor(static_cast<uint8>(255),
					                                                   static_cast<uint8>(255),
					                                                   static_cast<uint8>(255), 255);
				}
				else {
					Manager->TextureData[x + y * textureSize] = FColor(static_cast<uint8>(100),
					                                                   static_cast<uint8>(100),
					                                                   static_cast<uint8>(100), 255);
				}
			}
			else {
				Manager->TextureData[x + y * textureSize] = FColor(static_cast<uint8>(0),
				                                                   static_cast<uint8>(0),
				                                                   static_cast<uint8>(0), 255);
			}
		}
	}

	Manager->bHasFOWTextureUpdate = true;
}

void FogOfWarWorker::DrawUnveilShape(FVector2D observerTexLoc, TSet<FVector2D> sightShape) const {
	const auto textureSize = static_cast<int32>(Manager->TextureSize);

	for (auto point : sightShape) {
		// Bresenham's line algorithm
		// https://rosettacode.org/wiki/Bitmap/Bresenham%27s_line_algorithm#C.2B.2B
		float x1 = point.X;
		float y1 = point.Y;
		float x2 = observerTexLoc.X;
		float y2 = observerTexLoc.Y;
		const bool steep = FMath::Abs(y2 - y1) > FMath::Abs(x2 - x1);
		if (steep) {
			//std::swap(x1, y1);
			float temp = y1;
			y1 = x1;
			x1 = temp;
			//std::swap(x2, y2);
			temp = y2;
			y2 = x2;
			x2 = temp;
		}

		if (x1 > x2) {
			//std::swap(x1, x2);
			float temp = x2;
			x2 = x1;
			x1 = temp;
			//std::swap(y1, y2);
			temp = y2;
			y2 = y1;
			y1 = temp;
		}

		const float dx = x2 - x1;
		const float dy = FMath::Abs(y2 - y1);

		float error = dx / 2.0f;
		const int32 ystep = (y1 < y2) ? 1 : -1;
		int32 y = FMath::TruncToInt(y1);

		const int32 maxX = FMath::TruncToInt(x2);

		for (int32 x = FMath::TruncToInt(x1); x < maxX; x++) {
			if (steep) {
				//Unveil the positions we are currently seeing
				Manager->UnfoggedData[y + x * textureSize] = 1.0f;
			}
			else {
				//Unveil the positions we are currently seeing
				Manager->UnfoggedData[x + y * textureSize] = 1.0f;
			}

			error -= dy;
			if (error < 0) {
				y += ystep;
				error += dx;
			}
		}
	}
}
