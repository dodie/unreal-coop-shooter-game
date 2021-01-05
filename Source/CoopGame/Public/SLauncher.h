// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SWeapon.h"
#include "SLauncher.generated.h"

class AActor;

/**
 * 
 */
UCLASS()
class COOPGAME_API ASLauncher : public ASWeapon
{
	GENERATED_BODY()

public:
	ASLauncher();

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<AActor> ProjectileClass;

protected:
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void Fire() override;
	
};
