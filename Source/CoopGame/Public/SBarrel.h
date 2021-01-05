// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SBarrel.generated.h"


class URadialForceComponent;
class UParticleSystem;
class USHealthComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UDamageType;
class USoundCue;

UCLASS()
class COOPGAME_API ASBarrel : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASBarrel();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComp;

	URadialForceComponent* RadialForceComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Effects")
	UParticleSystem* ExplosionEffect;

	USHealthComponent* HealthComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Material")
	UMaterialInterface* ExplodedMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<UDamageType> DamageType;
	
	UFUNCTION()
	void OnHealthChanged(USHealthComponent* OwningHealthComp, float Health);

	UPROPERTY(ReplicatedUsing = OnRep_Exploded)
	bool bDied;

	void ExplosionVisuals();

	UFUNCTION()
	void OnRep_Exploded();

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundCue* ExplosionSound;

	FTimerHandle TimerHandle_FinalDetonation;

	void FinalDetonation();

};
