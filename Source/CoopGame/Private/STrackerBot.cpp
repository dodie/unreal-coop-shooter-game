// Fill out your copyright notice in the Description page of Project Settings.

#include "STrackerBot.h"
#include "SCharacter.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "AI/Navigation/NavigationSystem.h"
#include "AI/Navigation/NavigationPath.h"
#include "Components/SHealthComponent.h"
#include "Components/SphereComponent.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "EnvironmentQuery/EnvQuery.h"

ASTrackerBot::ASTrackerBot()
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshCom"));
	MeshComp->SetCanEverAffectNavigation(false);
	MeshComp->SetSimulatePhysics(true);
	RootComponent = MeshComp;

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));
	HealthComp->OnHealthChanged.AddDynamic(this, &ASTrackerBot::OnHealthChanged);

	RadialForceComp = CreateDefaultSubobject<URadialForceComponent>(TEXT("RadialForceComp"));
	RadialForceComp->SetupAttachment(MeshComp);
	RadialForceComp->Radius = 250;
	RadialForceComp->bImpulseVelChange = true;
	RadialForceComp->bAutoActivate = false;
	RadialForceComp->bIgnoreOwningActor = true;

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->SetSphereRadius(200);
	SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);   // No actual physics
	SphereComp->SetCollisionResponseToAllChannels(ECR_Ignore);       // This and next line: ignore other channels for less event and cheaper computation
	SphereComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereComp->SetupAttachment(MeshComp);

	RollSoundComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("RollSoundComponent"));
	RollSoundComponent->SetupAttachment(MeshComp);
	RollSoundComponent->bAutoActivate = true;

	bUseVelocityChange = true;
	RequiredDistanceToTarget = 200;
	MovementForce = 750;
}

void ASTrackerBot::BeginPlay()
{
	Super::BeginPlay();

	if (Role == ROLE_Authority)
	{
		NextPathPoint = GetNextPathPoint();
		GetWorldTimerManager().SetTimer(TimerHandle_CheckBots, this, &ASTrackerBot::OnCheckNearbyBots, 1.0f, true);
	}

	RollSoundComponent->SetSound(RollSound);
	RollSoundComponent->Play();
}

FVector ASTrackerBot::GetNextPathPoint()
{
	// TODO: try to run EQS to find 
	AActor* BestTarget = nullptr;
	float NearestTargetDistance = FLT_MAX;

	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		APawn* TestPawn = It->Get();

		if (TestPawn == nullptr || USHealthComponent::IsFriendly(this, TestPawn))
		{
			continue;
		}

		USHealthComponent* HealthComp = Cast<USHealthComponent>(TestPawn->GetComponentByClass(USHealthComponent::StaticClass()));
		if (HealthComp == nullptr || HealthComp->GetHealth() <= 0)
		{
			continue;
		}

		float Distance = (TestPawn->GetActorLocation() - GetActorLocation()).Size();
		if (Distance < NearestTargetDistance)
		{
			NearestTargetDistance = Distance;
			BestTarget = TestPawn;
		}
	}

	if (BestTarget == nullptr)
	{
		return GetActorLocation();
	}

	UNavigationPath* NavPath = UNavigationSystem::FindPathToActorSynchronously(this, GetActorLocation(), BestTarget);

	GetWorldTimerManager().ClearTimer(TimerHandle_RefreshPath);
	GetWorldTimerManager().SetTimer(TimerHandle_RefreshPath, this, &ASTrackerBot::RefreshPath, 5.0f, false);
	
	if (NavPath && NavPath->PathPoints.Num() > 1)
	{
		// return next path point
		return NavPath->PathPoints[1]; // 0th element is the current location
	}

	// Failed to find path, return current location.
	return GetActorLocation();
}

void ASTrackerBot::OnHealthChanged(USHealthComponent* OwningHealthComp, float Health)
{
	// Explode on death
	if (Health <= 0 && !bDied)
	{
		SelfDestruct();
	}

	// Pulse when hit
	if (MatInst == nullptr)
	{
		// By default all instances use the same material, so if we'd change a parameter on it, all objects would be affected.
		// To change a parameter on a single instance's material we have to create and save the material instance based on the original one.
		MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
	}

	if (MatInst)
	{
		// We need this null check because it's still not guaranteed to have a material here, because if there's no material set globally,
		// we can't create a dynamic instance from it
		MatInst->SetScalarParameterValue("LastTimeDamageTaken", GetWorld()->TimeSeconds);
	}
}

