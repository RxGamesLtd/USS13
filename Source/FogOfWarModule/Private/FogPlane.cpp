#include "FogPlane.h"

// Sets default values
AFogPlane::AFogPlane(const FObjectInitializer& FOI)
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    TextureRegions = FUpdateTextureRegion2D(0U, 0U, 0, 0, TextureSize, TextureSize);

    FOWTexture = UTexture2D::CreateTransient(TextureSize, TextureSize);
    FOWTexture->UpdateResource();
    FOWTexture->AddressX = TextureAddress::TA_Clamp;
    FOWTexture->AddressY = TextureAddress::TA_Clamp;
    FOWTexture->Filter = TextureFilter::TF_Default;
    FOWTexture->RefreshSamplerStates();
    LastFOWTexture = UTexture2D::CreateTransient(TextureSize, TextureSize);
    LastFOWTexture->UpdateResource();
    LastFOWTexture->AddressX = TextureAddress::TA_Clamp;
    LastFOWTexture->AddressY = TextureAddress::TA_Clamp;
    LastFOWTexture->Filter = TextureFilter::TF_Default;
    LastFOWTexture->RefreshSamplerStates();

    static ConstructorHelpers::FObjectFinder<UStaticMesh> StaticMeshOb_AW2(TEXT("StaticMesh'/EngineContent/BasicShapes/Cube.Cube'"));
    Plane = FOI.CreateDefaultSubobject<UStaticMeshComponent>(this, FName(TEXT("Plane")));
    Plane->SetStaticMesh(StaticMeshOb_AW2.Object);

    DynMaterial = Plane->CreateAndSetMaterialInstanceDynamicFromMaterial(0, FogMaterial);
    DynMaterial->SetScalarParameterValue("Blend", 0.f);
}

// Called when the game starts or when spawned
void AFogPlane::BeginPlay()
{
    Super::BeginPlay();
    FowThread.Reset(new FogWorker(*this));
}

void AFogPlane::EndPlay(const EEndPlayReason::Type reason)
{
    FowThread = nullptr;
    Observers.Empty();
}

// Called every frame
void AFogPlane::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (!bIsDoneBlending) {
        BlendDelta += DeltaTime;

        FMath::Clamp(BlendDelta, 0.f, 1.f);
        DynMaterial->SetScalarParameterValue("Blend", BlendDelta);

        if (FMath::IsNearlyEqual(BlendDelta, 1.f)) {
            bIsDoneBlending = true;
            BlendDelta = 0.f;
        }
    }

    if (FOWTexture && LastFOWTexture && bHasFOWTextureUpdate && bIsDoneBlending) {
        // Move to render location
        SetActorLocation(CameraPosition, false, nullptr, ETeleportType::TeleportPhysics);

        // Set initial state
        bHasFOWTextureUpdate = false;
        bIsDoneBlending = false;

        auto srcBpp = 4U;
        auto srcPitch = srcBpp * TextureSize;
        auto* lastData = reinterpret_cast<uint8*>(LastFrameTextureData.GetData());
        auto* currentData = reinterpret_cast<uint8*>(TextureData.GetData());

        // Refresh textures
        LastFOWTexture->UpdateResource();
        UpdateTextureRegions(LastFOWTexture, 0, 1U, &TextureRegions, srcPitch, srcBpp, lastData, false);
        FOWTexture->UpdateResource();
        UpdateTextureRegions(FOWTexture, 0, 1U, &TextureRegions, srcPitch, srcBpp, currentData, false);

        // Trigger the blueprint update
        OnFowTextureUpdated(FOWTexture, LastFOWTexture, CameraPosition, LastCameraPosition);

        // Store last used data
        LastFrameTextureData = TextureData;
        LastCameraPosition = CameraPosition;
    }
}

void AFogPlane::OnFowTextureUpdated_Implementation(UTexture2D* currentTexture, UTexture2D* lastTexture, FVector cameraLocation, FVector lastCameraLocation)
{
    //Handle in blueprint
}

void AFogPlane::RegisterFowActor(AActor* Actor)
{
    Observers.AddUnique(Actor);
}

void AFogPlane::UnRegisterFowActor(AActor* Actor)
{
    Observers.Remove(Actor);
}

void AFogPlane::UpdateTextureRegions(UTexture2D* Texture, int32 MipIndex, uint32 NumRegions,
    FUpdateTextureRegion2D* Regions, uint32 SrcPitch, uint32 SrcBpp,
    uint8* SrcData, bool bFreeData) const
{
    if (!Texture || !Texture->Resource) {
        return;
    }
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
