// malosgao presents.

#pragma once

#include "CoreMinimal.h"
#include "ImposterCore.h"
#include "GameFramework/Actor.h"
#include "ImposterManagerActor.generated.h"

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
	FString ImposterName = TEXT("Imposter");
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Imposter)
	TArray<TSoftObjectPtr<AActor>> CapturedActors;

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

private:
	UPROPERTY(Transient)
	UImposterCore* Core;

	void CopyDebugFromCore();
};
