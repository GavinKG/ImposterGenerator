// malosgao presents.


#include "ImposterCore.h"

#include "AssetToolsModule.h"
#include "DrawDebugHelpers.h"
#include "EditorLevelUtils.h"
#include "IAssetTools.h"
#include "ImposterActor.h"
#include "ImposterCaptureActor.h"
#include "PackageTools.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Canvas.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY(LogImposterGenerator)

FImposterGeneratorSettings::FImposterGeneratorSettings()
{
	// find default value

	if (!UnitQuad)
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> UnitQuadObj(TEXT("/Engine/BasicShapes/Plane"));
		if (UnitQuadObj.Succeeded())
		{
			UnitQuad = UnitQuadObj.Object;
		}
	}
}

UImposterCore::UImposterCore()
{
	UE_LOG(LogImposterGenerator, Display, TEXT("ImposterCore created."));
}

bool UImposterCore::CanAddActorToCapture(const AActor* Actor)
{
	if (!Actor)
	{
		return false;
	}

	TArray<UStaticMeshComponent*> StaticMeshComponents;
	Actor->GetComponents(StaticMeshComponents, false);

	bool bFoundValidComponent = false;
	for (const UStaticMeshComponent* StaticMeshComponent : StaticMeshComponents)
	{
		if (!StaticMeshComponent->bHiddenInGame && StaticMeshComponent->IsVisible())
		{
			bFoundValidComponent = true;
			break;
		}
	}

	return bFoundValidComponent;
}

bool UImposterCore::TryAddCapturedActor(AActor* Actor)
{
	if (CanAddActorToCapture(Actor))
	{
		CapturedActors.Add(Actor);
		return true;
	}
	return false;
}

void UImposterCore::RemoveAllCapturedActors()
{
	CapturedActors.Reset();
}

bool UImposterCore::Init(const FImposterGeneratorSettings& ImposterGeneratorSettings,
                         const TArray<AActor*>& CapturedActorArray,
                         const FString& ImposterNameString)
{
	// COPY all parameters to core.
	this->Settings = ImposterGeneratorSettings;
	this->CapturedActors = CapturedActorArray;
	this->ImposterName = ImposterNameString;

	if (!ShouldInit())
	{
		UE_LOG(LogImposterGenerator, Error, TEXT("Initialization failed. See errors above."));
		return false;
	}

	InitTex();
	InitCaptureBounds();
	InitCaptureDirections();
	InitMpc();

	ImposterCoreState = EImposterCoreState::Initialized;
	UE_LOG(LogImposterGenerator, Display, TEXT("Initialization finished."));
	return true;
}

// Returned direction vector is normalized.
FVector UImposterCore::ImposterGridToDirVector(FVector2D Grid) const
{
	Grid = Grid * 2 - 1;
	switch (Settings.ImposterType)
	{
	case EImposterSphereType::UpperHemi:
		return HemiOctahedronToDirVector(Grid);
	case EImposterSphereType::Full:
		return OctahedronToDirVector(Grid);
	}
	check(0);
	return FVector::ZeroVector;
}

FVector UImposterCore::OctahedronToDirVector(const FVector2D& Grid)
{
	// Same as Blueprint "Octahedron to Vector"
	const FVector2D GridAbs(FMath::Abs(Grid.X), FMath::Abs(Grid.Y));
	const float T1 = 1 - FVector2D::DotProduct(GridAbs, FVector2D::UnitVector);
	if (T1 >= 0)
	{
		FVector Ret(Grid.X, Grid.Y, T1);
		Ret.Normalize(0.0001f);
		return Ret;
	}
	else
	{
		FVector Ret((Grid.X >= 0 ? 1 : -1) * (1 - GridAbs.Y), (Grid.Y >= 0 ? 1 : -1) * (1 - GridAbs.X), T1);
		Ret.Normalize(0.0001f);
		return Ret;
	}
}

FVector UImposterCore::HemiOctahedronToDirVector(const FVector2D& Grid)
{
	// Same as Blueprint "Hemi Octahedron to Unit Vector"
	FVector2D Grid2(Grid.X + Grid.Y, Grid.X - Grid.Y);
	Grid2 *= 0.5f;
	const float Z = 1 - FVector2D::DotProduct(FVector2D(FMath::Abs(Grid2.X), FMath::Abs(Grid2.Y)),
	                                          FVector2D::UnitVector);
	FVector Ret(Grid2.X, Grid2.Y, Z);
	Ret.Normalize(0.0001f);
	return Ret;
}

