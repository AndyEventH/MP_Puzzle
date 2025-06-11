// Fill out your copyright notice in the Description page of Project Settings.

#include "LobbyGameMode.h"
#include "TimerManager.h"
#include "MultiplayerGameInstance.h"

void ALobbyGameMode::StartGame()
{
	auto GameInstance = Cast<UMultiplayerGameInstance>(GetGameInstance());

	if (GameInstance == nullptr) return;

	GameInstance->StartSession();

	UWorld* World = GetWorld();

	if (World == nullptr) return;

	if (HasAuthority())
	{
		SetReplicates(true);
		SetReplicateMovement(true);
	}

	bUseSeamlessTravel = true;
	World->ServerTravel("/Game/ThirdPerson/Maps/ThirdPersonMap?listen");
}

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	NumberOfPlayers++;

	if (NumberOfPlayers >= 2)
	{
		GetWorldTimerManager().SetTimer(GameStartTimer, this, &ALobbyGameMode::StartGame, 10);
	}
}

void ALobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	NumberOfPlayers--;
}