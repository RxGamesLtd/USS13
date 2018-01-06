// The MIT License (MIT)
// Copyright (c) 2018 RxCompile
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

#pragma once

#include <RHI.h>

#include "FogWorker.h"

#include "FogPlane.generated.h"

UCLASS()
class FOGOFWARMODULE_API AFogPlane : public AActor
{
    GENERATED_BODY()

public:
    AFogPlane();

    // Called when the game starts or when spawned
    void BeginPlay() override;

    // Called when the game ends
    void EndPlay(const EEndPlayReason::Type) override;

    // Called every frame
    void Tick(float DeltaSeconds) override;

    // Triggers a update in the blueprint
    UFUNCTION(BlueprintNativeEvent)
    void OnFowTextureUpdated(UTexture2D* currentTexture,
                             UTexture2D* lastTexture,
                             FVector cameraLocation,
                             FVector lastCameraLocation);

    // Register an actor to influence the FOW-texture
    UFUNCTION(BlueprintCallable, Category = FogOfWar)
    void RegisterFowActor(AActor* Actor);

    // UnRegister an actor to influence the FOW-texture
    UFUNCTION(BlueprintCallable, Category = FogOfWar)
    void UnRegisterFowActor(AActor* Actor);

    // How far will an actor be able to see
    // CONSIDER: Place it on the actors to allow for individual sight-radius
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogOfWar)
    float SightRange = 9.0f;

    // Check to see if we have a new FOW-texture.
    FThreadSafeBool bHasFOWTextureUpdate = false;

    UPROPERTY(EditAnywhere, Category = FogOfWar)
    float SecondsToForget = 1.0f;

    // The size of our textures
    UPROPERTY(EditAnywhere, Category = FogOfWar)
    int32 TextureSize = 512;

    // Our texture data
    TArray<FColor> TextureData;

    // Texture rendered for this position
    FVector CameraPosition;

    // Store the actors that will be unveiling the FOW-texture.
    UPROPERTY(VisibleAnywhere, Category = FogOfWar)
    TArray<AActor*> Observers;

    UPROPERTY(EditAnywhere, Category = FogOfWar)
    UMaterialInterface* FogMaterial;

    UPROPERTY()
    UStaticMeshComponent* Plane;

private:
    // Our texture data from the last frame
    TArray<FColor> LastFrameTextureData;

    FVector LastCameraPosition;

    // Stolen from https://wiki.unrealengine.com/Dynamic_Textures
    void UpdateTextureRegions(UTexture2D* Texture,
                              int32 MipIndex,
                              uint32 NumRegions,
                              FUpdateTextureRegion2D* Regions,
                              uint32 SrcPitch,
                              uint32 SrcBpp,
                              uint8* SrcData,
                              bool bFreeData) const;

    UPROPERTY(transient)
    UMaterialInstanceDynamic* DynMaterial;

    // Our dynamically updated texture
    UPROPERTY(transient, VisibleAnywhere)
    UTexture2D* FOWTexture;

    // Texture from last update. We blend between the two to do a smooth unveiling of newly discovered areas.
    UPROPERTY(transient)
    UTexture2D* LastFOWTexture;

    // Texture regions
    FUpdateTextureRegion2D TextureRegions;

    // Our fowupdatethread
    TUniquePtr<FogWorker> FowThread;

    // If the last texture blending is done
    bool bIsDoneBlending = false;

    float BlendDelta = 0.0f;
};
