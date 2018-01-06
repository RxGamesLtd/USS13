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

#include "FogPlane.h"

AFogPlane::AFogPlane()
{
    PrimaryActorTick.bCanEverTick = true;

    TextureRegions = FUpdateTextureRegion2D(0U, 0U, 0, 0, TextureSize, TextureSize);

    TextureData.AddZeroed(TextureSize * TextureSize);
    LastFrameTextureData.AddZeroed(TextureData.Num());

    static ConstructorHelpers::FObjectFinder<UStaticMesh> StaticMeshOb_AW2(
      TEXT("StaticMesh'/Engine/BasicShapes/Plane.Plane'"));
    Plane = CreateDefaultSubobject<UStaticMeshComponent>(FName(TEXT("Plane")));
    Plane->SetStaticMesh(StaticMeshOb_AW2.Object);
    Plane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AFogPlane::BeginPlay()
{
    Super::BeginPlay();
    if(FogMaterial->IsValidLowLevel())
    {
        DynMaterial = Plane->CreateAndSetMaterialInstanceDynamicFromMaterial(0, FogMaterial);
        DynMaterial->SetScalarParameterValue("Blend", 0.f);
    }

    FOWTexture = UTexture2D::CreateTransient(TextureSize, TextureSize);
    FOWTexture->UpdateResource();
    FOWTexture->AddressX = TA_Clamp;
    FOWTexture->AddressY = TA_Clamp;
    FOWTexture->Filter = TF_Default;
    FOWTexture->RefreshSamplerStates();

    LastFOWTexture = UTexture2D::CreateTransient(TextureSize, TextureSize);
    LastFOWTexture->UpdateResource();
    LastFOWTexture->AddressX = TA_Clamp;
    LastFOWTexture->AddressY = TA_Clamp;
    LastFOWTexture->Filter = TF_Default;
    LastFOWTexture->RefreshSamplerStates();

    FowThread = MakeUnique<FogWorker>(*this);
}

void AFogPlane::EndPlay(const EEndPlayReason::Type reason)
{
    FowThread = nullptr;
    Observers.Empty();
}

void AFogPlane::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if(!FOWTexture)
        return;
    if(!LastFOWTexture)
        return;
    if(!DynMaterial)
        return;

    if(!bIsDoneBlending)
    {
        BlendDelta += DeltaTime * 10.0f;

        BlendDelta = FMath::Clamp(BlendDelta, 0.0f, 1.0f);
        DynMaterial->SetScalarParameterValue("Blend", BlendDelta);

        if(FMath::IsNearlyEqual(BlendDelta, 1.0f))
        {
            bIsDoneBlending = true;
            BlendDelta = 1.0f;
        }
    }

    if(bHasFOWTextureUpdate && bIsDoneBlending)
    {
        // Move to render location
        SetActorLocation(CameraPosition, false, nullptr, ETeleportType::TeleportPhysics);

        // Set initial state
        bHasFOWTextureUpdate = false;
        bIsDoneBlending = false;

        // Prepare data to render on GPU
        auto srcBpp = 4U;
        auto srcPitch = srcBpp * TextureSize;
        auto lastData = reinterpret_cast<uint8*>(LastFrameTextureData.GetData());
        auto currentData = reinterpret_cast<uint8*>(TextureData.GetData());

        // Refresh textures
        LastFOWTexture->UpdateResource();
        UpdateTextureRegions(LastFOWTexture, 0, 1U, &TextureRegions, srcPitch, srcBpp, lastData, false);
        FOWTexture->UpdateResource();
        UpdateTextureRegions(FOWTexture, 0, 1U, &TextureRegions, srcPitch, srcBpp, currentData, false);

        // Trigger the blueprint update
        OnFowTextureUpdated(FOWTexture, LastFOWTexture, CameraPosition, LastCameraPosition);

        BlendDelta = 0.0f;

        DynMaterial->SetScalarParameterValue("Blend", BlendDelta);
        DynMaterial->SetTextureParameterValue("FOWTexture", FOWTexture);
        DynMaterial->SetTextureParameterValue("LastFOWTexture", LastFOWTexture);
        DynMaterial->SetVectorParameterValue("CameraPositon", CameraPosition);
        DynMaterial->SetVectorParameterValue("LastCameraPositon", LastCameraPosition);

        // Store last used data
        LastFrameTextureData = TextureData;
        LastCameraPosition = CameraPosition;
    }
}

void AFogPlane::OnFowTextureUpdated_Implementation(UTexture2D* currentTexture,
                                                   UTexture2D* lastTexture,
                                                   FVector cameraLocation,
                                                   FVector lastCameraLocation)
{
    // Handle in blueprint
}

void AFogPlane::RegisterFowActor(AActor* Actor)
{
    Observers.AddUnique(Actor);
}

void AFogPlane::UnRegisterFowActor(AActor* Actor)
{
    Observers.Remove(Actor);
}

void AFogPlane::UpdateTextureRegions(UTexture2D* Texture,
                                     int32 MipIndex,
                                     uint32 NumRegions,
                                     FUpdateTextureRegion2D* Regions,
                                     uint32 SrcPitch,
                                     uint32 SrcBpp,
                                     uint8* SrcData,
                                     bool bFreeData) const
{
    if(!Texture || !Texture->Resource)
    {
        return;
    }
    struct FUpdateTextureRegionsData
    {
        FTexture2DResource* Texture2DResource;
        int32 MipIndex;
        uint32 NumRegions;
        FUpdateTextureRegion2D* Regions;
        uint32 SrcPitch;
        uint32 SrcBpp;
        uint8* SrcData;
    };

    auto RegionData = new FUpdateTextureRegionsData;
    RegionData->Texture2DResource = static_cast<FTexture2DResource*>(Texture->Resource);
    RegionData->MipIndex = MipIndex;
    RegionData->NumRegions = NumRegions;
    RegionData->Regions = Regions;
    RegionData->SrcPitch = SrcPitch;
    RegionData->SrcBpp = SrcBpp;
    RegionData->SrcData = SrcData;

    ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
      UpdateTextureRegionsData, FUpdateTextureRegionsData*, RegionData, RegionData, bool, bFreeData, bFreeData, {
          for(uint32 RegionIndex = 0; RegionIndex < RegionData->NumRegions; ++RegionIndex)
          {
              int32 CurrentFirstMip = RegionData->Texture2DResource->GetCurrentFirstMip();
              if(RegionData->MipIndex >= CurrentFirstMip)
              {
                  RHIUpdateTexture2D(RegionData->Texture2DResource->GetTexture2DRHI(),
                                     RegionData->MipIndex - CurrentFirstMip,
                                     RegionData->Regions[RegionIndex],
                                     RegionData->SrcPitch,
                                     RegionData->SrcData +
                                       RegionData->Regions[RegionIndex].SrcY * RegionData->SrcPitch +
                                       RegionData->Regions[RegionIndex].SrcX * RegionData->SrcBpp);
              }
          }
          if(bFreeData)
          {
              FMemory::Free(RegionData->Regions);
              FMemory::Free(RegionData->SrcData);
          }
          delete RegionData;
      });
}
