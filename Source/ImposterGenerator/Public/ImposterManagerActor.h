// malosgao presents.

#pragma once

#include "CoreMinimal.h"
#include "ImposterCore.h"
#include "GameFramework/Actor.h"
#include "ImposterManagerActor.generated.h"

USTRUCT()
struct FMyStruct
{
	GENERATED_BODY()

	UPROPERTY()
	float MyFloat;
};


UCLASS(BlueprintType, Blueprintable, AutoCollapseCategories=(Debug))
class IMPOSTERGENERATOR_API AImposterManagerActor final : public AActor
{
	GENERATED_BODY()

public:
	AImposterManagerActor();

	// virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Imposter)
	FImposterGeneratorSettings Settings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Imposter)
	TArray<AActor*> CapturedActors;

	UFUNCTION(BlueprintCallable, CallInEditor, Category=Actions)
	void A_AddSceneActors();
	
	UFUNCTION(BlueprintCallable, CallInEditor, Category=Actions)
	void B_InitCore();

	UFUNCTION(BlueprintCallable, CallInEditor, Category=Actions)
	void C_Capture();

	UFUNCTION(BlueprintCallable, CallInEditor, Category=Actions)
	void D_Generate();
	
	UFUNCTION(BlueprintCallable, CallInEditor, Category=Actions)
	void E_Preview();

	UFUNCTION(BlueprintCallable, CallInEditor, Category=Debug)
	void DebugCaptureDirections();

	UFUNCTION(BlueprintCallable, CallInEditor, Category=Debug)
	void DebugCaptureBounds();



	

	UPROPERTY(VisibleAnywhere, Transient, Category="Debug|Capture")
	TMap<EImposterCaptureType, class UTextureRenderTarget2D*> CapturedTexMap;

	UPROPERTY(VisibleAnywhere, Transient, Category="Debug|Capture")
	UTextureRenderTarget2D* MiddleRT;

	UPROPERTY(VisibleAnywhere, Transient, Category="Debug|Capture")
	TArray<UTextureRenderTarget2D*> ComposedRenderTargets;

	

private:
	UPROPERTY(Transient)
	UImposterCore* Core;

	void CopyDebugFromCore();
};
