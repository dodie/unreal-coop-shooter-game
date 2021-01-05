// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SGameMode.generated.h"


enum class EWaveState : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActorKilled, AActor*, VictimActor, AActor*, KillerActor);

/**
 * 
 */
UCLASS()
class COOPGAME_API ASGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
protected:

	UFUNCTION(BlueprintImplementableEvent, Category = "GameMode")
	void SpawnNewBot();

	void SpawnBotTimeElapsed();

	void StartWave();

	void EndWave();

	void PrepareForNextWave();

	void CheckWaveState();

	void CheckPlayersState();

	void GameOver();

	FTimerHandle TimerHandle_BotSpawner;

	FTimerHandle TimerHandle_NextWaveStart;

	FTimerHandle TimerHandle_GameOverRestart;

	int32 NrOfBotsToSpawnInCurrentWave;

	int32 WaveCount;

	UPROPERTY(EditDefaultsOnly, Category = "GameMode")
	float TimeBetweenWaves;

	void SetWaveState(EWaveState WaveState);

	void SetWaveCount(int32 WaveCount);

	void RespawnDeadPlayers();

	void RestartGame();

public:
	ASGameMode();

	virtual void StartPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(BlueprintAssignable, Category = "GameMode")
	FOnActorKilled OnActorKilled;
	
};
