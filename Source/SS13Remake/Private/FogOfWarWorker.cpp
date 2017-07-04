#include "FogOfWarWorker.h"
#include "EngineMinimal.h"
#include "FogOfWarManager.h"
#include "SS13Remake.h"
#include <utility>

FogOfWarWorker::FogOfWarWorker(AFogOfWarManager* manager)
{
    Manager = manager;
    Thread = FRunnableThread::Create(this, TEXT("AFogOfWarWorker"), 0U, TPri_BelowNormal);
    TimeTillLastTick = 0.0f;
}

FogOfWarWorker::~FogOfWarWorker()
{
    UE_LOG(LogSS13Remake, Log, TEXT("Fog of War worker destroyed!"));
    delete Thread;
    Thread = nullptr;
}

void FogOfWarWorker::ShutDown()
{
    Stop();
    Thread->WaitForCompletion();
}

bool FogOfWarWorker::Init()
{
    if (Manager) {
        UE_LOG(LogSS13Remake, Log, TEXT("Fog of War worker thread started"));
        return true;
    }
    return false;
}

uint32 FogOfWarWorker::Run()
{
    FPlatformProcess::Sleep(0.03f);

    while (StopTaskCounter.GetValue() == 0) {
        if (!Manager || !Manager->GetWorld()) {
            return 0;
        }

        if (!Manager->bHasFOWTextureUpdate) {
            auto time = Manager->GetWorld()->TimeSeconds;
            //UE_LOG(LogSS13Remake, Log, TEXT("Fog start update."));
            auto delta = Manager->GetWorld()->TimeSince(TimeTillLastTick);
            UpdateFowTextureFromCamera(delta);
            //UE_LOG(LogSS13Remake, Log, TEXT("Fog updated! Ms:{0}"), delta);
            if (!Manager || !Manager->GetWorld()) {
                return 0;
            }
            Manager->fowUpdateTime = Manager->GetWorld()->TimeSince(time);
        }

        TimeTillLastTick = Manager->GetWorld()->TimeSeconds;

        FPlatformProcess::Sleep(0.05f);
    }
    return 0;
}

void FogOfWarWorker::Stop()
{
    UE_LOG(LogSS13Remake, Log, TEXT("Fog of War worker thread stoped."));
    StopTaskCounter.Increment();
}

