// Fill out your copyright notice in the Description page of Project Settings.

#include "SPowerup.h"
#include "Net/UnrealNetwork.h"


ASPowerup::ASPowerup()
{
	PrimaryActorTick.bCanEverTick = true;

	PowerupInterval = 0.0f;
	TotalNrOfTicks = 0;
	TicksProcessed = 0;

	bIsPowerupActive = false;

	SetReplicates(true);
}

void ASPowerup::OnTickPowerup()
{
	TicksProcessed++;

	if (TotalNrOfTicks <= TicksProcessed)
	{
		OnExpired();
		bIsPowerupActive = false; // TODO: sztem ez nem kell
		OnRep_PowerupActive();
		GetWorldTimerManager().ClearTimer(TimerHandle_PowerupTick);
	}

	OnPowerupTicked();
}

void ASPowerup::OnRep_PowerupActive()
{
	OnPowerupStateChanged(bIsPowerupActive);
}

void ASPowerup::ActivatePowerup(AActor* ActivateFor)
{
	OnActivated(ActivateFor);
	bIsPowerupActive = true;
	OnRep_PowerupActive();

	if (PowerupInterval > 0)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_PowerupTick, this, &ASPowerup::OnTickPowerup, PowerupInterval, true);
	}
	else
	{
		OnTickPowerup();
	}
}


void ASPowerup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASPowerup, bIsPowerupActive);
}