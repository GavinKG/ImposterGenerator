// Fill out your copyright notice in the Description page of Project Settings.


#include "ImposterCaptureActor.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstance.h"


// Sets default values
AImposterCaptureActor::AImposterCaptureActor()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	SceneCaptureComponent2D = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture"));
	SceneCaptureComponent2D->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

	SceneCaptureComponent2D->ProjectionType = ECameraProjectionMode::Orthographic;

	TArray<FEngineShowFlagsSetting> ShowFlagSettings;
	FEngineShowFlagsSetting AOShowFlags;
	AOShowFlags.ShowFlagName = TEXT("AmbientOcclusion");
	ShowFlagSettings.Add(AOShowFlags);
	FEngineShowFlagsSetting FogShowFlags;
	FogShowFlags.ShowFlagName = TEXT("Fog");
	ShowFlagSettings.Add(FogShowFlags);
	FEngineShowFlagsSetting AtmosphericFogShowFlags;
	AtmosphericFogShowFlags.ShowFlagName = TEXT("AtmosphericFog");
	ShowFlagSettings.Add(AtmosphericFogShowFlags);
	SceneCaptureComponent2D->ShowFlagSettings = ShowFlagSettings;
	SceneCaptureComponent2D->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	SceneCaptureComponent2D->bCaptureEveryFrame = 0; // we will trigger the capture manually.
	SceneCaptureComponent2D->bCaptureOnMovement = 0;
	SceneCaptureComponent2D->bAlwaysPersistRenderingState = true; // Set it to true to preserve GBuffer for post-processing material.

	// SceneCaptureComponent2D are not configured properly right now. Further init will be done inside RegisterActorInfo
}

void AImposterCaptureActor::SetCaptureTransform(const FVector& Location, const FRotator& Rotation)
{
	SceneCaptureComponent2D->SetWorldLocationAndRotation(Location, Rotation);
}

void AImposterCaptureActor::RegisterActorInfo(const TArray<AActor*>& Actors, const FBoxSphereBounds& Bounds)
{
	SceneCaptureComponent2D->ShowOnlyActors = Actors;
	SceneCaptureComponent2D->OrthoWidth = Bounds.SphereRadius * 2.0f;
	SceneCaptureComponent2D->MaxViewDistanceOverride = Bounds.SphereRadius * 2.0f; // make scene depth work.
}

void AImposterCaptureActor::SetPostProcessingMaterial(UMaterialInterface* Material)
{
	check(SceneCaptureComponent2D);
	SceneCaptureComponent2D->PostProcessSettings.WeightedBlendables.Array.Reset();
	if (!Material)
	{
		return;
	}
	FWeightedBlendable Blendable;
	Blendable.Object = Material; // UMaterialInstance implemented IBlendableInterface
	Blendable.Weight = 1;
	SceneCaptureComponent2D->PostProcessSettings.WeightedBlendables.Array.Add(Blendable);
}

void AImposterCaptureActor::Capture(ESceneCaptureSource CaptureSource, UTextureRenderTarget2D* RT)
{
	SceneCaptureComponent2D->CaptureSource = CaptureSource;
	SceneCaptureComponent2D->TextureTarget = RT;
	SceneCaptureComponent2D->CaptureScene();
}

// Called when the game starts or when spawned
void AImposterCaptureActor::BeginPlay()
{
	Super::BeginPlay();
}
