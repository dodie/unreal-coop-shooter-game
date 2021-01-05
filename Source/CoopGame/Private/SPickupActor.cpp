// Fill out your copyright notice in the Description page of Project Settings.

#include "SPickupActor.h"
#include "SPowerup.h"
#include "Components/SphereComponent.h"
#include "Components/DecalComponent.h"
#include "TimerManager.h"


ASPickupActor::ASPickupActor()
{
	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->SetSphereRadius(75.0f);
	RootComponent = SphereComp;

	DecalComp = CreateDefaultSubobject<UDecalComponent>(TEXT("DecalComp"));
	DecalComp->SetupAttachment(RootComponent);
	DecalComp->SetRelativeRotation(FRotator(90.0f, 0, 0));
	DecalComp->DecalSize = FVector(64.0f, 75.0f, 75.0f);

	Cooldown = 10.0f;

	SetReplicates(true);
}

void ASPickupActor::BeginPlay()
{
	Super::BeginPlay();
	if (Role == ROLE_Authority)
	{
		Respawn();
	}
}

void ASPickupActor::Respawn()
{
	if (PowerupClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Can't spawn powerup for %s: no PowerupClass is defined!"), *GetName());
		return;
	}
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	PowerupInstance = GetWorld()->SpawnActor<ASPowerup>(PowerupClass, GetTransform(), SpawnParams);
}

void ASPickupActor::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (Role == ROLE_Authority && PowerupInstance != nullptr)
	{
		PowerupInstance->ActivatePowerup(OtherActor);
		PowerupInstance = nullptr;
		GetWorldTimerManager().SetTimer(TimerHandle_Respawn, this, &ASPickupActor::Respawn, Cooldown);
	}
}

