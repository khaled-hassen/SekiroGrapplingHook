// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SekiroGrapplingHookCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GrapplingPoint.h"
#include "Kismet/GameplayStatics.h"
#include "CableComponent.h"
#include "Components/TimelineComponent.h"

//////////////////////////////////////////////////////////////////////////
// ASekiroGrapplingHookCharacter

ASekiroGrapplingHookCharacter::ASekiroGrapplingHookCharacter()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)

	// GrapplingHook setup
	GrapplingHook = CreateDefaultSubobject<UCableComponent>(TEXT("GrapplingHook"));
	GrapplingHook->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, "GrapplingHook");
	GrapplingHook->bVisible = false;
	GrapplingHook->EndLocation = FVector::ZeroVector;
	GrapplingHook->CableLength = 0;

	ThrowGrapplingHookTimeline = CreateDefaultSubobject<UTimelineComponent>("ThrowGrapplingHookTimeline");
}

//////////////////////////////////////////////////////////////////////////
// Input

void ASekiroGrapplingHookCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &ASekiroGrapplingHookCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASekiroGrapplingHookCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ASekiroGrapplingHookCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ASekiroGrapplingHookCharacter::LookUpAtRate);

	// Grappling input
	PlayerInputComponent->BindAction("Grapple", IE_Pressed, this, &ASekiroGrapplingHookCharacter::Grapple);
}


void ASekiroGrapplingHookCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ASekiroGrapplingHookCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ASekiroGrapplingHookCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ASekiroGrapplingHookCharacter::MoveRight(float Value)
{
	if ( (Controller != NULL) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

// Called every frame
void ASekiroGrapplingHookCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ensure(GrapplingPointBlueprint))
	{
		TArray<AActor*>SpawnedGrapplingPoints;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), GrapplingPointBlueprint, SpawnedGrapplingPoints);

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
				}
			}
		}
	}
}

void ASekiroGrapplingHookCharacter::AddToGrapplingPoints(class AGrapplingPoint* GrapplingPoint)
{
	float Distance = (GetActorLocation() - GrapplingPoint->GetActorLocation()).Size();

	if (Distance <= MaxGrapplingDistance)
	{
		FVector ControllerForwardVector = FVector(GetControlRotation().Vector().X, GetControlRotation().Vector().Y, 0.f).GetSafeNormal();
		FVector GrapplingPointDirection = FVector((GetActorLocation() - GrapplingPoint->GetActorLocation()).X,
			(GetActorLocation() - GrapplingPoint->GetActorLocation()).Y, 0.f).GetSafeNormal();

		float Angle = FMath::Acos(FMath::Abs(FVector::DotProduct(ControllerForwardVector, GrapplingPointDirection)));

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

void ASekiroGrapplingHookCharacter::RemoveFromGrapplingPoints(class AGrapplingPoint* GrapplingPoint)
{
	if (GrapplingPoints.Contains(GrapplingPoint))
	{
		int32 Index = GrapplingPoints.Find(GrapplingPoint);
		GrapplingPoints.RemoveAt(Index);
		Angles.RemoveAt(Index);
	}
}

AGrapplingPoint* ASekiroGrapplingHookCharacter::GetClosestGrapplingPoint() const
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

void ASekiroGrapplingHookCharacter::Grapple()
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

			ThrowGrapplingHookTimeline->PlayFromStart();
		}
	}
}

void ASekiroGrapplingHookCharacter::LaunchCharacterTowardsTarget()
{
	AGrapplingPoint* Target = GetClosestGrapplingPoint();
	if (Target && GrapplingHook)
	{
		FVector LaunchVelocity = FVector::ZeroVector;
		bool bHasSolution = UGameplayStatics::SuggestProjectileVelocity_CustomArc(this,
			LaunchVelocity,
			GetActorLocation(), 
			Target->GetActorLocation(),
			0.0f, 0.4f);

		if (bHasSolution)
		{
			LaunchCharacter(LaunchVelocity, true, true);
			GrapplingHook->SetVisibility(false);
			GrapplingHook->SetWorldLocation(GetMesh()->GetSocketLocation("GrapplingHook"));
		}
	}
}

void ASekiroGrapplingHookCharacter::ThrowGrapplingHook(float Value)
{
	if (GrapplingHook && GetClosestGrapplingPoint())
	{
		FVector NewLocation = FMath::Lerp<FVector, float>(GrapplingHook->GetComponentLocation(), GetClosestGrapplingPoint()->GetActorLocation(), Value);
		GrapplingHook->SetWorldLocation(NewLocation);
	}
}