void UImposterCore::InitTex()
{
	CapturedTexMap.Reset();
	for (EImposterCaptureType CaptureType : Settings.CaptureTextureTypes)
	{
		UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(this);
		check(RT);
		RT->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
		RT->ClearColor = FLinearColor::Transparent;
		RT->bAutoGenerateMips = false;
		RT->InitAutoFormat((Settings.ImposterTextureRes),
		                   (Settings.ImposterTextureRes));
		RT->UpdateResourceImmediate(true);
		CapturedTexMap.Add(CaptureType, RT);
	}

	MiddleRT = NewObject<UTextureRenderTarget2D>(this);
	check(MiddleRT);
	MiddleRT->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
	MiddleRT->bAutoGenerateMips = false;
	MiddleRT->InitAutoFormat(Settings.ImposterTextureRes / Settings.GridSize,
	                         Settings.ImposterTextureRes / Settings.GridSize);
	// todo: supersampling?
	MiddleRT->ClearColor = FLinearColor::Transparent;
	MiddleRT->UpdateResourceImmediate(true);
}

void UImposterCore::InitCaptureBounds()
{
	UGameplayStatics::GetActorArrayBounds(CapturedActors, false, CaptureBounds.Origin,
	                                      CaptureBounds.BoxExtent);
	CaptureBounds.SphereRadius = CaptureBounds.BoxExtent.Size();
	UE_LOG(LogImposterGenerator, Display, TEXT("Capture Bounds generated with origin %s, extent %s, radius %f"),
	       *CaptureBounds.Origin.ToString(), *CaptureBounds.BoxExtent.ToString(), CaptureBounds.SphereRadius);
}

void UImposterCore::InitCaptureDirections()
{
	CaptureDirections.Reset();
	for (uint32 Y = 0; Y < Settings.GridSize; ++Y)
	{
		for (uint32 X = 0; X < Settings.GridSize; ++X)
		{
			FVector2D Loc(static_cast<float>(X) / (Settings.GridSize - 1),
			              static_cast<float>(Y) / (Settings.GridSize - 1));
			CaptureDirections.Add(ImposterGridToDirVector(Loc));
		}
	}
}

void UImposterCore::InitMpc()
{
	UKismetMaterialLibrary::SetScalarParameterValue(GetWorld(), Settings.CaptureMpc, FName(TEXT("CaptureRadius")),
	                                                CaptureBounds.SphereRadius);
	UKismetMaterialLibrary::SetVectorParameterValue(GetWorld(), Settings.CaptureMpc, FName(TEXT("CaptureOrigin")),
	                                                CaptureBounds.Origin);
	UKismetMaterialLibrary::SetVectorParameterValue(GetWorld(), Settings.CaptureMpc, FName(TEXT("CaptureExtent")),
	                                                CaptureBounds.BoxExtent);
	UKismetMaterialLibrary::SetScalarParameterValue(GetWorld(), Settings.CaptureMpc, FName(TEXT("TextureSize")),
	                                                static_cast<float>(Settings.ImposterTextureRes));
}

bool UImposterCore::ShouldInit(bool bShouldLog) const
{
	bool bResult = true;

	if (CapturedActors.Num() == 0)
	{
		if (bShouldLog)
		{
			UE_LOG(LogImposterGenerator, Error, TEXT("No actors to capture."));
		}
		bResult = false;
	}

	if (Settings.CaptureTextureTypes.Num() == 0)
	{
		if (bShouldLog)
		{
			UE_LOG(LogImposterGenerator, Error, TEXT("Capture texture type not specified."));
		}
		bResult = false;
	}

	if (Settings.ImposterTextureRes % 2u != 0)
	{
		if (bShouldLog)
		{
			UE_LOG(LogImposterGenerator, Error, TEXT("ImposterTextureRes should be power of 2."));
		}
		bResult = false;
	}

	if (bShouldLog && !Settings.ImposterMaterial)
	{
		UE_LOG(LogImposterGenerator, Warning,
		       TEXT("Imposter material not assigned properly. Imposter MIC might not be generated."));
	}

	if (bShouldLog && Settings.ImposterMaterialTextureBindingMap.Num() == 0)
	{
		UE_LOG(LogImposterGenerator, Warning,
		       TEXT(
			       "Imposter material texture bindings not set properly. Imposter will have no texture to use when exporting and you have to manually assign it."
		       ));
	}

	if (bShouldLog && !Settings.CaptureMpc)
	{
		UE_LOG(LogImposterGenerator, Warning,
		       TEXT(
			       "Capture Material Property Collection not assigned. Materials that uses capture properties may not work proerly."
		       ))
	}


	return bResult;
}

