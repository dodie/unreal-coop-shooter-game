// Fill out your copyright notice in the Description page of Project Settings.

#include "SGameMode.h"
#include "SBarrel.h"
#include "TimerManager.h"
#include "Components/SHealthComponent.h"
#include "SGameState.h"
#include "SPlayerState.h"


ASGameMode::ASGameMode()
{
	GameStateClass = ASGameState::StaticClass();
	PlayerStateClass = ASPlayerState::StaticClass();

	TimeBetweenWaves = 5.0f;

	PrimaryActorTick.bCanEverTick = true;
	SetActorTickInterval(1.0f);
	SetActorTickEnabled(true);
}

void ASGameMode::SetWaveState(EWaveState WaveState)
{
	ASGameState* GS = GetGameState<ASGameState>();
	if (ensureAlways(GS))
	{
		GS->SetWaveState(WaveState);
	}
}

void ASGameMode::SetWaveCount(int32 NewWaveCount)
{
	WaveCount = NewWaveCount;
	ASGameState* GS = GetGameState<ASGameState>();
	if (ensureAlways(GS))
	{
		GS->SetWaveCount(WaveCount);
	}
}

void ASGameMode::RespawnDeadPlayers()
{
	// check if there are any alive players
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();

		if (PC && PC->GetPawn() == nullptr)
		{
			UE_LOG(LogTemp, Log, TEXT("RespawnDeadPlayer"));
			RestartPlayer(PC);
		}
	}
}

void ASGameMode::RestartGame()
{
	UE_LOG(LogTemp, Log, TEXT("RestartGame"));

	// Reset wave count
	SetWaveCount(0);

	// Remove bots
	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		APawn* TestPawn = It->Get();

		if (TestPawn == nullptr)
		{
			continue;
		}

		USHealthComponent* HealthComp = Cast<USHealthComponent>(TestPawn->GetComponentByClass(USHealthComponent::StaticClass()));
		if (HealthComp && HealthComp->GetHealth() > 0)
		{
			HealthComp->Damage(1000);
		}
	}

	// Remove all barrels
	for (TObjectIterator<ASBarrel> Itr; Itr; ++Itr)
	{
		if (Itr->IsA(ASBarrel::StaticClass()))
		{
			Itr->Destroy();
		}
	}

	// Prepare next wave
	PrepareForNextWave();
}

void ASGameMode::SpawnBotTimeElapsed()
{
	UE_LOG(LogTemp, Log, TEXT("SpawnBotTimeElapsed"));

	SpawnNewBot();
	NrOfBotsToSpawnInCurrentWave--;

	if (NrOfBotsToSpawnInCurrentWave <= 0)
	{
		EndWave();
	}
}

void ASGameMode::StartWave()
{
	UE_LOG(LogTemp, Log, TEXT("StartWave"));

	SetWaveCount(WaveCount + 1);
	NrOfBotsToSpawnInCurrentWave = 2 * WaveCount;
	GetWorldTimerManager().SetTimer(TimerHandle_BotSpawner, this, &ASGameMode::SpawnBotTimeElapsed, 1.0f, true, 0.0f);

	SetWaveState(EWaveState::WaveInProgress);
}

void ASGameMode::EndWave()
{
	UE_LOG(LogTemp, Log, TEXT("EndWave"));

	GetWorldTimerManager().ClearTimer(TimerHandle_BotSpawner);

	SetWaveState(EWaveState::WaitingToComplete);
}

void ASGameMode::PrepareForNextWave()
{
	UE_LOG(LogTemp, Log, TEXT("PrepareForNextWave"));
	RespawnDeadPlayers();
	GetWorldTimerManager().SetTimer(TimerHandle_NextWaveStart, this, &ASGameMode::StartWave, TimeBetweenWaves, false);

	SetWaveState(EWaveState::WaitingToStart);
}

void ASGameMode::CheckWaveState()
{

	// spawning in current wave is still ongoing, nothing to do
	if (NrOfBotsToSpawnInCurrentWave > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("CheckWaveState: still spawning"));

		return;
	}

	// we are already preparing for the next wave, nothing to do
	if (GetWorldTimerManager().IsTimerActive(TimerHandle_NextWaveStart))
	{
		UE_LOG(LogTemp, Log, TEXT("CheckWaveState: already preparing"));

		return;
	}

	// check if there are any alive bots in the current wave, if so, nothing to do
	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		APawn* TestPawn = It->Get();

		if (TestPawn == nullptr || TestPawn->IsPlayerControlled())
		{
			continue;
		}

		USHealthComponent* HealthComp = Cast<USHealthComponent>(TestPawn->GetComponentByClass(USHealthComponent::StaticClass()));
		if (HealthComp && HealthComp->GetHealth() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("CheckWaveState: bots still alive: %s"), *TestPawn->GetName());

			return;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("CheckWaveState: starting next round"));
	PrepareForNextWave();

	SetWaveState(EWaveState::WaveComplete);
}

void ASGameMode::CheckPlayersState()
{
	// check if there are any alive players
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();

		if (PC && PC->GetPawn())
		{
			APawn* MyPawn = PC->GetPawn();
			USHealthComponent* HealthComp = Cast<USHealthComponent>(MyPawn->GetComponentByClass(USHealthComponent::StaticClass()));

			if (ensure(HealthComp) && HealthComp->GetHealth() > 0)
			{
				UE_LOG(LogTemp, Log, TEXT("CheckPlayersState: player is still alive: %s"), *MyPawn->GetName());
				return;
			}
		}
	}

	// check if game already ended
	ASGameState* GS = GetGameState<ASGameState>();
	if (ensureAlways(GS))
	{
		if (GS->GetWaveState() == EWaveState::GameOver)
		{
			return;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("CheckWaveState: ending game"));
	GameOver();
}

void ASGameMode::GameOver()
{
	UE_LOG(LogTemp, Log, TEXT("GameOver"));

	EndWave();

	SetWaveState(EWaveState::GameOver);
	GetWorldTimerManager().SetTimer(TimerHandle_NextWaveStart, this, &ASGameMode::RestartGame, TimeBetweenWaves, false);
}

void ASGameMode::StartPlay()
{
	UE_LOG(LogTemp, Log, TEXT("StartPlay"));

	Super::StartPlay();
	PrepareForNextWave();
}

void ASGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	CheckWaveState();
	CheckPlayersState();
}
