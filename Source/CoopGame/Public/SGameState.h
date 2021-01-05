// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "SGameState.generated.h"

UENUM(BlueprintType)
enum class EWaveState : uint8
{
	WaitingToStart,
	WaveInProgress,
	WaitingToComplete,
	WaveComplete,
	GameOver
};

UCLASS()
class COOPGAME_API ASGameState : public AGameStateBase
{
	GENERATED_BODY()

protected:

	UFUNCTION()
	void OnRep_WaveState(EWaveState OldState);

	UFUNCTION(BlueprintImplementableEvent, Category = "GameState")
	void WaveStateChanged(EWaveState NewState, EWaveState OldState);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_WaveState, Category = "GameState")
	EWaveState WaveState;

	UFUNCTION()
	void OnRep_WaveCount(int32 OldWaveCount);

	UFUNCTION(BlueprintImplementableEvent, Category = "GameState")
	void WaveCountChanged(int32 NewWaveCount, int32 OldWaveCount);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_WaveCount, Category = "GameState")
	int32 WaveCount;

public:

	void SetWaveState(EWaveState NewWaveState);

	EWaveState GetWaveState();

	void SetWaveCount(int32 WaveCount);

	int32 GetWaveCount();

};