bool UImposterCore::ShouldCapture(bool bShouldLog) const
{
	bool bResult = true;

	if (ImposterCoreState < EImposterCoreState::Initialized)
	{
		if (bShouldLog)
		{
			UE_LOG(LogImposterGenerator, Error, TEXT("Imposter core not initialized."));
			bResult = false;
		}
	}

	return bResult;
}

bool UImposterCore::ShouldGenerate(bool bShouldLog) const
{
	bool bResult = true;

	if (ImposterCoreState < EImposterCoreState::Captured)
	{
		if (bShouldLog)
		{
			UE_LOG(LogImposterGenerator, Error, TEXT("Not captured."));
		}
		bResult = false;
	}

	if (Settings.WorkingPath.IsEmpty())
	{
		if (bShouldLog)
		{
			UE_LOG(LogImposterGenerator, Error,
			       TEXT(
				       "Working path not specified. Working path is the only path required and you did not specify it. Great job dude!"
			       ));
		}
		bResult = false;
	}

	if (CapturedTexMap.Num() == 0)
	{
		if (bShouldLog)
		{
			UE_LOG(LogImposterGenerator, Error, TEXT("Nothing to export."))
		}
		bResult = false;
	}

	return bResult;
}

void UImposterCore::Capture()
{
	if (!ShouldCapture(true))
	{
		UE_LOG(LogImposterGenerator, Error, TEXT("Initialization failed. See above for details."));
		return;
	}

	// Check whether this object is spawned by a world-related object.
	check(GetWorld());

	// Put capture actor to current active level if not spawned.
	if (!ImposterCaptureActor)
	{
		FActorSpawnParameters Params;
		Params.ObjectFlags |= RF_Transient; // should not save with map.
		ImposterCaptureActor = GetWorld()->SpawnActor<AImposterCaptureActor>(Params);
		check(ImposterCaptureActor);
	}

	ImposterCaptureActor->RegisterActorInfo(CapturedActors, CaptureBounds);

	// For each capture type
	for (const EImposterCaptureType CaptureType : Settings.CaptureTextureTypes)
	{
		const ESceneCaptureSource SceneCaptureSource = GetSceneCaptureSourceFrom(CaptureType);

		if (SceneCaptureSource == SCS_FinalColorLDR)
		{
			// Should use a custom pp material.
			UMaterialInterface** PtrToCaptureMaterial = Settings.CaptureMaterialMap.Find(CaptureType);
			if (PtrToCaptureMaterial && *PtrToCaptureMaterial)
			{
				ImposterCaptureActor->SetPostProcessingMaterial(*PtrToCaptureMaterial);
				UE_LOG(LogImposterGenerator, Display, TEXT("Capture %s with post-processing material %s..."),
				       *CaptureTypeEnumToString(CaptureType), *(*PtrToCaptureMaterial)->GetName());
			}
			else
			{
				UE_LOG(LogImposterGenerator, Warning,
				       TEXT("Material for capture type %s could not be found. Capture will be skipped for this type."),
				       *CaptureTypeEnumToString(CaptureType));
				continue;
			}
		}
		else
		{
			ImposterCaptureActor->SetPostProcessingMaterial(nullptr);
			UE_LOG(LogImposterGenerator, Display, TEXT("Capture %s with custom scene capture source..."),
			       *CaptureTypeEnumToString(CaptureType));
		}

		// for each capture direction
		for (int32 i = 0; i < CaptureDirections.Num(); ++i)
		{
			const FVector& Dir = CaptureDirections[i];

			// Transform Scene Capture Component
			FVector Loc = CaptureBounds.Origin + Dir * CaptureBounds.SphereRadius;
			FRotator Rot = FRotationMatrix::MakeFromX(-Dir).Rotator(); // UKismetMathLibrary::MakeRotFromX()
			ImposterCaptureActor->SetCaptureTransform(Loc, Rot);

			// Capture to MiddleRT
			ImposterCaptureActor->Capture(SceneCaptureSource, MiddleRT);

			// Compose final tex
			UTextureRenderTarget2D* CompTex = CapturedTexMap[CaptureType];
			check(CompTex);
			UCanvas* Canvas;
			FVector2D CanvasSize; // whole canvas size, same as ImposterTextureRes
			FDrawToRenderTargetContext CompContext;

			UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(GetWorld(), CompTex,
			                                                       Canvas, CanvasSize, CompContext);

			// draw canvas
			const FVector2D SliceSize = CanvasSize / Settings.GridSize;
			FVector2D SliceOffset(i % Settings.GridSize, i / Settings.GridSize); // in grid coord, [0, GridSize)
			SliceOffset = SliceOffset / Settings.GridSize * CanvasSize;
			Canvas->K2_DrawTexture(MiddleRT, SliceOffset, SliceSize, FVector2D::ZeroVector, FVector2D::UnitVector,
			                       FLinearColor::White, BLEND_Opaque);

			UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), CompContext);
		} // end capture type
	} // end capture direction

	// Apply additional blit
	for (const FImposterBlitSettings& BlitSettings : Settings.AdditionalCaptureBlit)
	{
		// Get Source RT
		UTextureRenderTarget2D** pRT = CapturedTexMap.Find(BlitSettings.CaptureType);
		if (!pRT)
		{
			continue;
		}
		UTextureRenderTarget2D* SrcRT = *pRT;
		check(SrcRT); // CapturedTexMap should not contain null RT.

		// Create Destination RT (ignore performance)
		UTextureRenderTarget2D* DstRT = NewObject<UTextureRenderTarget2D>(this);
		check(DstRT);
		DstRT->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
		DstRT->bAutoGenerateMips = false;
		DstRT->InitAutoFormat(static_cast<uint32>(SrcRT->SizeX),
		                      static_cast<uint32>(SrcRT->SizeY));
		DstRT->UpdateResourceImmediate(false);

		// Draw material.
		UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(BlitSettings.BlitMaterial, this);
		Mid->SetTextureParameterValue(TEXT("Texture"), SrcRT);
		UKismetRenderingLibrary::DrawMaterialToRenderTarget(GetWorld(), DstRT, Mid); // TRY

		// Replace.
		CapturedTexMap[BlitSettings.CaptureType] = DstRT;
	}


	ImposterCoreState = EImposterCoreState::Captured;
	UE_LOG(LogImposterGenerator, Display, TEXT("Capture complete."));
}

