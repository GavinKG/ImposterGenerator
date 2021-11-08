// malosgao presents.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ImposterActor.generated.h"

UCLASS()
class IMPOSTERGENERATOR_API AImposterActor : public AActor
{
	GENERATED_BODY()

public:
	AImposterActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* StaticMeshComponent;

	// Imposter should not cast shadows since it uses "Camera Position" instead of light position to do geometry calculation, and we cannot get light direction easily.
	// May hardcoded a light direction in the future so I leave it with a parameter.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCastShadows = false;

protected:
};
