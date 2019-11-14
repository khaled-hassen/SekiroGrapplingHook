// Fill out your copyright notice in the Description page of Project Settings.
#include "GrapplingPoint.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AGrapplingPoint::AGrapplingPoint()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Point = CreateDefaultSubobject<UStaticMeshComponent>("Point");
	Point->SetCollisionProfileName(TEXT("NoCollision"));
}

// Called when the game starts or when spawned
void AGrapplingPoint::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGrapplingPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AGrapplingPoint::ChangeToBaseMat()
{
	if (BaseMaterial)
	{
		Point->SetMaterial(0, BaseMaterial);
	}
}

void AGrapplingPoint::ChangeToGrapplingMat()
{
	if (GrapplingMaterial)
	{
		Point->SetMaterial(0, GrapplingMaterial);
	}
}