// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "PPCharacter.h"
#include "Animation/ThirdPersonAnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/PPCharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/KismetMathLibrary.h"
//////////////////////////////////////////////////////////////////////////
// APetProjectCharacter

APPCharacter::APPCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UPPCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	AddTickPrerequisiteComponent(InputComponent);

	// Set size for collision capsule
	UCapsuleComponent* capsuleComponent = GetCapsuleComponent();
	capsuleComponent->InitCapsuleSize(42.f, 96.0f);
	// capsuleComponent->bAbsoluteRotation = true;
	
	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;
	// TurnToAimingLocationRate = 0.5f;

	IsRunning = false;
	IsAiming = false;
	TargetInteractableActor = nullptr;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	// the rotation will be animation driven, so don't update rotation programmically
	GetCharacterMovement()->bOrientRotationToMovement = false; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 250.f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;
	// GetCharacterMovement()->AddTickPrerequisiteActor(this);
	
	FNavAgentProperties& navProps = GetCharacterMovement()->GetNavAgentPropertiesRef();
	navProps.bCanWalk = true;
	navProps.bCanJump = true;
	navProps.bCanCrouch = true;
	navProps.bCanFly = false;
	navProps.bCanSwim = false;

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
	//this->GetMesh()->PrimaryComponentTick.TickGroup = ETickingGroup::TG_PrePhysics;
	//this->GetMesh()->AddTickPrerequisiteComponent(GetCharacterMovement());

#if WITH_EDITOR
	VelocityDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("VelocityDirection"));
	VelocityDirection->bAbsoluteRotation = true;
	VelocityDirection->SetupAttachment(RootComponent);
	VelocityDirection->bVisible = false;
	VelocityDirection->bHiddenInGame = false;
	VelocityDirection->ArrowColor = FColor::Red;

	AccelerationDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("AccelerationDirection"));
	AccelerationDirection->bAbsoluteRotation = true;
	AccelerationDirection->SetupAttachment(RootComponent);
	AccelerationDirection->bVisible = false;
	AccelerationDirection->bHiddenInGame = false;
	AccelerationDirection->ArrowColor = FColor::Green;

	ControllerDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("ControllerDirection"));
	ControllerDirection->bAbsoluteRotation = true;
	ControllerDirection->SetupAttachment(RootComponent);
	ControllerDirection->bVisible = false;
	ControllerDirection->bHiddenInGame = false;
	ControllerDirection->ArrowColor = FColor::Black;
#endif
}

void APPCharacter::Tick(float deltaSeconds)
{
	// auto ddd = GFrameCounter;
	Super::Tick(deltaSeconds);

	FHitResult hitresult(ForceInit);

	FVector cameraLocation = FollowCamera->GetComponentLocation();
	AimingLocation = cameraLocation + FollowCamera->GetForwardVector() * 10000.f;
	if (GetWorld()->LineTraceSingleByChannel(
		hitresult,
		cameraLocation,
		AimingLocation,
		ECollisionChannel::ECC_Visibility,
		FCollisionQueryParams(FName(TEXT("Character Camera Trace")), true, this)))
	{
		AimingLocation = hitresult.ImpactPoint;

		AActor* hitActor = hitresult.Actor.Get();
		if (hitActor && hitActor->ActorHasTag(FName("Climbable")))
		{
			TargetInteractableActor = hitActor;
		}
	}

	// DrawDebugSphere(this->GetWorld(), AimingLocation, 15.f, 20, FColor::Red);
	
	UThirdPersonAnimInstance* animInstance = Cast<UThirdPersonAnimInstance>(GetMesh()->GetAnimInstance());
	GetCharacterMovement()->MaxWalkSpeed = animInstance->GetClampedJogSpeed(IsRunning && !bIsCrouched);
	// GetCharacterMovement()->MaxWalkSpeed = 174.f;
	// UE_LOG(LogTemp, Log, TEXT("max speed %f"), GetCharacterMovement()->MaxWalkSpeed);

#if WITH_EDITOR
	UCharacterMovementComponent* movement = GetCharacterMovement();
	VelocityDirection->SetWorldRotation(UKismetMathLibrary::MakeRotFromX(this->GetVelocity()));
	AccelerationDirection->SetWorldRotation(UKismetMathLibrary::MakeRotFromX(movement->GetCurrentAcceleration()));
	ControllerDirection->SetWorldRotation(this->GetControlRotation());


	// AimingLocation
	/*UE_LOG(LogTemp, Log, TEXT("b: %s"), *(this->GetMesh()->Get).GetLocation().ToString());
*/
#endif
}

void APPCharacter::ProcessJump()
{
	if (this->CanJump())
	{
		Jump();
	}
	else 
	{
		AddMovementInput(FVector::UpVector);
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void APPCharacter::SetupPlayerInputComponent(class UInputComponent* playerInputComponent)
{
	// Set up gameplay key bindings
	check(playerInputComponent);
	playerInputComponent->BindAction("Jump", IE_Pressed, this, &APPCharacter::ProcessJump);
	playerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// playerInputComponent->BindAction("Interact", IE_Pressed, InteractionComponent, &UInteractionComponent::TryInteraction);

	DECLARE_DELEGATE_OneParam(FBoolDelegate, bool);

	playerInputComponent->BindAction<FBoolDelegate>("Aim", IE_Pressed, this, &APPCharacter::SetAiming, true);
	playerInputComponent->BindAction<FBoolDelegate>("Aim", IE_Released, this, &APPCharacter::SetAiming, false);

	playerInputComponent->BindAction<FBoolDelegate>("Crouch", IE_Pressed, this, &ACharacter::Crouch, false);
	playerInputComponent->BindAction<FBoolDelegate>("Crouch", IE_Released, this, &ACharacter::UnCrouch, false);

	playerInputComponent->BindAction<FBoolDelegate>("Run", IE_Pressed, this, &APPCharacter::SetRunning, true);
	playerInputComponent->BindAction<FBoolDelegate>("Run", IE_Released, this, &APPCharacter::SetRunning, false);

	playerInputComponent->BindAxis("MoveForward", this, &APPCharacter::MoveForward);
	playerInputComponent->BindAxis("MoveRight", this, &APPCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	playerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	playerInputComponent->BindAxis("TurnRate", this, &APPCharacter::TurnAtRate);
	playerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	playerInputComponent->BindAxis("LookUpRate", this, &APPCharacter::LookUpAtRate);

	
	//// handle touch devices
	//playerInputComponent->BindTouch(IE_Pressed, this, &APPCharacter::TouchStarted);
	//playerInputComponent->BindTouch(IE_Released, this, &APPCharacter::TouchStopped);

	//// VR headset functionality
	//playerInputComponent->BindAction("ResetVR", IE_Pressed, this, &APPCharacter::OnResetVR);

// 	UPPCharacterMovementComponent* characterMovement = Cast<UPPCharacterMovementComponent>(this->GetCharacterMovement());

}


void APPCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void APPCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void APPCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void APPCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void APPCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void APPCharacter::SetRunning(bool isRunning)
{
	IsRunning = isRunning;
}

void APPCharacter::SetAiming(bool isAiming)
{
	IsAiming = isAiming;
}

void APPCharacter::MoveForward(float Value)
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

void APPCharacter::MoveRight(float Value)
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
