// malosgao presents.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ImposterCaptureActor.generated.h"


// should be stateless to ImposterCore. Act as a bridge between core and level.
UCLASS()
class IMPOSTERGENERATOR_API AImposterCaptureActor : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	class USceneCaptureComponent2D* SceneCaptureComponent2D;

public:
	AImposterCaptureActor();

	void SetCaptureTransform(const FVector& Location, const FRotator& Rotation);

	void RegisterActorInfo(const TArray<AActor*>& Actors, const FBoxSphereBounds& Bounds);

	void SetPostProcessingMaterial(UMaterialInterface* Material);
	
	void Capture(ESceneCaptureSource CaptureSource, UTextureRenderTarget2D* RT);
	
protected:
	virtual void BeginPlay() override;
};
