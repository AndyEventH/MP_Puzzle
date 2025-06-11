// Fill out your copyright notice in the Description page of Project Settings.


#include "MainMenu.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Components/EditableText.h"
#include "UObject/ConstructorHelpers.h"
#include "ServerRow.h"
#include "Components/TextBlock.h"

UMainMenu::UMainMenu(const FObjectInitializer& ObjectInitializer)
{
	ConstructorHelpers::FClassFinder<UUserWidget> ServerRowBPClass(TEXT("/Game/MenuSystem/WBP_ServerRow"));
	if (ServerRowBPClass.Class == nullptr) return;

	ServerRowClass = ServerRowBPClass.Class;
}

bool UMainMenu::Initialize()
{
	bool Success = Super::Initialize();

	if (!Success) return false;

	if (HostButton == nullptr) return false;
	if (JoinButton == nullptr) return false;
	if (QuitButton == nullptr) return false;
	if (CancelJoinMenuButton == nullptr) return false;
	if (ConfirmJoinMenuButton == nullptr) return false;
	if (CancelHostMenuButton == nullptr) return false;
	if (ConfirmHostMenuButton == nullptr) return false;

	HostButton->OnClicked.AddDynamic(this, &UMainMenu::OpenHostMenu);
	JoinButton->OnClicked.AddDynamic(this, &UMainMenu::OpenJoinMenu);
	QuitButton->OnClicked.AddDynamic(this, &UMainMenu::QuitPressed);
	ConfirmJoinMenuButton->OnClicked.AddDynamic(this, &UMainMenu::JoinServer);
	CancelJoinMenuButton->OnClicked.AddDynamic(this, &UMainMenu::OpenMainMenu);
	CancelHostMenuButton->OnClicked.AddDynamic(this, &UMainMenu::OpenMainMenu);
	ConfirmHostMenuButton->OnClicked.AddDynamic(this, &UMainMenu::HostServer);

	return true;
}

void UMainMenu::HostServer()
{
	if (MenuInterface == nullptr) return;
	FString ServerName = ServerHostName->GetText().ToString();
	MenuInterface->Host(ServerName);
}

void UMainMenu::JoinServer()
{
	if (SelectedIndex.IsSet() && MenuInterface != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Selected index %d"), SelectedIndex.GetValue());
		MenuInterface->Join(SelectedIndex.GetValue());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Index not set"));
	}

	//if (IPAddressField == nullptr) return;
	//const FString& Address = IPAddressField->GetText().ToString();

}

void UMainMenu::QuitPressed()
{
	UWorld* World = GetWorld();
	if (World == nullptr) return;

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (PlayerController == nullptr) return;

	PlayerController->ConsoleCommand("quit");
}

void UMainMenu::OpenMainMenu()
{
	if (MenuSwitcher == nullptr) return;
	if (CancelJoinMenuButton == nullptr) return;
	MenuSwitcher->SetActiveWidget(MainMenu);
}

void UMainMenu::OpenHostMenu()
{
	MenuSwitcher->SetActiveWidget(HostMenu);
}

void UMainMenu::OpenJoinMenu()
{
	if (MenuSwitcher == nullptr) return;
	if (JoinMenu == nullptr) return;
	MenuSwitcher->SetActiveWidget(JoinMenu);

	if (MenuInterface == nullptr) return;
	MenuInterface->RefreshServerList();
}

void UMainMenu::SetServerList(TArray<FServerData> ServerData)
{
	UWorld* World = GetWorld();
	if (World == nullptr) return;

	ServerList->ClearChildren();

	uint32 i = 0;
	for (const FServerData& ServerName : ServerData)
	{
		UServerRow* Row = CreateWidget<UServerRow>(World, ServerRowClass);
		if (Row == nullptr) return;

		Row->ServerName->SetText(FText::FromString(ServerName.Name));
		Row->HostUser->SetText(FText::FromString(ServerName.HostUsername));
		FString Connections = 
			FString::Printf(TEXT("%d / %d"), ServerName.CurrentPlayers, ServerName.MaxPlayers);
		Row->Connections->SetText(FText::FromString(Connections));

		Row->Setup(this, i);
		i++;
		ServerList->AddChild(Row);
	}
}

void UMainMenu::UpdateChildren()
{
	for (int i = 0; i < ServerList->GetChildrenCount(); i++)
	{
		auto Row = Cast<UServerRow>(ServerList->GetChildAt(i));

		if (Row != nullptr)
		{
			Row->Selected = (SelectedIndex.IsSet() && SelectedIndex.GetValue() == i);
		}
	}
}

void UMainMenu::SelectIndex(uint32 Index)
{
	SelectedIndex = Index;
	UpdateChildren();
}


