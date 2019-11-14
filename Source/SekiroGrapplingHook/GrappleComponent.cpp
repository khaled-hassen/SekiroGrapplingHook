// Fill out your copyright notice in the Description page of Project Settings.


#include "GrappleComponent.h"
#include "Components/TimelineComponent.h"
#include "CableComponent.h"
#include "GrapplingPoint.h"
#include "SekiroGrapplingHookCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values for this component's properties
UGrappleComponent::UGrappleComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// GrapplingHook setup
	GrapplingHook = CreateDefaultSubobject<UCableComponent>(TEXT("GrapplingHook"));
	GrapplingHook->bVisible = false;
	GrapplingHook->EndLocation = FVector::ZeroVector;
	GrapplingHook->CableLength = 0;

	ThrowGrapplingHookTimeline = CreateDefaultSubobject<UTimelineComponent>("ThrowGrapplingHookTimeline");
}


// Called when the game starts
void UGrappleComponent::BeginPlay()
{
	Super::BeginPlay();

	ASekiroGrapplingHookCharacter* Player = Cast<ASekiroGrapplingHookCharacter>(GetOwner());
	if (Player)
	{
		GrapplingHook->AttachToComponent(Player->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, "GrapplingHook");
	} 
}


// Called every frame
void UGrappleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FindGrapplingPoints();
}

void UGrappleComponent::FindGrapplingPoints()
{
	if (!bIsGrappling)
	{
		if (ensure(GrapplingPointBlueprint))
		{
			TArray<AActor*>SpawnedGrapplingPoints;
			UGameplayStatics::GetAllActorsOfClass(this, GrapplingPointBlueprint, SpawnedGrapplingPoints);

			if (SpawnedGrapplingPoints.Num() != 0)
			{
				for (AActor* GrapplingPointActor : SpawnedGrapplingPoints)
				{
					AGrapplingPoint* GrapplingPoint = Cast<AGrapplingPoint>(GrapplingPointActor);

					if (GrapplingPoint)
					{
						if (GrapplingPoint->WasRecentlyRendered())
						{
							AddToGrapplingPoints(GrapplingPoint);
						}
						else
						{
							RemoveFromGrapplingPoints(GrapplingPoint);
						}
						ChangeColor(GrapplingPoint);
					}
				}
			}
		}
	}
}

void UGrappleComponent::ChangeColor(AGrapplingPoint* GrapplingPoint)
{
	GrapplingPoint->ChangeToBaseMat();
	if (GetClosestGrapplingPoint())
	{
		GetClosestGrapplingPoint()->ChangeToGrapplingMat();
	}
}

void UGrappleComponent::AddToGrapplingPoints(class AGrapplingPoint* GrapplingPoint)
{
	ASekiroGrapplingHookCharacter* Player = Cast<ASekiroGrapplingHookCharacter>(GetOwner());
	if (!Player) { return; }

	float Distance = (Player->GetActorLocation() - GrapplingPoint->GetActorLocation()).Size();

	if (Distance <= MaxGrapplingDistance)
	{
		FVector ControllerForwardVector = FVector(Player->GetControlRotation().Vector().X, Player->GetControlRotation().Vector().Y, 0.f).GetSafeNormal();
		FVector GrapplingPointDirection = FVector((Player->GetActorLocation() - GrapplingPoint->GetActorLocation()).X,
			(Player->GetActorLocation() - GrapplingPoint->GetActorLocation()).Y, 0.f).GetSafeNormal();

		float Angle = FMath::Acos(FMath::Abs(FVector::DotProduct(ControllerForwardVector, GrapplingPointDirection)));
		if (FVector::DotProduct(-ControllerForwardVector, GrapplingPointDirection) >= 0)
		{
			if (GrapplingPoints.Contains(GrapplingPoint))
			{
				int32 Index = GrapplingPoints.Find(GrapplingPoint);
				Angles[Index] = Angle;
			}
			else
			{
				GrapplingPoints.Add(GrapplingPoint);
				Angles.Add(Angle);
			}
		}
	}
}

