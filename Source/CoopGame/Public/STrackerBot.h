// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "STrackerBot.generated.h"

class UStaticMeshComponent;
class USHealthComponent;
class UMaterialInstanceDynamic;
class UParticleSystem;
class URadialForceComponent;
class USphereComponent;
class USoundCue;
class UAudioComponent;

UCLASS()
class COOPGAME_API ASTrackerBot : public APawn
{
	GENERATED_BODY()

public:
	ASTrackerBot();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	USHealthComponent* HealthComp;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	USphereComponent* SphereComp;

	FVector GetNextPathPoint();

	FVector NextPathPoint;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	bool bUseVelocityChange;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	float MovementForce;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	float RequiredDistanceToTarget;

	UMaterialInstanceDynamic* MatInst;

	UFUNCTION()
	void OnHealthChanged(USHealthComponent* OwningHealthComp, float Health);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Effects")
	UParticleSystem* ExplosionEffect;

	UPROPERTY()
	bool bDied;

	void SelfDestruct();

	void SelfDamage();

	void PlayExplosionEffects();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<UDamageType> DamageType;

	URadialForceComponent* RadialForceComp;

	FTimerHandle TimerHandle_SelfDestruct;

	bool bStartedSelfDestruct;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundCue* SelfDestructSound;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundCue* ExplodeSound;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	UAudioComponent* RollSoundComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundCue* RollSound;

	void OnCheckNearbyBots();

	int32 PowerLevel;

	FTimerHandle TimerHandle_CheckBots;

	FTimerHandle TimerHandle_RefreshPath;

	void RefreshPath();

public:	

	virtual void Tick(float DeltaTime) override;

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
};
