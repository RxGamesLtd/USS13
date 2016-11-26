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

#pragma once

#include "RHI.h"
#include "GameFramework/Actor.h"
#include "FogOfWarWorker.h"
#include "FogOfWarManager.generated.h"

UCLASS()

class SS13REMAKE_API AFogOfWarManager : public AActor {
    GENERATED_BODY()

public:
    AFogOfWarManager(const FObjectInitializer &FOI);

    virtual ~AFogOfWarManager();

    void BeginPlay() override;

    void Tick(float DeltaSeconds) override;

    //Triggers a update in the blueprint
    UFUNCTION(BlueprintNativeEvent)

    void OnFowTextureUpdated(UTexture2D *currentTexture, UTexture2D *lastTexture);

    //Register an actor to influence the FOW-texture
    UFUNCTION(BlueprintCallable, Category = FogOfWar)

    void RegisterFowActor(AActor *Actor);

    //Register an actor to influence the FOW-texture
    UFUNCTION(BlueprintCallable, Category = FogOfWar)

    void UnRegisterFowActor(AActor *Actor);

    //Stolen from https://wiki.unrealengine.com/Dynamic_Textures
    void UpdateTextureRegions(
            UTexture2D *Texture,
            int32 MipIndex,
            uint32 NumRegions,
            FUpdateTextureRegion2D *Regions,
            uint32 SrcPitch,
            uint32 SrcBpp,
            uint8 *SrcData,
            bool bFreeData) const;

    //How far will an actor be able to see
    //CONSIDER: Place it on the actors to allow for individual sight-radius
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogOfWar)
    float SightRange = 9.0f;

    //The number of samples per 100 unreal units
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogOfWar)
    float SamplesPerMeter = 2.0f;

    //If the last texture blending is done
    UPROPERTY(BlueprintReadWrite)
    bool bIsDoneBlending;

    //Should we blur? It takes up quite a lot of CPU time...
    UPROPERTY(EditAnywhere, Category = FogOfWar)
    bool bIsBlurEnabled = true;

    UPROPERTY(EditAnywhere, Category = FogOfWar)
    float SecondsToForget = 1.0f;

    //The size of our textures
    uint32 TextureSize = 1024;

    //Array containing what parts of the map we've unveiled.
    UPROPERTY()
    TArray<float> UnfoggedData;

    //Temp array for horizontal blur pass
    UPROPERTY()
    TArray<uint8> HorizontalBlurData;

    //Our texture data (result of vertical blur pass)
    UPROPERTY()
    TArray<FColor> TextureData;

    //Our texture data from the last frame
    UPROPERTY()
    TArray<FColor> LastFrameTextureData;

    //Check to see if we have a new FOW-texture.
    bool bHasFOWTextureUpdate = false;

    //Blur size
    uint8 blurKernelSize = 15;

    //Blur kernel
    UPROPERTY()
    TArray<float> blurKernel;

    //Store the actors that will be unveiling the FOW-texture.
    UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = FogOfWar)
    TArray<AActor *> Observers;

    //DEBUG: Time it took to update the fow texture
    float fowUpdateTime = 0;

    //Getter for the working thread
    bool IsBlurEnabled() const;

private:
    //Triggers the start of a new FOW-texture-update
    void StartFOWTextureUpdate();

    //Our dynamically updated texture
    UPROPERTY()
    UTexture2D *FOWTexture;

    //Texture from last update. We blend between the two to do a smooth unveiling of newly discovered areas.
    UPROPERTY()
    UTexture2D *LastFOWTexture;

    //Texture regions
    FUpdateTextureRegion2D *textureRegions;

    //Our fowupdatethread
    FogOfWarWorker *FowThread;
};
