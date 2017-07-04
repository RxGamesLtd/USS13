#include "FogWorker.h"
#include "FogOfWarModule.h"
#include "FogPlane.h"
#include <utility>

DEFINE_STAT(STAT_FogUpdatesCount);

FogWorker::FogWorker(AFogPlane& manager)
    : Manager(manager)
    , bIsTaskStopped(true)
    , TimeTillLastTick(0.0f)
    , TextureSize(manager.TextureSize)
{
    Thread.Reset(FRunnableThread::Create(this, TEXT("AFogWorker"), 0U, TPri_BelowNormal));
}

bool FogWorker::Init()
{
    UE_LOG(LogFow, Log, TEXT("Fog of War worker thread started"));
    UnfoggedData.Init(0.0f, TextureSize * TextureSize);
    bIsTaskStopped = false;
    return true;
}

uint32 FogWorker::Run()
{
    while (!bIsTaskStopped) {
        FPlatformProcess::Sleep(0.1f);
        if (Manager.bHasFOWTextureUpdate) {
            continue;
        } 

        SCOPE_CYCLE_COUNTER(STAT_FogUpdatesCount);
        auto world = GEngine->GetWorld();
        if (world) {
            auto delta = world->TimeSince(TimeTillLastTick);

            Update(delta);

            TimeTillLastTick = world->TimeSeconds;
        }
    }
    return 0;
}

void FogWorker::Stop()
{
    UE_LOG(LogFow, Log, TEXT("Fog of War worker thread stoped."));
    bIsTaskStopped = true;
    Thread->WaitForCompletion();
}

void FogWorker::ForgetOldLocations(float time)
{
    const float fadedDelta = time / Manager.SecondsToForget;
    //forget about old positions
    for (auto&& dta : UnfoggedData) {
        dta -= fadedDelta;
        if (dta < 0.0f) {
            dta = 0.0f;
        }
    }
}

void FogWorker::UpdateTextureData()
{
    for (int32 y = 0; y < TextureSize; ++y) {
        for (int32 x = 0; x < TextureSize; ++x) {
            auto visibility = UnfoggedData[x + y * TextureSize];
            if (visibility > 0.9f) {
                Manager.TextureData[x + y * TextureSize] = FColor(255, 255, 255);
            } else if (visibility > 0.1f) {
                Manager.TextureData[x + y * TextureSize] = FColor(100, 100, 100);
            } else {
                Manager.TextureData[x + y * TextureSize] = FColor(0, 0, 0);
            }
        }
    }

    Manager.bHasFOWTextureUpdate = true;
}

void FogWorker::UpdateRenderOrigin() const
{
    // First PC is local player on client
    const auto& PC = GEngine->GetWorld()->GetFirstPlayerController();
    if (!PC) {
        return;
    }

    // Cache camera location
    if (!PC->PlayerCameraManager) {
        return;
    }

    // point on FogPlane in screen center in world space
    Manager.CameraPosition = PC->PlayerCameraManager->ViewTarget.Target->GetActorLocation();
}

void FogWorker::Update(float time)
{
    // Debug stuff
    static const FName TraceTag(TEXT("FOWTrace"));

    const UWorld* world = GEngine->GetWorld();
    if (!world) {
        return;
    }

    UpdateRenderOrigin();

    FVector origin, SurfaceExtent;
    Manager.GetActorBounds(false, origin, SurfaceExtent);
    if (FMath::IsNearlyZero(SurfaceExtent.Size2D())) {
        return;
    }

    ForgetOldLocations(time);

    // Cache observers location
    TArray<FVector> observers;
    observers.Reserve(Manager.Observers.Num());
    for (const auto& o : Manager.Observers) {
        if (o->IsValidLowLevel()) {
            observers.Add(o->GetActorLocation());
        }
    }

    // iterate through observers to unveil fog
    for (auto& observerLocation : observers) {
        FVector2D observerTexLoc(observerLocation - Manager.CameraPosition);
        TArray<FVector2D> sightShape;

        observerTexLoc /= FVector2D(SurfaceExtent);
        observerTexLoc *= TextureSize / 2.0f;
        observerTexLoc += FVector2D(TextureSize / 2.0f, TextureSize / 2.0f);

        FCollisionQueryParams queryParams(TraceTag, true);

        for (float i = 0; i < 2 * PI; i += HALF_PI / 100.0f) {
            auto x = Manager.SightRange * FMath::Cos(i);
            auto y = Manager.SightRange * FMath::Sin(i);

            FVector sightLoc = observerLocation + FVector(x, y, 0);

            FHitResult hit;
            if (world->LineTraceSingleByChannel(hit, observerLocation, sightLoc, ECC_GameTraceChannel2, queryParams)) {
                sightLoc = hit.Location;
            }

            FVector2D hitTexLoc(sightLoc - Manager.CameraPosition);
            hitTexLoc /= FVector2D(SurfaceExtent);
            hitTexLoc *= TextureSize / 2.0f;
            hitTexLoc += FVector2D(TextureSize / 2.0f, TextureSize / 2.0f);

            sightShape.AddUnique(hitTexLoc);
        }
        // draw a unveil shape
        DrawUnveilShape(observerTexLoc, sightShape);
        // flood fill area
        //FloodFill(observerTexLoc.X, observerTexLoc.Y);
    }

    UpdateTextureData();
}

