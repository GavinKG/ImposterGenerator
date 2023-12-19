// malosgao presents.

#pragma once

#include "CoreMinimal.h"

#include "ImposterCore.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogImposterGenerator, Log, All);


UENUM()
enum class EImposterSphereType : uint8
{
	Full,
	UpperHemi
	// Billboards, we do not support billboard right now...
};

UENUM()
enum class EImposterCaptureType : uint8
{
	// These types can be captured by setting capture component's capture source, and it might be faster since GBuffer will not be composed.
	// post-processing material will not be used (ignored if assigned) here.
	SceneColor,
	// SCS_SceneColorHDR
	BaseColor,
	// SCS_BaseColor
	WorldNormal,
	// SCS_WorldNormal
	SceneDepth,
	// SCS_SceneDepth

	// These types should be used with a post-processing material to extract a particular RT in GBuffer. SCS_FinalColorLDR will be used to enable custom post-processing material.
	// There is no way to reuse GBuffer without tinkering engine code, so for each type, whole scene will be rendered entirely. Use aggregate capture type if you can.
	Depth01,
	Specular,
	Metallic,
	Opacity,
	Roughness,
	AO,
	Emissive,

	// Useful aggregate types below. Use these to avoid custom channel packing and speed will be faster. These type will always use a custom post-processing material.
	BaseColorAlpha,
	// RGB: Base Color, A: Opacity
	NormalDepth,
	// RGB: Normal, A: Linear Depth
	MetallicRoughnessSpecularAO,
	// R: Metallic, G: Roughness, B: Specular, A: AO

	// Custom type.
	// Actually, all types above can be considered as a named custom type since logics are the same.
	Custom1,
	Custom2,
	Custom3,
	Custom4,
	Custom5,
	Custom6,
};

USTRUCT(BlueprintType)
struct FImposterBlitSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Imposter)
	EImposterCaptureType CaptureType;

	UPROPERTY(EditAnywhere, Category = Imposter)
	class UMaterialInterface* BlitMaterial;
};

// Imposter Generate overall settings, or profiles.
USTRUCT(BlueprintType)
struct FImposterGeneratorSettings
{
	GENERATED_BODY()

	FImposterGeneratorSettings();

	UPROPERTY(EditAnywhere, Category = Imposter)
	EImposterSphereType ImposterType;

	UPROPERTY(EditAnywhere, Category = Imposter)
	TArray<EImposterCaptureType> CaptureTextureTypes;

	UPROPERTY(EditAnywhere, Category = Imposter)
	uint32 GridSize = 8;

	UPROPERTY(EditAnywhere, Category = Imposter)
	uint32 ImposterTextureRes = 4096;

	UPROPERTY(EditAnywhere, Category = Imposter)
	TMap<EImposterCaptureType, class UMaterialInterface*> CaptureMaterialMap;

	// Used for custom multi-pass post-processing (aka. blit) on captured material map, like UV dilation and distance field generation.
	// Blit will be executed in order.
	// Same capture type can be used to achieve multi-pass blit.
	// Source texture parameter should be named "Texture"
	UPROPERTY(EditAnywhere, Category = Imposter)
	TArray<FImposterBlitSettings> AdditionalCaptureBlit;

	UPROPERTY(EditAnywhere, Category = Imposter)
	class UMaterialInterface* ImposterMaterial;

	UPROPERTY(EditAnywhere, Category = Imposter)
	FName ImposterMaterialGridSizeParam = TEXT("FramesXY");

	UPROPERTY(EditAnywhere, Category = Imposter)
	FName ImposterMaterialSizeParam = TEXT("Default Mesh Size");

	UPROPERTY(EditAnywhere, Category = Imposter)
	TMap<EImposterCaptureType, FString> ImposterMaterialTextureBindingMap;

	UPROPERTY(EditAnywhere, Category = Imposter)
	UStaticMesh* UnitQuad;

	// This MPC is used by Imposter and SceneDepth remapping material.
	UPROPERTY(EditAnywhere, Category = Imposter)
	UMaterialParameterCollection* CaptureMpc;

	UPROPERTY(EditAnywhere, Category=Export)
	FString WorkingPath = TEXT("/Game");
};

/**
 * 
 */
UCLASS(BlueprintType)
class IMPOSTERGENERATOR_API UImposterCore final : public UObject
{
	GENERATED_BODY()

	friend class AImposterManagerActor;

	UImposterCore();

	// Operation guard.
	enum class EImposterCoreState : uint8
	{
		NotInitialized,
		Initialized,
		Captured,
		Exported,
	};

public:
	bool TryAddCapturedActor(AActor* Actor);
	void RemoveAllCapturedActors();

