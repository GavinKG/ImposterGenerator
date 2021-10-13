// Fill out your copyright notice in the Description page of Project Settings.

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

protected:
};
