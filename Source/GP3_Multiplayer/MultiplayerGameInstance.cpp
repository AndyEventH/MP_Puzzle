// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerGameInstance.h"
#include "Engine/Engine.h"
#include "UObject/ConstructorHelpers.h"
#include "Blueprint/UserWidget.h"
#include "MainMenu.h"
#include "MenuWidget.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"


const static FName SESSION_NAME = TEXT("GP3_Session");
const static FName SERVER_NAME_SETTINGS_KEY = TEXT("ServerName");

UMultiplayerGameInstance::UMultiplayerGameInstance(const FObjectInitializer& ObjectInitializer)
{
	ConstructorHelpers::FClassFinder<UUserWidget> MainMenuWidgetBP(TEXT("/Game/MenuSystem/WBP_MainMenu"));
	if (MainMenuWidgetBP.Class == nullptr) return;

	

	MenuClass = MainMenuWidgetBP.Class;

	ConstructorHelpers::FClassFinder<UUserWidget> InGameWidgetBP(TEXT("/Game/MenuSystem/WBP_InGameMenu"));
	if (InGameWidgetBP.Class == nullptr) return;

	InGameMenuClass = InGameWidgetBP.Class;
}

void UMultiplayerGameInstance::Init()
{
	IOnlineSubsystem* SubSystem = IOnlineSubsystem::Get();

	if (SubSystem != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Found SubSystem %s"), *SubSystem->GetSubsystemName().ToString());
		SessionInterface = SubSystem->GetSessionInterface();

		if (SessionInterface.IsValid())
		{
			SessionInterface->OnCreateSessionCompleteDelegates.
				AddUObject(this, &UMultiplayerGameInstance::OnCreateSessionComplete);
			SessionInterface->OnDestroySessionCompleteDelegates.
				AddUObject(this, &UMultiplayerGameInstance::OnDestroySessionComplete);
			SessionInterface->OnFindSessionsCompleteDelegates.
				AddUObject(this, &UMultiplayerGameInstance::OnFindSessionComplete);
			SessionInterface->OnJoinSessionCompleteDelegates.
				AddUObject(this, &UMultiplayerGameInstance::OnJoinSessionComplete);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to find Subsystem"));
	}

	if (GEngine != nullptr)
		GEngine->OnNetworkFailure().AddUObject(this, &UMultiplayerGameInstance::OnNetworkFailure);

}

void UMultiplayerGameInstance::LoadMenu()
{
	if (MenuClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("No Menu Class Found"));
		return;
	}


	Menu = CreateWidget<UMenuWidget>(this, MenuClass);
	if (Menu == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Impossible to create menu widget"));
		return;
	}

	Menu->Setup();
	Menu->SetMenuInterface(this);
}

void UMultiplayerGameInstance::InGameLoadMenu()
{
	if (InGameMenuClass == nullptr) return;

	InGameMenu = CreateWidget<UMenuWidget>(this, InGameMenuClass);
	if (InGameMenu == nullptr) return;

	InGameMenu->Setup();
	InGameMenu->SetMenuInterface(this);
}

void UMultiplayerGameInstance::Host(FString ServerName)
{
	DesiredServerName = ServerName;

	if (SessionInterface.IsValid())
	{
		auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);

		if (ExistingSession != nullptr)
		{
			SessionInterface->DestroySession(NAME_GameSession);
		}
		else
		{
			CreateSession();
		}

	}
}

void UMultiplayerGameInstance::Join(uint32 Index)
{
	if (!SessionInterface.IsValid()) return;
	if (!SessionSearch.IsValid()) return;

	if (Menu != nullptr)
	{
		Menu->TearDown();
	}

	SessionInterface->JoinSession(0, NAME_GameSession, SessionSearch->SearchResults[Index]);


}

void UMultiplayerGameInstance::StartSession()
{
	if (SessionInterface.IsValid())
		SessionInterface->StartSession(NAME_GameSession);
}

