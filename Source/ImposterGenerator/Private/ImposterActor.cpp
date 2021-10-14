// Fill out your copyright notice in the Description page of Project Settings.


#include "ImposterActor.h"


// Sets default values
AImposterActor::AImposterActor()
{
	PrimaryActorTick.bCanEverTick = false;
	
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	
	StaticMeshComponent->SetCastShadow(bCastShadows);
	
}
