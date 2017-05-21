#include "FogOfWarManager.h"
#include "EngineMinimal.h"

AFogOfWarManager::AFogOfWarManager(const FObjectInitializer& FOI)
    : Super(FOI)
    , bIsDoneBlending(false)
    , FOWTexture(nullptr)
    , LastFOWTexture(nullptr)
    , FowThread(nullptr)
{
    PrimaryActorTick.bCanEverTick = true;
    textureRegions = new FUpdateTextureRegion2D(0U, 0U, 0, 0, TextureSize, TextureSize);

    //15 Gaussian samples. Sigma is 2.0.
    //CONSIDER: Calculate the kernel instead, more flexibility...
    blurKernel.Init(0.0f, blurKernelSize);
    blurKernel[0] = 0.000489f;
    blurKernel[1] = 0.002403f;
    blurKernel[2] = 0.009246f;
    blurKernel[3] = 0.02784f;
    blurKernel[4] = 0.065602f;
    blurKernel[5] = 0.120999f;
    blurKernel[6] = 0.174697f;
    blurKernel[7] = 0.197448f;
    blurKernel[8] = 0.174697f;
    blurKernel[9] = 0.120999f;
    blurKernel[10] = 0.065602f;
    blurKernel[11] = 0.02784f;
    blurKernel[12] = 0.009246f;
    blurKernel[13] = 0.002403f;
    blurKernel[14] = 0.000489f;
}

AFogOfWarManager::~AFogOfWarManager()
{
    if (FowThread) {
        FowThread->ShutDown();
        delete FowThread;
    }
    delete textureRegions;
}

void AFogOfWarManager::BeginPlay()
{
    Super::BeginPlay();
    Observers.Reset();
    bIsDoneBlending = true;
    StartFOWTextureUpdate();
}

void AFogOfWarManager::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (FOWTexture && LastFOWTexture && bHasFOWTextureUpdate && bIsDoneBlending) {
        // Set initial state
        bHasFOWTextureUpdate = false;
        bIsDoneBlending = false;

        auto srcPitch = 4U * TextureSize;
        auto* srcData = reinterpret_cast<uint8*>(LastFrameTextureData.GetData());
        auto* destData = reinterpret_cast<uint8*>(TextureData.GetData());

        // Refresh textures
        LastFOWTexture->UpdateResource();
        UpdateTextureRegions(LastFOWTexture, 0, 1U, textureRegions, srcPitch, 4U, srcData, false);
        FOWTexture->UpdateResource();
        UpdateTextureRegions(FOWTexture, 0, 1U, textureRegions, srcPitch, 4U, destData, false);

        //Trigger the blueprint update
        OnFowTextureUpdated(FOWTexture, LastFOWTexture, CameraPosition, LastCameraPosition);
    }
}

void AFogOfWarManager::StartFOWTextureUpdate()
{
    if (!FOWTexture) {
        FOWTexture = UTexture2D::CreateTransient(TextureSize, TextureSize);
        LastFOWTexture = UTexture2D::CreateTransient(TextureSize, TextureSize);
        int arraySize = TextureSize * TextureSize;
        TextureData.Init(FColor(0, 0, 0, 255), arraySize);
        LastFrameTextureData.Init(FColor(0, 0, 0, 255), arraySize);
        HorizontalBlurData.Init(0, arraySize);
        UnfoggedData.Init(0.0f, arraySize);

        if (FowThread) {
            FowThread->ShutDown();
            delete FowThread;
            FowThread = nullptr;
        }
        FowThread = new FogOfWarWorker(this);
    }
}

void AFogOfWarManager::OnFowTextureUpdated_Implementation(UTexture2D* currentTexture, UTexture2D* lastTexture, FVector cameraLocation, FVector lastCameraLocation)
{
    //Handle in blueprint
}

void AFogOfWarManager::RegisterFowActor(AActor* Actor)
{
    Observers.AddUnique(Actor);
}

void AFogOfWarManager::UnRegisterFowActor(AActor* Actor)
{
    Observers.Remove(Actor);
}

bool AFogOfWarManager::IsBlurEnabled() const
{
    return bIsBlurEnabled;
}

void AFogOfWarManager::UpdateTextureRegions(UTexture2D* Texture, int32 MipIndex, uint32 NumRegions,
    FUpdateTextureRegion2D* Regions, uint32 SrcPitch, uint32 SrcBpp,
    uint8* SrcData, bool bFreeData) const
{
    if (Texture && Texture->Resource) {
        struct FUpdateTextureRegionsData {
            FTexture2DResource* Texture2DResource;
            int32 MipIndex;
            uint32 NumRegions;
            FUpdateTextureRegion2D* Regions;
            uint32 SrcPitch;
            uint32 SrcBpp;
            uint8* SrcData;
        };

        FUpdateTextureRegionsData* RegionData = new FUpdateTextureRegionsData;

        RegionData->Texture2DResource = static_cast<FTexture2DResource*>(Texture->Resource);
        RegionData->MipIndex = MipIndex;
        RegionData->NumRegions = NumRegions;
        RegionData->Regions = Regions;
        RegionData->SrcPitch = SrcPitch;
        RegionData->SrcBpp = SrcBpp;
        RegionData->SrcData = SrcData;

        ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
            UpdateTextureRegionsData,
            FUpdateTextureRegionsData*, RegionData, RegionData,
            bool, bFreeData, bFreeData,
            {
                for (uint32 RegionIndex = 0; RegionIndex < RegionData->NumRegions; ++RegionIndex) {
                    int32 CurrentFirstMip = RegionData->Texture2DResource->GetCurrentFirstMip();
                    if (RegionData->MipIndex >= CurrentFirstMip) {
                        RHIUpdateTexture2D(
                            RegionData->Texture2DResource->GetTexture2DRHI(),
                            RegionData->MipIndex - CurrentFirstMip,
                            RegionData->Regions[RegionIndex],
                            RegionData->SrcPitch,
                            RegionData->SrcData
                                + RegionData->Regions[RegionIndex].SrcY * RegionData->SrcPitch
                                + RegionData->Regions[RegionIndex].SrcX * RegionData->SrcBpp);
                    }
                }
                if (bFreeData) {
                    FMemory::Free(RegionData->Regions);
                    FMemory::Free(RegionData->SrcData);
                }
                delete RegionData;
            });
    }
}