void UImposterCore::GenerateImposter(bool bCreateSublevel, const FVector& Offset)
{
	if (!ShouldGenerate())
	{
		UE_LOG(LogImposterGenerator, Error, TEXT("Generate failed. See above for more information."))
		return;
	}

	// Step.1: Export textures (using UKismetRenderingLibrary).
	TMap<EImposterCaptureType, UTexture2D*> ExportedTexMap;
	const FString TextureRootPath = FPaths::Combine(Settings.WorkingPath, ImposterName);
	for (const auto Elem : CapturedTexMap)
	{
		const FString TexturePath = FPaths::Combine(TextureRootPath, FString::Printf(
			                                            TEXT("T_%s_%s"), *ImposterName,
			                                            *CaptureTypeEnumToString(Elem.Key)));
		UTexture2D* ExportedTex = UKismetRenderingLibrary::RenderTargetCreateStaticTexture2DEditorOnly(
			Elem.Value, TexturePath, TC_Default, TMGS_FromTextureGroup);
		ExportedTexMap.Add(Elem.Key, ExportedTex);
	}

	// Step.2: Export Material Instance
	UMaterialInstanceConstant* Mic = CreateMIC_EditorOnly(Settings.ImposterMaterial, GetMaterialPackageName());
	for (const auto& Elem : ExportedTexMap)
	{
		FString* TexParam = Settings.ImposterMaterialTextureBindingMap.Find(Elem.Key);
		if (!TexParam)
		{
			UE_LOG(LogImposterGenerator, Warning, TEXT("%s not found in texture binding map."),
			       *CaptureTypeEnumToString(Elem.Key));
			continue;
		}
		Mic->SetTextureParameterValueEditorOnly(FName(**TexParam), Elem.Value);
	}
	Mic->SetScalarParameterValueEditorOnly(Settings.ImposterMaterialGridSizeParam, Settings.GridSize);
	Mic->SetScalarParameterValueEditorOnly(Settings.ImposterMaterialSizeParam, CaptureBounds.SphereRadius * 2);

	ULevel* SubLevel = nullptr;

	if (bCreateSublevel)
	{
		ULevelStreaming* LevelStreaming = Cast<ULevelStreamingDynamic>(
			EditorLevelUtils::CreateNewStreamingLevelForWorld(*GetWorld(), ULevelStreamingDynamic::StaticClass(),
			                                                  GetSubLevelPackageName()));
		SubLevel = LevelStreaming->GetLoadedLevel();
		check(SubLevel);
		EditorLevelUtils::MakeLevelCurrent(LevelStreaming); // for spawn actor
	}

	// Step.3: Generate Imposter to scene.
	FActorSpawnParameters Params;
	Params.Name = FName(GetActorAssetName());
	AImposterActor* ImposterActor = GetWorld()->SpawnActor<AImposterActor>(
		CaptureBounds.Origin + Offset, FRotator::ZeroRotator, Params);
	UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(
		ImposterActor->GetComponentByClass(UStaticMeshComponent::StaticClass()));
	check(StaticMeshComponent);
	StaticMeshComponent->SetStaticMesh(Settings.UnitQuad);
	StaticMeshComponent->SetMaterial(0, Mic);
}

