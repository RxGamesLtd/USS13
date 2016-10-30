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

FogOfWarWorker::FogOfWarWorker(AFogOfWarManager *manager) {
    Manager = manager;
    Thread = FRunnableThread::Create(this, TEXT("AFogOfWarWorker"), 0U, TPri_BelowNormal);
    TimeTillLastTick = 0.0f;
}

FogOfWarWorker::~FogOfWarWorker() {
    delete Thread;
    Thread = nullptr;
}

void FogOfWarWorker::ShutDown() {
    Stop();
    Thread->WaitForCompletion();
}

bool FogOfWarWorker::Init() {
    if (Manager) {
        Manager->GetWorld()->GetFirstPlayerController()->ClientMessage("Fog of War worker thread started");
        return true;
    }
    return false;
}

uint32 FogOfWarWorker::Run() {
    FPlatformProcess::Sleep(0.03f);

    while (StopTaskCounter.GetValue() == 0) {
        float time;
        if (!Manager || !Manager->GetWorld()) {
            return 0;
        }
        time = Manager->GetWorld()->TimeSeconds;

        if (!Manager->bHasFOWTextureUpdate) {
            UpdateFowTexture(Manager->GetWorld()->TimeSince(TimeTillLastTick));
            if (!Manager->GetWorld()) {
                return 0;
            }
            Manager->fowUpdateTime = Manager->GetWorld()->TimeSince(time);
        }
        TimeTillLastTick = Manager->GetWorld()->TimeSeconds;

        FPlatformProcess::Sleep(0.1f);
    }
    return 0;
}

void FogOfWarWorker::UpdateFowTexture(float time) const {
    Manager->LastFrameTextureData = TArray<FColor>(Manager->TextureData);
    uint32 halfTextureSize = Manager->TextureSize / 2;
    int32 signedSize = static_cast<int32>(Manager->TextureSize); //For convenience....
    TSet<FVector2D> currentlyInSight;
    TSet<FVector2D> texelsToBlur;
    int32 sightTexels = FMath::TruncToInt(Manager->SightRange * Manager->SamplesPerMeter);
    float dividend = 100.0f / Manager->SamplesPerMeter;
    const FName TraceTag(TEXT("FOW trace"));

    if (!Manager->GetWorld())
        return;

    Manager->GetWorld()->DebugDrawTraceTag = TraceTag;

    for (auto Itr(Manager->FowActors.CreateIterator()); Itr; ++Itr) {
        //Find actor position
        if (!(*Itr)->IsValidLowLevel()) continue;
        FVector position = (*Itr)->GetActorLocation();

        //We divide by 100.0 because 1 texel equals 1 meter of visibility-data.
        int32 posX = static_cast<int32>(position.X / dividend) + halfTextureSize;
        int32 posY = static_cast<int32>(position.Y / dividend) + halfTextureSize;
        //float integerX;
        //float integerY;

        //FVector2D fractions = FVector2D(FMath::Modf(position.X / dividend, &integerX), FMath::Modf(position.Y / dividend, &integerY));
        FVector2D textureSpacePos = FVector2D(posX, posY);
        int32 size = static_cast<int32>(Manager->TextureSize);

        FCollisionQueryParams queryParams(FName(TEXT("FOW trace")), false, (*Itr));
        int32 halfKernelSize = (Manager->blurKernelSize - 1) / 2;

        //Store the positions we want to blur
        for (int32 y = posY - sightTexels - halfKernelSize; y <= posY + sightTexels + halfKernelSize; y++) {
            for (int32 x = posX - sightTexels - halfKernelSize; x <= posX + sightTexels + halfKernelSize; x++) {
                if (x > 0 && x < size && y > 0 && y < size) {
                    texelsToBlur.Add(FIntPoint(x, y));
                }
            }
        }

        //forget about old positions
        for (int i = 0; i < size * size; ++i) {
            float secondsToForget = Manager->SecondsToForget;
            Manager->UnfoggedData[i] -= 1.0f / secondsToForget * time;

            if (Manager->UnfoggedData[i] < 0.0f)
                Manager->UnfoggedData[i] = 0.0f;
        }

        //Unveil the positions our actors are currently looking at
        for (int y = posY - sightTexels; y <= posY + sightTexels; y++) {
            for (int x = posX - sightTexels; x <= posX + sightTexels; x++) {
                //Kernel for radial sight
                if (x > 0 && x < size && y > 0 && y < size) {
                    FVector2D currentTextureSpacePos = FVector2D(x, y);
                    int length = static_cast<int>((textureSpacePos - currentTextureSpacePos).Size());
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
    } else {
        for (int32 y = 0; y < signedSize; y++) {
            for (int32 x = 0; x < signedSize; x++) {
                if (Manager->UnfoggedData[x + (y * signedSize)] > 0.0f) {
                    if (currentlyInSight.Contains(FVector2D(x, y))) {
                        Manager->TextureData[x + y * signedSize] = FColor(static_cast<uint8>(255),
                                                                          static_cast<uint8>(255),
                                                                          static_cast<uint8>(255), 255);
                    } else {
                        Manager->TextureData[x + y * signedSize] = FColor(static_cast<uint8>(100),
                                                                          static_cast<uint8>(100),
                                                                          static_cast<uint8>(100), 255);
                    }
                } else {
                    Manager->TextureData[x + y * signedSize] = FColor(static_cast<uint8>(0), static_cast<uint8>(0),
                                                                      static_cast<uint8>(0), 255);
                }
            }
        }
    }

    Manager->bHasFOWTextureUpdate = true;
}

void FogOfWarWorker::Stop() {
    StopTaskCounter.Increment();
}
