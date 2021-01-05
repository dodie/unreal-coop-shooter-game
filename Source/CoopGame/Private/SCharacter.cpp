// Fill out your copyright notice in the Description page of Project Settings.

#include "SCharacter.h"
#include "SWeapon.h"
#include "Components/CapsuleComponent.h"
#include "Components/SHealthComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CoopGame.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"

ASCharacter::ASCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->bUsePawnControlRotation = true;

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComponent);

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));
	HealthComp->OnHealthChanged.AddDynamic(this, &ASCharacter::OnHealthChanged);

	GetMovementComponent()->GetNavAgentPropertiesRef().bCanCrouch = true;
	GetCharacterMovement()->GravityScale = 1.5f;
	GetCharacterMovement()->JumpZVelocity = 420.0f;

	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

	ZoomedFov = 65.0f;
	ZoomSpeed = 20.0f;

	CurrentWeaponIndex = 0;

	WeaponAttachSocketName = "WeaponSocket";
}

void ASCharacter::BeginPlay()
{
	Super::BeginPlay();

	DefaultFov = CameraComp->FieldOfView;
	
	if (Role == ROLE_Authority)
	{
		SwitchWeapon(CurrentWeaponIndex);
	}
}

void ASCharacter::MoveForward(float Value)
{
	AddMovementInput(GetActorForwardVector(), Value);
}

void ASCharacter::MoveRight(float Value)
{
	AddMovementInput(GetActorRightVector(), Value);
}

void ASCharacter::BeginCrouch()
{
	Crouch();
}

void ASCharacter::EndCrouch()
{
	UnCrouch();
}

void ASCharacter::BeginZoom()
{
	bWantsToZoom = true;
}

void ASCharacter::EndZoom()
{
	bWantsToZoom = false;
}

void ASCharacter::StartFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StartFire();
	}
}

void ASCharacter::StopFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFire();
	}
}

void ASCharacter::OnHealthChanged(USHealthComponent* HealthComp, float Health)
{
	if (Health <= 0.0f && !bDied)
	{
		StopFire();

		bDied = true;

		GetMovementComponent()->StopMovementImmediately();
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		DetachFromControllerPendingDestroy();
		SetLifeSpan(10.0f);
		CurrentWeapon->SetLifeSpan(10.0f);

		if (DeathSound)
		{
			UGameplayStatics::SpawnSoundAtLocation(this, DeathSound, GetActorLocation());
		}
	}
}

void ASCharacter::SwitchWeapon(int32 WeaponIndex)
{
	if (Role < ROLE_Authority)
	{
		ServerSwitchWeapon(WeaponIndex);
		return;
	}

	if (CurrentWeapon)
	{
		CurrentWeapon->Destroy();
	}

	UE_LOG(LogTemp, Warning, TEXT("SwitchWeapon, %d"), WeaponIndex);
	if (WeaponClasses[WeaponIndex]) {
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		CurrentWeapon = GetWorld()->SpawnActor<ASWeapon>(WeaponClasses[WeaponIndex], FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if (CurrentWeapon)
		{
			CurrentWeapon->SetOwner(this);
			CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, WeaponAttachSocketName);
		}
	}
}

void ASCharacter::ServerSwitchWeapon_Implementation(int32 WeaponIndex)
{
	UE_LOG(LogTemp, Warning, TEXT("ServerSwitchWeapon_Implementation, %d"), WeaponIndex);

	SwitchWeapon(WeaponIndex);
}

bool ASCharacter::ServerSwitchWeapon_Validate(int32 WeaponIndex)
{
	return true;
}

void ASCharacter::SwitchWeaponUp()
{
	CurrentWeaponIndex += 1;
	if (CurrentWeaponIndex > 2)
	{
		CurrentWeaponIndex = 2;
	}

	UE_LOG(LogTemp, Warning, TEXT("SwitchWeaponUp, %d"), CurrentWeaponIndex);

	SwitchWeapon(CurrentWeaponIndex);
}

void ASCharacter::SwitchWeaponDown()
{
	CurrentWeaponIndex -= 1;
	if (CurrentWeaponIndex < 0)
	{
		CurrentWeaponIndex = 0;
	}
	UE_LOG(LogTemp, Warning, TEXT("SwitchWeaponDown, %d"), CurrentWeaponIndex);

	SwitchWeapon(CurrentWeaponIndex);
}

// Called every frame
void ASCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float TargetFov = bWantsToZoom ? ZoomedFov : DefaultFov;

	float NewFov = FMath::FInterpTo(CameraComp->FieldOfView, TargetFov, DeltaTime, ZoomSpeed);
	CameraComp->FieldOfView = NewFov;

}

// Called to bind functionality to input
void ASCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ASCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASCharacter::MoveRight);

	PlayerInputComponent->BindAxis("LookUp", this, &ASCharacter::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn", this, &ASCharacter::AddControllerYawInput);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ASCharacter::BeginCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &ASCharacter::EndCrouch);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASCharacter::Jump);

	PlayerInputComponent->BindAction("Zoom", IE_Pressed, this, &ASCharacter::BeginZoom);
	PlayerInputComponent->BindAction("Zoom", IE_Released, this, &ASCharacter::EndZoom);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ASCharacter::StartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ASCharacter::StopFire);

	PlayerInputComponent->BindAction("SwitchWeaponUp", IE_Pressed, this, &ASCharacter::SwitchWeaponUp);
	PlayerInputComponent->BindAction("SwitchWeaponDown", IE_Pressed, this, &ASCharacter::SwitchWeaponDown);
}

FVector ASCharacter::GetPawnViewLocation() const
{
	if (CameraComp)
	{
		return CameraComp->GetComponentLocation();
	}

	return Super::GetPawnViewLocation();
}

void ASCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCharacter, CurrentWeapon);
	DOREPLIFETIME(ASCharacter, bDied);
}