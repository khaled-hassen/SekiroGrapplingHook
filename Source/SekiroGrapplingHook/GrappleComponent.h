// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrappleComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SEKIROGRAPPLINGHOOK_API UGrappleComponent : public UActorComponent
{
	GENERATED_BODY()

private:

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	float MaxGrapplingDistance = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TSubclassOf<class AGrapplingPoint> GrapplingPointBlueprint;

	UPROPERTY()
	class UCableComponent* GrapplingHook;

	UPROPERTY()
	class UTimelineComponent* ThrowGrapplingHookTimeline = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay")
	UCurveFloat* ThrowTimeCurve = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TArray<float> Angles;
	UPROPERTY(BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TArray<class AGrapplingPoint*> GrapplingPoints;

	bool bIsGrappling = false;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Sets default values for this component's properties
	UGrappleComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable)
	class AGrapplingPoint* GetClosestGrapplingPoint() const;

	void AddToGrapplingPoints(class AGrapplingPoint* GrapplingPoint);
	void RemoveFromGrapplingPoints(class AGrapplingPoint* GrapplingPoint);
	void Grapple();

	UFUNCTION(BlueprintCallable, Category = "GamePlay")
	void LaunchCharacterTowardsTarget();

	UFUNCTION()
	void ThrowGrapplingHook(float Value);
};
