// Fill out your copyright notice in the Description page of Project Settings.

#include "SWeapon.h"
#include "CoopGame.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "TimerManager.h"
#include "Sound/SoundCue.h"
#include "Net/UnrealNetwork.h"

ASWeapon::ASWeapon()
{
	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MuzzleSocketName = "MuzzleSocket";
	TracerTargetName = "Target";
	BaseDamage = 20.0f;
	RateOfFire = 600; // bullets per minute
	BulletSpreadInDegrees = 2.0f;

	SetReplicates(true);
	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;
}

void ASWeapon::BeginPlay()
{
	Super::BeginPlay();

	TimeBetweenShots = 60 / RateOfFire;
}

void ASWeapon::StartFire()
{
	float FirstDelay = FMath::Max(LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f);
	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &ASWeapon::Fire, TimeBetweenShots, true, FirstDelay);
}

void ASWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
}

void ASWeapon::Fire()
{
	if (Role < ROLE_Authority)
	{
		ServerFire();
	}

	AActor* MyOwner = GetOwner();
	if (MyOwner)
	{
		FVector EyeLocation;
		FRotator EyeRotation;

		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);
		// viewpoint is by default the actor's location + eye height
		// now it's overriden at ASCharacter::GetPawnViewLocation to return the camera position

		// bullet spread
		float HalfRad = FMath::DegreesToRadians(BulletSpreadInDegrees);
		FVector ShotDirection = FMath::VRandCone(EyeRotation.Vector(), HalfRad, HalfRad);
		
		/*
		Using EyeLocation as a TraceStart would work, but if you are standing in front of a wall
		you might accidentally hit the wall behind you as the camera might overlap with it.
		*/
		FVector TraceStart = EyeLocation + (ShotDirection * 100);
		FVector TraceEnd = EyeLocation + (ShotDirection * 10000);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(MyOwner);
		QueryParams.AddIgnoredActor(this);
		
		// more expensive, but trace against each triangle individually for more precise hits
		// without this, only a bounding box detection would happen
		QueryParams.bTraceComplex = true;

		// needed for physics detection
		QueryParams.bReturnPhysicalMaterial = true;

		FHitResult HitResult;
		FVector TracerEndPoint = TraceEnd;

		EPhysicalSurface SurfaceType = SurfaceType_Default;

		// In Project Settings ->Engine->Collision there's a custom channel defined in slot 1, which is now used for collision detection
		// instead of e.g. the visibility channel
		bool BlockingHit = GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, COLLISION_WEAPON, QueryParams);
		if (BlockingHit)
		{
			SurfaceType = UPhysicalMaterial::DetermineSurfaceType(HitResult.PhysMaterial.Get()); // PhysMat is a weak ref

			float ActualDamage = BaseDamage;
			if (SurfaceType == SURFACE_FLESHVULNERABLE)
			{
				ActualDamage *= 4;
			}

			// at some point it came that this apply damage should only be done on the server side, because the client shouldn't manage
			// HPs. However, because ApplyPointDamage also affects physics, let's execute it everywhere and worry about the HP in the SHealthComponent.
			// TODO: check back on this later because I think now it runs both on client and on server and if I sync the pawn it will be bad
			AActor* HitActor = HitResult.GetActor();
			UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection, HitResult, MyOwner->GetInstigatorController(), MyOwner, DamageType);
		
			PlayImpactEffects(SurfaceType, HitResult.ImpactPoint);
			TracerEndPoint = HitResult.ImpactPoint;
		}

		PlayFireEffects(TracerEndPoint);

		if (Role == ROLE_Authority)
		{
			float RandomFloatValue = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			FVector DummyVector = FVector(RandomFloatValue);
			HitScanTrace.Dummy = DummyVector; // just to make sure we replicate even if nothing is changes, so firing at the same spot will be noticed by the client
			HitScanTrace.TraceTo = TracerEndPoint;
			HitScanTrace.SurfaceType = SurfaceType;
		}

		LastFireTime = GetWorld()->TimeSeconds;
	}
}

void ASWeapon::ServerFire_Implementation()
{
	Fire();
}

bool ASWeapon::ServerFire_Validate()
{
	return true;
}

void ASWeapon::OnRep_HitScanTrace()
{
	PlayFireEffects(HitScanTrace.TraceTo);
	PlayImpactEffects(HitScanTrace.SurfaceType, HitScanTrace.TraceTo);
}

void ASWeapon::PlayFireEffects(FVector TracerEndPoint)
{
	if (MuzzleEffect)
	{
		// attach muzzle effect to the rifle
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);
	}

	if (FireSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(this, FireSound, GetActorLocation());
	}

	if (TracerEffect)
	{
		// This is a special emitter, requires a "Target" to be set
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
		UParticleSystemComponent* TracerComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerEffect, MuzzleLocation);
		if (TracerComp)
		{
			TracerComp->SetVectorParameter(TracerTargetName, TracerEndPoint);
		}
	}

	APawn* MyOwner = Cast<APawn>(GetOwner());
	if (MyOwner)
	{
		APlayerController* PC = Cast<APlayerController>(MyOwner->GetController());
		if (PC)
		{
			PC->ClientPlayCameraShake(FireCamShake);
		}
	}
}

void ASWeapon::PlayImpactEffects(EPhysicalSurface SurfaceType, FVector ImpactPoint)
{
	UParticleSystem* SelectedEffect = nullptr;
	switch (SurfaceType)
	{
	case SURFACE_FLESHDEFAULT:
	case SURFACE_FLESHVULNERABLE:
		SelectedEffect = FleshImpactEffect;
		break;
	default:
		SelectedEffect = DefaultImpactEffect;
		break;
	}

	if (SelectedEffect)
	{
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
		FVector ShotDirection = ImpactPoint - MuzzleLocation;
		ShotDirection.Normalize();

		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, ImpactPoint, ShotDirection.Rotation());
	}
}

void ASWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ASWeapon, HitScanTrace, COND_SkipOwner);
}