void UMultiplayerGameInstance::LoadMainMenu()
{
	APlayerController* PlayerController = GetFirstLocalPlayerController();
	if (PlayerController == nullptr) return;

	PlayerController->ClientTravel("/Game/ThirdPerson/Maps/MainMenuMap", ETravelType::TRAVEL_Absolute);
}

void UMultiplayerGameInstance::RefreshServerList()
{
	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	if (SessionSearch.IsValid())
	{
		//SessionSearch->bIsLanQuery = true;
		SessionSearch->MaxSearchResults = 100;
		SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

		UE_LOG(LogTemp, Warning, TEXT("Starting Find Session"));
		SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
	}
}

void UMultiplayerGameInstance::OnCreateSessionComplete(FName SessionName, bool Success)
{
	if (!Success)
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not create session"));
	}

	if (Menu != nullptr)
	{
		Menu->TearDown();
	}
	UEngine* Engine = GetEngine();
	if (Engine == nullptr) return;
	Engine->AddOnScreenDebugMessage(0, 2, FColor::Green, TEXT("Hosting"));

	UWorld* World = GetWorld();
	if (World == nullptr) return;

	World->ServerTravel("/Game/ThirdPerson/Maps/LobbyMap?listen");
}

void UMultiplayerGameInstance::OnDestroySessionComplete(FName SessionName, bool Success)
{
	if (Success)
		CreateSession();
}

void UMultiplayerGameInstance::OnFindSessionComplete(bool Success)
{
	if (Success && SessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Finished Finding Session"));

		TArray<FServerData> ServerData;


		for (const FOnlineSessionSearchResult& SearchResult : SessionSearch->SearchResults)
		{
			UE_LOG(LogTemp, Warning, TEXT("Found Session Names: %s"), *SearchResult.GetSessionIdStr());
			FServerData Data;
			Data.Name = SearchResult.GetSessionIdStr();
			Data.MaxPlayers = SearchResult.Session.SessionSettings.NumPublicConnections;
			Data.CurrentPlayers = Data.MaxPlayers - SearchResult.Session.NumOpenPublicConnections;
			Data.HostUsername = SearchResult.Session.OwningUserName;

			FString ServerName;

			if (SearchResult.Session.SessionSettings.Get(SERVER_NAME_SETTINGS_KEY, ServerName))
			{
				Data.Name = ServerName;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Did not find expected data"));
			}


			ServerData.Add(Data);
		}

		Cast<UMainMenu>(Menu)->SetServerList(ServerData);
	}

}

void UMultiplayerGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (!SessionInterface.IsValid()) return;


	UEngine* Engine = GetEngine();
	if (Engine == nullptr) return;



	FString Address;
	if (!SessionInterface->GetResolvedConnectString(SessionName, Address))
	{
		UE_LOG(LogTemp, Warning, TEXT("Could Not get connect string"));
		return;
	}

	Engine->AddOnScreenDebugMessage(0, 2, FColor::Green, FString::Printf(TEXT("Joining %s"), *Address));

	APlayerController* PlayerController = GetFirstLocalPlayerController();
	if (PlayerController == nullptr) return;
	PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);

}

void UMultiplayerGameInstance::CreateSession()
{
	if (SessionInterface.IsValid())
	{
		FOnlineSessionSettings SessionSettings;

		if (IOnlineSubsystem::Get()->GetSubsystemName() == "NULL")
		{
			SessionSettings.bIsLANMatch = true;
		}
		else
		{
			SessionSettings.bIsLANMatch = false;
		}

		SessionSettings.NumPublicConnections = 5;
		SessionSettings.bShouldAdvertise = true;
		SessionSettings.bUsesPresence = true;
		SessionSettings.bUseLobbiesIfAvailable = true;

		SessionSettings.Set(SERVER_NAME_SETTINGS_KEY, DesiredServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

		SessionInterface->CreateSession(0, NAME_GameSession, SessionSettings);
	}
}

void UMultiplayerGameInstance::OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	LoadMainMenu();
}
