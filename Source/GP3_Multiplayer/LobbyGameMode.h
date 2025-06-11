// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GP3_MultiplayerGameMode.h"
#include "LobbyGameMode.generated.h"

/**
 * 
 */
UCLASS()
class GP3_MULTIPLAYER_API ALobbyGameMode : public AGP3_MultiplayerGameMode
{
	GENERATED_BODY()
	
private:
	int NumberOfPlayers = 0;

	FTimerHandle GameStartTimer;

	void StartGame();
	void PostLogin(APlayerController* NewPlayer) override;
	void Logout(AController* Exiting) override;
};
