// Fill out your copyright notice in the Description page of Project Settings.


#include "InGameMenu.h"
#include "Components/Button.h"

bool UInGameMenu::Initialize()
{
	bool Success = Super::Initialize();
	if (!Success) return false;

	if (ResumeButton == nullptr) return false;
	ResumeButton->OnClicked.AddDynamic(this, &UInGameMenu::ResumePressed);

	if (QuitButton == nullptr) return false;
	QuitButton->OnClicked.AddDynamic(this, &UInGameMenu::QuitPressed);

	return true;
}

void UInGameMenu::ResumePressed()
{
	TearDown();
}

void UInGameMenu::QuitPressed()
{
	if (MenuInterface == nullptr) return;

	TearDown();
	MenuInterface->LoadMainMenu();
}
