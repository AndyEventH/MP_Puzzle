// Copyright Epic Games, Inc. All Rights Reserved.

#include "GP3_MultiplayerGameMode.h"
#include "GP3_MultiplayerCharacter.h"
#include "UObject/ConstructorHelpers.h"

AGP3_MultiplayerGameMode::AGP3_MultiplayerGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
