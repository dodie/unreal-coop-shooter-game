// Fill out your copyright notice in the Description page of Project Settings.

#include "SBarrel.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "Particles/ParticleSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SHealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundCue.h"

ASBarrel::ASBarrel()
{
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetSimulatePhysics(true);
	MeshComp->SetCollisionObjectType(ECC_PhysicsBody);
	RootComponent = MeshComp;

	RadialForceComp = CreateDefaultSubobject<URadialForceComponent>(TEXT("RadialForceComp"));
	RadialForceComp->SetupAttachment(MeshComp);
	RadialForceComp->Radius = 250;
	RadialForceComp->bImpulseVelChange = true;
	RadialForceComp->bAutoActivate = false;
	RadialForceComp->bIgnoreOwningActor = true;

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));
	HealthComp->OnHealthChanged.AddDynamic(this, &ASBarrel::OnHealthChanged);

	SetReplicates(true);
	SetReplicateMovement(true); // replicating physics movement is expensive, but in this case it's part of the gameplay
	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;

	HealthComp->TeamNum = 254;
}

void ASBarrel::OnHealthChanged(USHealthComponent* HealthComp, float Health)
{
	if (Health <= 0.0f && !bDied)
	{
		bDied = true;

		ExplosionVisuals();

		FVector UpImpulse = FVector::UpVector * 200;
		MeshComp->AddImpulse(UpImpulse, NAME_None, true); // We are not replicating the phy stuff for efficiency. If the client would do something about physics, it should be invoked from OnRep_...

		// For details, see projectile
		TArray<AActor*> IgnoredActors;
		UGameplayStatics::ApplyRadialDamage(GetWorld(), 100.0f, GetActorLocation(), 200.0f, DamageType, IgnoredActors, this, GetInstigatorController(), true, ECollisionChannel::ECC_Visibility);
		
		RadialForceComp->FireImpulse();

		GetWorldTimerManager().SetTimer(TimerHandle_FinalDetonation, this, &ASBarrel::FinalDetonation, FMath::FRandRange(3.0f, 20.0f), false);
	}
}

void ASBarrel::ExplosionVisuals()
{
	if (ExplosionEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, RootComponent->GetComponentLocation(), RootComponent->GetComponentRotation());
	}

	if (ExplodedMaterial)
	{
		MeshComp->SetMaterial(0, ExplodedMaterial);
	}

	if (ExplosionSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(this, ExplosionSound, GetActorLocation());
	}
}

void ASBarrel::OnRep_Exploded()
{
	ExplosionVisuals();
}

void ASBarrel::FinalDetonation()
{
	ExplosionVisuals();
	Destroy();
}

void ASBarrel::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASBarrel, bDied);
}