void FogWorker::DrawUnveilShape(FVector2D observerTexLoc, TArray<FVector2D> sightShape)
{
    for (auto i = 0; i < sightShape.Num(); ++i) {
        auto point = sightShape[i];
        //auto next = sightShape[(i + 1) % sightShape.Num()];
        auto next = observerTexLoc;

        DrawLine(point, next);
    }
}

void FogWorker::DrawLine(FVector2D start, FVector2D end)
{
    // Bresenham's line algorithm
    // https://rosettacode.org/wiki/Bitmap/Bresenham%27s_line_algorithm#C.2B.2B
    float x1 = start.X;
    float y1 = start.Y;
    float x2 = end.X;
    float y2 = end.Y;

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

    for (int32 x = FMath::TruncToInt(x1); x < maxX; x++) {
        // Unveil the positions we are currently seeing
        if (steep) {
            DrawPoint(FVector2D(y, x));
        } else {
            DrawPoint(FVector2D(x, y));
        }

        error -= dy;
        if (error < 0) {
            y += ystep;
            error += dx;
        }
    }
}

void FogWorker::DrawPoint(FVector2D point, const int32 brushSize)
{
    for (auto offX = -brushSize; offX <= brushSize; ++offX) {
        for (auto offY = -brushSize; offY <= brushSize; ++offY) {
            auto brushPixelX = point.X + offX;
            auto brushPixelY = point.Y + offY;
            if (brushPixelX >= 0 && brushPixelX < TextureSize && brushPixelY >= 0 && brushPixelY < TextureSize) {
                UnfoggedData[brushPixelX + brushPixelY * TextureSize] = 1.0f;
            }
        }
    }
}

void FogWorker::FloodFill(int32 x, int32 y)
{
    if (x < 0 || x >= TextureSize || y < 0 || y >= TextureSize) {
        return;
    }

    // Wikipedia
    // Flood-fill (node, target-color, replacement-color):
    // 1. If target - color is equal to replacement - color, return.
    // 2. If color of node is not equal to target - color, return.
    if (FMath::IsNearlyEqual(UnfoggedData[x + y * TextureSize], 1.0f)) {
        return;
    }
    // 3. Set Q to the empty queue.
    TQueue<FIntVector> Q;
    // 4. Add node to Q.
    Q.Enqueue(FIntVector(x, y, 0));

    FIntVector N;
    // 5. For each element N of Q :
    while (Q.Dequeue(N)) {
        // 6. Set w and e equal to N.
        auto w = N, e = N;
        // 7. Move w to the west until the color of the node to the west of w no longer matches target - color.
        while (w.X - 1 > 0 && !FMath::IsNearlyEqual(UnfoggedData[w.X - 1 + w.Y * TextureSize], 1.0f)) {
            w.X--;
        }
        // 8. Move e to the east until the color of the node to the east of e no longer matches target - color.
        while (e.X + 1 < TextureSize && !FMath::IsNearlyEqual(UnfoggedData[e.X + 1 + e.Y * TextureSize], 1.0f)) {
            e.X++;
        }
        // 9. For each node n between w and e :
        for (auto i = w.X; i <= e.X; ++i) {
            FIntVector n(i, N.Y, 0);
            // 10. Set the color of n to replacement - color.
            UnfoggedData[n.X + n.Y * TextureSize] = 1.0f;
            // 11. If the color of the node to the north of n is target - color, add that node to Q.
            if (n.Y + 1 < TextureSize && !FMath::IsNearlyEqual(UnfoggedData[n.X + (n.Y + 1) * TextureSize], 1.0f))
                Q.Enqueue(FIntVector(n.X, n.Y + 1, 0));
            // 12. If the color of the node to the south of n is target - color, add that node to Q.
            if (n.Y - 1 > 0 && !FMath::IsNearlyEqual(UnfoggedData[n.X + (n.Y - 1) * TextureSize], 1.0f)) {
                Q.Enqueue(FIntVector(n.X, n.Y - 1, 0));
            }
        }
        // 13. Continue looping until Q is exhausted.
    }
    // 14. Return.
}