void UGrappleComponent::RemoveFromGrapplingPoints(class AGrapplingPoint* GrapplingPoint)
{
	if (GrapplingPoints.Contains(GrapplingPoint))
	{
		int32 Index = GrapplingPoints.Find(GrapplingPoint);
		GrapplingPoints.RemoveAt(Index);
		Angles.RemoveAt(Index);
	}
}

AGrapplingPoint* UGrappleComponent::GetClosestGrapplingPoint() const
{
	if ((Angles.Num() != 0) && (GrapplingPoints.Num() != 0))
	{
		float MinAngle = Angles[0];
		for (const float Angle : Angles)
		{
			MinAngle = (Angle < MinAngle) ? Angle : MinAngle;
		}

		return GrapplingPoints[Angles.Find(MinAngle)];
	}

	return nullptr;
}

void UGrappleComponent::Grapple()
{
	AGrapplingPoint* GrapplingPoint = GetClosestGrapplingPoint();

	if (GrapplingPoint)
	{
		GrapplingHook->SetVisibility(true);

		if (ensure(ThrowTimeCurve))
		{
			// get the ThrowHook time
			float MinTime = 0.f, CurveLength = 0.f;
			ThrowTimeCurve->GetTimeRange(MinTime, CurveLength);

			// setup the Timeline
			ThrowGrapplingHookTimeline->SetLooping(false);
			ThrowGrapplingHookTimeline->SetIgnoreTimeDilation(true);
			ThrowGrapplingHookTimeline->SetTimelineLength(CurveLength);

			// set the ThrowHookDelegate to be bound to ThrowGrapplingHook
			FOnTimelineFloat ThrowHookDelegate;
			ThrowHookDelegate.BindUFunction(this, "ThrowGrapplingHook");
			ThrowGrapplingHookTimeline->AddInterpFloat(ThrowTimeCurve, ThrowHookDelegate, "Value");

			// launch the player when the grappling hook is at target location
			FOnTimelineEvent TimelineFinished;
			TimelineFinished.BindUFunction(this, FName("LaunchCharacterTowardsTarget"));
			ThrowGrapplingHookTimeline->SetTimelineFinishedFunc(TimelineFinished);

			bIsGrappling = true;
			ThrowGrapplingHookTimeline->PlayFromStart();
		}
	}
}

void UGrappleComponent::LaunchCharacterTowardsTarget()
{
	ASekiroGrapplingHookCharacter* Player = Cast<ASekiroGrapplingHookCharacter>(GetOwner());
	if (!Player) { return; }

	AGrapplingPoint* Target = GetClosestGrapplingPoint();
	if (Target && GrapplingHook)
	{
		FVector LaunchVelocity = FVector::ZeroVector;
		bool bHasSolution = UGameplayStatics::SuggestProjectileVelocity_CustomArc(this,
			LaunchVelocity,
			Player->GetActorLocation(),
			Target->GetActorLocation(),
			0.0f, 0.5f);

		if (bHasSolution)
		{
			Player->LaunchCharacter(LaunchVelocity * GetWorld()->GetDeltaSeconds() * 65, true, true);
			GrapplingHook->SetVisibility(false);
			bIsGrappling = false;
			GrapplingHook->SetWorldLocation(Player->GetMesh()->GetSocketLocation("GrapplingHook"));
		}
	}
}

void UGrappleComponent::ThrowGrapplingHook(float Value)
{
	if (GrapplingHook && GetClosestGrapplingPoint())
	{
		FVector NewLocation = FMath::Lerp<FVector, float>(GrapplingHook->GetComponentLocation(), GetClosestGrapplingPoint()->GetActorLocation(), Value);
		GrapplingHook->SetWorldLocation(NewLocation);
	}
}