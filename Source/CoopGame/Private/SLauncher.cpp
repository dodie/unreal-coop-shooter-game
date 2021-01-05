// Fill out your copyright notice in the Description page of Project Settings.

#include "SLauncher.h"
#include "Runtime/Engine/Classes/Engine/World.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"

ASLauncher::ASLauncher():ASWeapon()
{
	RateOfFire = 60;
}

void ASLauncher::Fire()
{
	if (FireSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(this, FireSound, GetActorLocation());
	}

	if (Role < ROLE_Authority)
	{
		ServerFire();
		return;
	}

	if (ProjectileClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("No ProjecticleClass defined for SLauncher!"));
		return;
	}

	AActor* MyOwner = GetOwner();
	if (MyOwner == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("No Owner defined for SLauncher!"));
		return;
	}

	FVector MuzzleLocation = MeshComp->GetSocketLocation("MuzzleSocket");

	FVector EyeLocation;
	FRotator EyeRotation;
	MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

	//Set Spawn Collision Handling Override
	FActorSpawnParameters ActorSpawnParams;
	ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ActorSpawnParams.Instigator = MyOwner->GetInstigatorController()->GetPawn();
	
	// spawn the projectile at the muzzle
	GetWorld()->SpawnActor<AActor>(ProjectileClass, MuzzleLocation, EyeRotation, ActorSpawnParams);

	LastFireTime = GetWorld()->TimeSeconds;
}

