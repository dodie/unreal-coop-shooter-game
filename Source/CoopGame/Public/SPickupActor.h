// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SPickupActor.generated.h"

class USphereComponent;
class UDecalComponent;
class ASPowerup;

UCLASS()
class COOPGAME_API ASPickupActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ASPickupActor();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USphereComponent* SphereComp;
	
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UDecalComponent* DecalComp;

	UPROPERTY(EditInstanceOnly, Category = "Powerup")
	TSubclassOf<ASPowerup> PowerupClass;

	UPROPERTY(EditInstanceOnly, Category = "Powerup")
	float Cooldown;

	UFUNCTION()
	void Respawn();

	ASPowerup* PowerupInstance;

	FTimerHandle TimerHandle_Respawn;

	

public:	

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

};
