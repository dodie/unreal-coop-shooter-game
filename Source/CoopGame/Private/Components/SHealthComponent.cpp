// Fill out your copyright notice in the Description page of Project Settings.

#include "SHealthComponent.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"
#include "SGameMode.h"

USHealthComponent::USHealthComponent()
{
	DefaultHealth = 100.0f;
	bIsDead = false;
	TeamNum = 255;
	SetIsReplicated(true);
}

void USHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (GetOwner()->Role == ROLE_Authority)
	{
		AActor* MyOwner = GetOwner();
		if (MyOwner)
		{
			MyOwner->OnTakeAnyDamage.AddDynamic(this, &USHealthComponent::HandleTakeAnyDamage);
		}
	}

	Health = DefaultHealth;
}

void USHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.0f || bIsDead)
	{
		return;
	}

	if (DamagedActor != DamageCauser && IsFriendly(DamagedActor, DamageCauser))
	{
		return;
	}

	Health = FMath::Clamp(Health - Damage, 0.0f, DefaultHealth);

	UE_LOG(LogTemp, Log, TEXT("Health Changed: %s"), *FString::SanitizeFloat(Health));
	OnHealthChanged.Broadcast(this, Health);


	bIsDead = Health <= 0.0f;
	if (bIsDead)
	{
		ASGameMode* GM = Cast<ASGameMode>(GetWorld()->GetAuthGameMode());
		if (GM)
		{
			if (InstigatedBy) // without this check it would crash on events when there's no instigator controller
			{
				GM->OnActorKilled.Broadcast(GetOwner(), InstigatedBy->GetPawn());
			}
		}
	}

}

void USHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USHealthComponent, Health);
}

void USHealthComponent::OnRep_Health()
{
	OnHealthChanged.Broadcast(this, Health);
}

void USHealthComponent::Heal(float HealAmount)
{
	if (HealAmount < 0.0f || Health <= 0.0f)
	{
		return;
	}

	Health = FMath::Clamp(Health + HealAmount, 0.0f, DefaultHealth);

	UE_LOG(LogTemp, Log, TEXT("Health Changed: %s"), *FString::SanitizeFloat(Health));
	OnHealthChanged.Broadcast(this, Health);
}

void USHealthComponent::Damage(float DamageAmount)
{
	if (DamageAmount < 0.0f || Health <= 0.0f)
	{
		return;
	}

	Health = FMath::Clamp(Health - DamageAmount, 0.0f, DefaultHealth);

	UE_LOG(LogTemp, Log, TEXT("Health Changed: %s"), *FString::SanitizeFloat(Health));
	OnHealthChanged.Broadcast(this, Health);
}

float USHealthComponent::GetHealth() const
{
	return Health;
}

bool USHealthComponent::IsFriendly(AActor* ActorA, AActor* ActorB)
{
	if (ActorA == nullptr || ActorB == nullptr)
	{
		// assume non-friendly if it's not an actor (e.g. environmental)
		return false;
	}

	USHealthComponent* HealthCompA = Cast<USHealthComponent>(ActorA->GetComponentByClass(USHealthComponent::StaticClass()));
	USHealthComponent* HealthCompB = Cast<USHealthComponent>(ActorB->GetComponentByClass(USHealthComponent::StaticClass()));

	if (HealthCompA == nullptr || HealthCompB == nullptr)
	{
		// assume non-friendly if it does not have a health component (e.g. environmental)
		return false;
	}

	return HealthCompA->TeamNum == HealthCompB->TeamNum;
}