void UImposterCore::PreviewImposter()
{
	// MID
	UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Settings.ImposterMaterial, this);
	for (const auto& Elem : CapturedTexMap)
	{
		FString* TexParam = Settings.ImposterMaterialTextureBindingMap.Find(Elem.Key);
		if (!TexParam)
		{
			UE_LOG(LogImposterGenerator, Warning, TEXT("(PreviewImposter) %s not found in texture binding map."),
			       *CaptureTypeEnumToString(Elem.Key));
			continue;
		}
		Mid->SetTextureParameterValue(**TexParam, Elem.Value);
		Mid->SetScalarParameterValue(Settings.ImposterMaterialGridSizeParam, Settings.GridSize);
		Mid->SetScalarParameterValue(Settings.ImposterMaterialSizeParam, CaptureBounds.SphereRadius * 2);
	}

	// Transient Actor
	FActorSpawnParameters Params;
	Params.Name = FName(GetActorAssetName());
	Params.ObjectFlags |= RF_Transient;
	AImposterActor* ImposterActor = GetWorld()->SpawnActor<AImposterActor>(
		CaptureBounds.Origin, FRotator::ZeroRotator, Params);
	UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(
	ImposterActor->GetComponentByClass(UStaticMeshComponent::StaticClass()));
	check(StaticMeshComponent);
	StaticMeshComponent->SetStaticMesh(Settings.UnitQuad);
	StaticMeshComponent->SetMaterial(0, Mid);
	
}

void UImposterCore::DebugCaptureDirections() const
{
	FlushPersistentDebugLines(GetWorld());
	for (const FVector& Dir : CaptureDirections)
	{
		DrawDebugDirectionalArrow(GetWorld(), CaptureBounds.Origin + Dir * CaptureBounds.SphereRadius * 1.5,
		                          CaptureBounds.Origin + Dir * CaptureBounds.SphereRadius, 10.0f, FColor::Red,
		                          false, 5.0f, 0, 1);
	}
}

void UImposterCore::DebugCaptureBounds() const
{
	FlushPersistentDebugLines(GetWorld());
	DrawDebugSphere(GetWorld(), CaptureBounds.Origin, CaptureBounds.SphereRadius, 32, FColor::Red, false, 5.f, 0, 0);
	DrawDebugBox(GetWorld(), CaptureBounds.Origin, CaptureBounds.BoxExtent, FColor::Yellow, false, 6.f, 0, 1);
}

ESceneCaptureSource UImposterCore::GetSceneCaptureSourceFrom(EImposterCaptureType ImposterCaptureType)
{
	switch (ImposterCaptureType)
	{
	case EImposterCaptureType::SceneColor:
		return SCS_SceneColorHDR;
	case EImposterCaptureType::SceneDepth:
		return SCS_SceneDepth;
	case EImposterCaptureType::WorldNormal:
		return SCS_Normal;
	case EImposterCaptureType::BaseColor:
		return SCS_BaseColor;
	default:
		return SCS_FinalColorLDR;
	}
}