void ASTrackerBot::SelfDestruct()
{
	bDied = true;

	PlayExplosionEffects();
	UGameplayStatics::SpawnSoundAtLocation(this, ExplodeSound, GetActorLocation());

	if (Role == ROLE_Authority)
	{
		// For details, see projectile
		TArray<AActor*> IgnoredActors;
		UGameplayStatics::ApplyRadialDamage(GetWorld(), 40.0f * PowerLevel, GetActorLocation(), 200.0f, DamageType, IgnoredActors, this, GetInstigatorController(), true, ECollisionChannel::ECC_Visibility);

		RadialForceComp->FireImpulse();
		SetLifeSpan(0.2f); 
		
		// If we'd immediately Destroy() it, then it would leave no time for the attached effects to play (for some reason, it only surfaces in multiplayer clients)
		// Another approach is to use a timer, like I did in SProjectile, but this is more lean for sure.
		// Note: the time we wait does not have to correlate with the length of the effects... as long as it's not too short
	}
}

void ASTrackerBot::SelfDamage()
{
	UGameplayStatics::ApplyDamage(this, 10, GetInstigatorController(), this, nullptr);
}

void ASTrackerBot::PlayExplosionEffects()
{
	MeshComp->SetVisibility(false, true);
	//MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Do we need this? It spits some warnings. Maybe we should call stop.

	if (ExplosionEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, RootComponent->GetComponentLocation(), RootComponent->GetComponentRotation());
	}
}

void ASTrackerBot::OnCheckNearbyBots()
{
	const float Radius = 600;

	FCollisionShape CollShape;
	CollShape.SetSphere(600.0f);

	FCollisionObjectQueryParams QueryParams;
	QueryParams.AddObjectTypesToQuery(ECC_PhysicsBody); // other objects we detect have to have it's mesh comp set to phy body
	QueryParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, QueryParams, CollShape);

	int32 NrOfBots = 0;
	for (FOverlapResult Result : Overlaps)
	{
		ASTrackerBot* Bot = Cast<ASTrackerBot>(Result.GetActor());
		if (Bot && Bot != this)
		{
			NrOfBots++;
		}
	}

	const int32 MaxPower = 4;

	PowerLevel = FMath::Clamp(NrOfBots, 0, MaxPower) + 1;

	// Pulse 
	if (MatInst == nullptr)
	{
		MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
	}

	if (MatInst)
	{
		MatInst->SetScalarParameterValue("RedBlinkFrequency", PowerLevel);
		MatInst->SetScalarParameterValue("RedBlinkPower", PowerLevel / 2.0f);
	}
}

void ASTrackerBot::RefreshPath()
{
	NextPathPoint = GetNextPathPoint();
}

void ASTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Role == ROLE_Authority && !bDied)
	{
		float DistanceToTarget = (GetActorLocation() - NextPathPoint).Size();

		if (DistanceToTarget <= RequiredDistanceToTarget)
		{
			NextPathPoint = GetNextPathPoint();
		}
		else
		{
			FVector Direction = NextPathPoint - GetActorLocation();
			Direction.Normalize();
			Direction *= MovementForce;

			MeshComp->AddForce(Direction, NAME_None, bUseVelocityChange);

			//DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + Direction, 32, FColor::Yellow, false, 0.0f, 0, 1.0f);
			//DrawDebugSphere(GetWorld(), NextPathPoint, 32.0f, 12, FColor::Yellow, false, 0.0f);
		}
	}

	// Adjust roll sound - for the draft check the blueprint version
	float VolumeMultiplier = FMath::GetMappedRangeValueClamped(
		FVector2D(10.0f, 1000.0f),
		FVector2D(0.1f, 2.0f), // set 0.1 as a minimum otherwise it will disable the sound component
		GetVelocity().Size());
	RollSoundComponent->SetVolumeMultiplier(VolumeMultiplier);
}

void ASTrackerBot::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	
	if (!bStartedSelfDestruct)
	{
		ASCharacter* Character = Cast<ASCharacter>(OtherActor);
		
		if (Character && Character->IsPlayerControlled() && !USHealthComponent::IsFriendly(OtherActor, this))
		{
			UGameplayStatics::SpawnSoundAttached(SelfDestructSound, RootComponent);
			if (Role == ROLE_Authority)
			{
				GetWorldTimerManager().SetTimer(TimerHandle_SelfDestruct, this, &ASTrackerBot::SelfDamage, 0.1f, true);
				bStartedSelfDestruct = true;
			}
		}
	}
	
}