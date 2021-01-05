// Fill out your copyright notice in the Description page of Project Settings.

#include "SProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"

// TODO: approximate initial position on clients
ASProjectile::ASProjectile()
{
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(5.0f);
	CollisionComp->SetCollisionProfileName("Projectile");

	CollisionComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	CollisionComp->CanCharacterStepUpOn = ECB_No;
	// CCD is enabled in Blueprint to ensure the projectile does not go through walls

	//CollisionComp->IgnoreActorWhenMoving(GetInstigatorController()->GetPawn(), true);
	// TODO: sometimes this collides with the gun. The above line was a futile attempt to help this

	RootComponent = CollisionComp;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 3000.f;
	ProjectileMovement->MaxSpeed = 3000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = true;

	RadialForceComp = CreateDefaultSubobject<URadialForceComponent>(TEXT("RadialForceComp"));
	RadialForceComp->SetupAttachment(CollisionComp);
	RadialForceComp->Radius = 250;
	RadialForceComp->bImpulseVelChange = true;
	RadialForceComp->bAutoActivate = false;
	RadialForceComp->bIgnoreOwningActor = true;

	SetReplicates(true);
	SetReplicateMovement(true);
	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;

	bExploded = false;
}

void ASProjectile::BeginPlay()
{
	Super::BeginPlay();
	GetWorldTimerManager().SetTimer(TimerHandle_Explosion, this, &ASProjectile::Explode, 1.0f, false);
}

void ASProjectile::Explode()
{
	bExploded = true;

	ExplosionVisuals();

	TArray<AActor*> IgnoredActors;//TODO: the this in the line below is not good, as the projectile will be the damage causer, not the player who shot
	UGameplayStatics::ApplyRadialDamage(GetWorld(), 100.0f, GetActorLocation(), 200.0f, DamageType, IgnoredActors, this, GetInstigatorController(), true, ECollisionChannel::ECC_Visibility);
	//  The receiver of the damage needs to be set to a Custom Collision preset and Object Type switched from WorldStatic to WorldDynamic for it to register.

	RadialForceComp->FireImpulse();

	GetWorldTimerManager().SetTimer(TimerHandle_Explosion, this, &ASProjectile::ExplodeCleanup, 0.2f, false);

	// TODO:
	//MeshComp->SetVisibility(false, true);
	//MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASProjectile::ExplodeCleanup()
{
	Destroy();
}

void ASProjectile::ExplosionVisuals()
{
	if (bExploded)
	{
		if (ExplosionEffect)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, RootComponent->GetComponentLocation(), RootComponent->GetComponentRotation());
			CollisionComp->SetVisibility(false, true);
			ProjectileMovement->DestroyComponent(true);
		}

		if (ExplosionSound)
		{
			UGameplayStatics::SpawnSoundAtLocation(this, ExplosionSound, GetActorLocation());
		}
	}
}

void ASProjectile::OnRep_Exploded()
{
	ExplosionVisuals();
}

void ASProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ASProjectile, bExploded, COND_SkipOwner);
}