	// 1 - Init
	bool Init(const FImposterGeneratorSettings& ImposterGeneratorSettings,
	          const TArray<AActor*>& CapturedActorArray,
	          const FString& ImposterNameString);

	// 2 - Capture
	void Capture();

	// 3 - Generate Imposter Object in place
	void GenerateImposter(bool bCreateSublevel, const FVector& Offset = FVector::ZeroVector);

	void PreviewImposter();
	
	// Debugging functions
	void DebugCaptureDirections() const;
	void DebugCaptureBounds() const;

public: // members

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Imposter)
	FImposterGeneratorSettings Settings;

private:

	// Set by Init()
	UPROPERTY()
	FString ImposterName = TEXT("Imposter");

	UPROPERTY()
	TArray<AActor*> CapturedActors;

	UPROPERTY()
	TArray<FVector> CaptureDirections;

	UPROPERTY()
	FBoxSphereBounds CaptureBounds;

	UPROPERTY()
	TMap<EImposterCaptureType, class UTextureRenderTarget2D*> CapturedTexMap;

	UPROPERTY()
	class AImposterCaptureActor* ImposterCaptureActor;

	UPROPERTY()
	class UTextureRenderTarget2D* MiddleRT;

	UPROPERTY()
	UMaterialParameterCollectionInstance* CaptureMpcInst;


public: // static

	static bool CanAddActorToCapture(const AActor* Actor);

	// Just like normal compression & decompression using octahedron
	// https://jcgt.org/published/0003/02/01/
	// Engine\Shaders\Private\DeferredShadingCommon.ush
	static FVector OctahedronToDirVector(const FVector2D& Grid);
	static FVector HemiOctahedronToDirVector(const FVector2D& Grid);

	// See EImposterCaptureType for details.
	static ESceneCaptureSource GetSceneCaptureSourceFrom(EImposterCaptureType ImposterCaptureType);

	static FString CaptureTypeEnumToString(EImposterCaptureType Type);

	static void PrefixReplaceContentToGame(FString& Path);

	static class UMaterialInstanceConstant* CreateMIC_EditorOnly(UMaterialInterface* Material, FString InName);


private:
	// Can be called repeatedly.
	void InitTex();
	void InitCaptureBounds();
	void InitCaptureDirections();
	void InitMpc();


	// Imposter Root Dir:  WorkingPath/ImposterName
	// Imposter Actor:     BP_ImposterName
	// Imposter Package:   WorkingPath/ImposterName/BP_ImposterName
	// Textures:           T_ImposterName_CaptureType
	// Texture Package:    WorkingPath/ImposterName/T_ImposterName_CaptureType
	// Material Instance:  MI_ImposterName
	// Material Package:   WorkingPath/ImposterName/MI_ImposterName
	// SubLevel:           ImposterName_SubLevel
	// SubLevel Package:   WorkingPath/ImposterName/ImposterName_SubLevel

	FORCEINLINE FString GetImposterRootDir() const
	{
		return FPaths::Combine(Settings.WorkingPath, ImposterName);
	}

	FORCEINLINE FString GetActorAssetName() const
	{
		return FString::Printf(TEXT("BP_%s"), *ImposterName);
	}

	FORCEINLINE FString GetActorPackageName() const
	{
		return FPaths::Combine(GetImposterRootDir(), GetActorAssetName());
	}

	FORCEINLINE FString GetTextureAssetName(EImposterCaptureType CaptureType) const
	{
		return FString::Printf(TEXT("T_%s_%s"), *ImposterName, *CaptureTypeEnumToString(CaptureType));
	}

	FORCEINLINE FString GetTexturePackageName(EImposterCaptureType CaptureType) const
	{
		return FPaths::Combine(GetImposterRootDir(), GetTextureAssetName(CaptureType));
	}

	FORCEINLINE FString GetMaterialAssetName() const
	{
		return FString::Printf(TEXT("MI_%s"), *ImposterName);
	}

	FORCEINLINE FString GetMaterialPackageName() const
	{
		return FPaths::Combine(GetImposterRootDir(), GetMaterialAssetName());
	}

	FORCEINLINE FString GetSubLevelAssetName() const
	{
		return FString::Printf(TEXT("%s_SubLevel"), *ImposterName);
	}

	FORCEINLINE FString GetSubLevelPackageName() const
	{
		return FPaths::Combine(GetImposterRootDir(), GetSubLevelAssetName());
	}

	// Should
	bool ShouldInit(bool bShouldLog = true) const;
	bool ShouldCapture(bool bShouldLog = true) const;
	bool ShouldGenerate(bool bShouldLog = true) const;

	FVector ImposterGridToDirVector(FVector2D Grid) const;

	void Reset();


private: // members
	EImposterCoreState ImposterCoreState = EImposterCoreState::NotInitialized;
};
