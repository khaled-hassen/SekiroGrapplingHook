// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrapplingPoint.generated.h"

UCLASS()
class SEKIROGRAPPLINGHOOK_API AGrapplingPoint : public AActor
{
	GENERATED_BODY()
	
private:	
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Point = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "GamePlay")
	UMaterialInstance* BaseMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "GamePlay")
	UMaterialInstance* GrapplingMaterial = nullptr;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Sets default values for this actor's properties
	AGrapplingPoint();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void ChangeToBaseMat();
	void ChangeToGrapplingMat();
};