void FogOfWarWorker::UpdateFowTexture(float time) const
{
    Manager->LastFrameTextureData = TArray<FColor>(Manager->TextureData);
    int32 halfTextureSize = Manager->TextureSize / 2;
    int32 signedSize = static_cast<int32>(Manager->TextureSize); //For convenience....
    TSet<FVector2D> currentlyInSight;
    TSet<FVector2D> texelsToBlur;
    int32 sightTexels = FMath::TruncToInt(Manager->SightRange * Manager->SamplesPerMeter);
    float dividend = 100.0f / Manager->SamplesPerMeter;
    const FName TraceTag(TEXT("FOWTrace"));

    const auto PC = Manager->GetWorld()->GetFirstPlayerController();

    //const FVector cameraPos = Manager->GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetCameraLocation();

    if (!Manager->GetWorld())
        return;

    Manager->GetWorld()->DebugDrawTraceTag = TraceTag;

    for (auto Itr(Manager->Observers.CreateIterator()); Itr; ++Itr) {
        //Find actor position
        if (!(*Itr)->IsValidLowLevel())
            continue;
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

void FogOfWarWorker::UpdateFowTextureFromCamera(float time) const
{

    Manager->LastFrameTextureData = TArray<FColor>(Manager->TextureData);
    Manager->LastCameraPosition = FVector(Manager->CameraPosition);

    TSet<FVector2D> currentlyInSight;

    if (!Manager->GetWorld())
        return;

    // First PC is local player on client
    const auto& PC = Manager->GetWorld()->GetFirstPlayerController();

    if (!PC)
        return;

    // Get viewport size
    int32 viewSizeX, viewSizeY;
    PC->GetViewportSize(viewSizeX, viewSizeY);
    const FVector2D viewportVSize(viewSizeX, viewSizeY);

    // Cache camera location
    if (!PC->PlayerCameraManager)
        return;

    Manager->CameraPosition = FVector(PC->PlayerCameraManager->GetCameraLocation());

    // Alias texture size
    auto textureSize = static_cast<int32>(Manager->TextureSize);
    const FVector2D textureVSize(textureSize, textureSize);

    // Debug stuff
    static const FName TraceTag(TEXT("FOWTrace"));

    //forget about old positions
    for (auto&& dta : Manager->UnfoggedData) {
        dta -= time / Manager->SecondsToForget;
        if (dta < 0.0f)
            dta = 0.0f;
    }

    TArray<FVector> observers;
    observers.Reserve(Manager->Observers.Num());
    for (const auto& o : Manager->Observers) {
        observers.Add(o->GetActorLocation());
    }

    // iterate through observers to unveil fog
    //for (const auto& observer : Manager->Observers) {
    //FVector observerLocation = observer->GetActorLocation();
    for (auto& observerLocation : observers) {

        observerLocation.Z = 1;

        FVector2D observerTexLoc;
        // skip actors too far
        if (!PC->ProjectWorldLocationToScreen(observerLocation, observerTexLoc, true))
            continue;

        // Translate to Texture space
        observerTexLoc /= viewportVSize;
        observerTexLoc *= textureVSize;

        FCollisionQueryParams queryParams(TraceTag, true);

        TSet<FVector2D> sightShape;

        for (float i = 0; i < 2 * PI; i += HALF_PI / 100.0f) {
            auto x = Manager->SightRange * FMath::Cos(i);
            auto y = Manager->SightRange * FMath::Sin(i);

            FVector sightLoc = observerLocation + FVector(x, y, 0);

            FHitResult hit;
            if (Manager->GetWorld()->LineTraceSingleByChannel(hit, observerLocation, sightLoc, ECC_SightStatic, queryParams)) {
                sightLoc = hit.Location;
            }

            FVector2D hitTexLoc;
            if (PC->ProjectWorldLocationToScreen(sightLoc, hitTexLoc, true)) {
                // Translate to Texture space
                hitTexLoc /= viewportVSize;
                hitTexLoc *= textureVSize;

                FMath::Clamp(hitTexLoc, FVector2D::ZeroVector, textureVSize);

                if (hitTexLoc.X < 0 || hitTexLoc.X >= textureSize || hitTexLoc.Y < 0 || hitTexLoc.Y >= textureSize)
                    continue;

                sightShape.Add(hitTexLoc);
            }
        }
        //UE_LOG(LogSS13Remake, Log, TEXT("Fog sights checked."));
        // draw a unveil shape
        DrawUnveilShape(observerTexLoc, sightShape);
        //UE_LOG(LogSS13Remake, Log, TEXT("Fog bound drawn."));
        //flood fill area
        //FloodFill(observerTexLoc.X, observerTexLoc.Y);
        //UE_LOG(LogSS13Remake, Log, TEXT("Fog sights filled."));
    }

    for (int32 y = 0; y < textureSize; ++y) {
        for (int32 x = 0; x < textureSize; ++x) {
            auto visibility = Manager->UnfoggedData[x + y * textureSize];
            if (visibility > 0.9f) {
                Manager->TextureData[x + y * textureSize] = FColor(static_cast<uint8>(255),
                    static_cast<uint8>(255),
                    static_cast<uint8>(255), 255);
            } else if (visibility > 0.1f) {
                Manager->TextureData[x + y * textureSize] = FColor(static_cast<uint8>(100),
                    static_cast<uint8>(100),
                    static_cast<uint8>(100), 255);
            } else {
                Manager->TextureData[x + y * textureSize] = FColor(static_cast<uint8>(0),
                    static_cast<uint8>(0),
                    static_cast<uint8>(0), 255);
            }
        }
    }

    Manager->bHasFOWTextureUpdate = true;
}

void FogOfWarWorker::DrawUnveilShape(FVector2D observerTexLoc, TSet<FVector2D> sightShape) const
{
    const auto textureSize = static_cast<int32>(Manager->TextureSize);

    auto points = sightShape.Array();
    for (auto i = 0; i < points.Num(); ++i) {
        auto point = points[i];
        //auto next = points[(i + 1) % points.Num()];
        auto next = observerTexLoc;
        // Bresenham's line algorithm
        // https://rosettacode.org/wiki/Bitmap/Bresenham%27s_line_algorithm#C.2B.2B
        float x1 = point.X;
        float y1 = point.Y;
        float x2 = next.X;
        float y2 = next.Y;
        const bool steep = FMath::Abs(y2 - y1) > FMath::Abs(x2 - x1);
        if (steep) {
            std::swap(x1, y1);
            std::swap(x2, y2);
        }

        if (x1 > x2) {
            std::swap(x1, x2);
            std::swap(y1, y2);
        }

        const float dx = x2 - x1;
        const float dy = FMath::Abs(y2 - y1);

        float error = dx / 2.0f;
        const int32 ystep = (y1 < y2) ? 1 : -1;
        int32 y = FMath::TruncToInt(y1);

        const int32 maxX = FMath::TruncToInt(x2);

        const int32 brushSize = 1;

        for (int32 x = FMath::TruncToInt(x1); x < maxX; x++) {
            if (steep) {
                //Unveil the positions we are currently seeing
                for (auto offX = -brushSize; offX <= brushSize; ++offX)
                    for (auto offY = -brushSize; offY <= brushSize; ++offY)
                        if (x + offY >= 0 && x + offY < textureSize && y + offX >= 0 && y + offX < textureSize)
                            Manager->UnfoggedData[y + offX + (x + offY) * textureSize] = 1.0f;
            } else {
                //Unveil the positions we are currently seeing
                for (auto offX = -brushSize; offX <= brushSize; ++offX)
                    for (auto offY = -brushSize; offY <= brushSize; ++offY)
                        if (x + offX >= 0 && x + offX < textureSize && y + offY >= 0 && y + offY < textureSize)
                            Manager->UnfoggedData[x + offX + (y + offY) * textureSize] = 1.0f;
            }

            error -= dy;
            if (error < 0) {
                y += ystep;
                error += dx;
            }
        }
    }
}

void FogOfWarWorker::FloodFill(int x, int y) const
{
    const auto textureSize = static_cast<int32>(Manager->TextureSize);

    if (x < 0 || x >= textureSize || y < 0 || y >= textureSize)
        return;

    // Recursive
    //auto& current = Manager->UnfoggedData[x + y * textureSize];
    //if (!FMath::IsNearlyEqual(current, 1.0f))
    //{
    //	current = 1.0f;
    //	FloodFill(x + 1, y);
    //	FloodFill(x - 1, y);
    //	FloodFill(x, y + 1);
    //	FloodFill(x, y - 1);
    //}

    //Flood-fill (node, target-color, replacement-color):
    //1. If target - color is equal to replacement - color, return.
    //2. If color of node is not equal to target - color, return.
    if (FMath::IsNearlyEqual(Manager->UnfoggedData[x + y * textureSize], 1.0f))
        return;
    //3. Set Q to the empty queue.
    TQueue<FIntVector> Q;
    //4. Add node to Q.
    Q.Enqueue(FIntVector(x, y, 0));

    FIntVector N;
    //5. For each element N of Q :
    while (Q.Dequeue(N)) {
        //6.         Set w and e equal to N.
        auto w = N, e = N;
        //7.         Move w to the west until the color of the node to the west of w no longer matches target - color.
        while (w.X - 1 > 0 && !FMath::IsNearlyEqual(Manager->UnfoggedData[w.X - 1 + w.Y * textureSize], 1.0f))
            w.X--;
        //8.         Move e to the east until the color of the node to the east of e no longer matches target - color.
        while (e.X + 1 < textureSize && !FMath::IsNearlyEqual(Manager->UnfoggedData[e.X + 1 + e.Y * textureSize], 1.0f))
            e.X++;
        //9.         For each node n between w and e :
        for (auto i = w.X; i <= e.X; ++i) {
            FIntVector n(i, N.Y, 0);
            //10.             Set the color of n to replacement - color.
            Manager->UnfoggedData[n.X + n.Y * textureSize] = 1.0f;
            //11.             If the color of the node to the north of n is target - color, add that node to Q.
            if (n.Y + 1 < textureSize && !FMath::IsNearlyEqual(Manager->UnfoggedData[n.X + (n.Y + 1) * textureSize], 1.0f))
                Q.Enqueue(FIntVector(n.X, n.Y + 1, 0));
            //12.             If the color of the node to the south of n is target - color, add that node to Q.
            if (n.Y - 1 > 0 && !FMath::IsNearlyEqual(Manager->UnfoggedData[n.X + (n.Y - 1) * textureSize], 1.0f))
                Q.Enqueue(FIntVector(n.X, n.Y - 1, 0));
        }
        //13. Continue looping until Q is exhausted.
    }
    //14. Return.
}