FString UImposterCore::CaptureTypeEnumToString(EImposterCaptureType Type)
{
	// todo: better way? e.g.: use reflection

	switch (Type)
	{
	case EImposterCaptureType::SceneColor: return TEXT("SceneColor");
	case EImposterCaptureType::Depth01: return TEXT("SceneDepth");
	case EImposterCaptureType::WorldNormal: return TEXT("WorldNormal");
	case EImposterCaptureType::Metallic: return TEXT("Metallic");
	case EImposterCaptureType::Opacity: return TEXT("Opacity");
	case EImposterCaptureType::Roughness: return TEXT("Roughness");
	case EImposterCaptureType::Specular: return TEXT("Specular");
	case EImposterCaptureType::AO: return TEXT("AO");
	case EImposterCaptureType::BaseColor: return TEXT("BaseColor");
	case EImposterCaptureType::SceneDepth: return TEXT("SceneDepth");;
	case EImposterCaptureType::Emissive: return TEXT("Emissive");;
	case EImposterCaptureType::BaseColorAlpha: return TEXT("BaseColorAlpha");;
	case EImposterCaptureType::NormalDepth: return TEXT("NormalDepth");;
	case EImposterCaptureType::MetallicRoughnessAO: return TEXT("MetallicRoughnessAO");;
	case EImposterCaptureType::Custom1: return TEXT("Custom1");;
	case EImposterCaptureType::Custom2: return TEXT("Custom2");;
	case EImposterCaptureType::Custom3: return TEXT("Custom3");;
	case EImposterCaptureType::Custom4: return TEXT("Custom4");;
	case EImposterCaptureType::Custom5: return TEXT("Custom5");;
	case EImposterCaptureType::Custom6: return TEXT("Custom6");;
	}
	return TEXT("Invalid");
}

void UImposterCore::PrefixReplaceContentToGame(FString& Path)
{
	Path.RemoveFromStart(TEXT("/"));
	Path.RemoveFromStart(TEXT("Content/"));
	Path.StartsWith(TEXT("Game/")) == true ? Path.InsertAt(0, TEXT("/")) : Path.InsertAt(0, TEXT("/Game/"));
}


void UImposterCore::Reset()
{
	CaptureDirections.Reset();
	CapturedTexMap.Reset();
}

// copied from BlueprintMaterialTextureNodesBPLibrary.
UMaterialInstanceConstant* UImposterCore::CreateMIC_EditorOnly(UMaterialInterface* Material, FString InName)
{
	check(Material);

	TArray<UObject*> ObjectsToSync;

	// Create an appropriate and unique name 
	FString Name;
	FString PackageName;
	IAssetTools& AssetTools = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	//Use asset name only if directories are specified, otherwise full path
	if (!InName.Contains(TEXT("/")))
	{
		FString AssetName = Material->GetOutermost()->GetName();
		const FString SanitizedBasePackageName = UPackageTools::SanitizePackageName(AssetName);
		const FString PackagePath = FPackageName::GetLongPackagePath(SanitizedBasePackageName) + TEXT("/");
		AssetTools.CreateUniqueAssetName(PackagePath, InName, PackageName, Name);
	}
	else
	{
		InName.RemoveFromStart(TEXT("/"));
		InName.RemoveFromStart(TEXT("Content/"));
		InName.StartsWith(TEXT("Game/")) == true
			? InName.InsertAt(0, TEXT("/"))
			: InName.InsertAt(0, TEXT("/Game/"));
		AssetTools.CreateUniqueAssetName(InName, TEXT(""), PackageName, Name);
	}

	UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
	Factory->InitialParent = Material;

	UObject* NewAsset = AssetTools.CreateAsset(Name, FPackageName::GetLongPackagePath(PackageName),
	                                           UMaterialInstanceConstant::StaticClass(), Factory);

	ObjectsToSync.Add(NewAsset);
	GEditor->SyncBrowserToObjects(ObjectsToSync);

	UMaterialInstanceConstant* MIC = Cast<UMaterialInstanceConstant>(NewAsset);

	return MIC;
}
