// PressureButton.cpp
#include "PressureButton.h"

#include "GP3_MultiplayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "UObject/NameTypes.h"

APressureButton::APressureButton()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	ButtonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ButtonMesh"));
	RootComponent = ButtonMesh;
	ButtonMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ButtonMesh->SetCollisionObjectType(ECC_WorldDynamic);
	ButtonMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	ButtonMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	ButtonMesh->SetMobility(EComponentMobility::Movable);
}

void APressureButton::BeginPlay()
{
	Super::BeginPlay();

	InitialMeshLocation = ButtonMesh->GetRelativeLocation();
	PressedMeshLocation = InitialMeshLocation + (-InitialMeshLocation.RightVector * 30.f);
}

void APressureButton::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsPressing)
	{
		CurrentPressAlpha += DeltaTime / PressDuration;
		if (CurrentPressAlpha >= 1.f)
		{
			CurrentPressAlpha = 1.f;
			bIsPressing = false;
		}
		FVector NewLoc = FMath::Lerp(InitialMeshLocation, PressedMeshLocation, CurrentPressAlpha);
		ButtonMesh->SetRelativeLocation(NewLoc);
	}
	else if (bIsReleasing)
	{
		CurrentPressAlpha -= DeltaTime / PressDuration;
		if (CurrentPressAlpha <= 0.f)
		{
			CurrentPressAlpha = 0.f;
			bIsReleasing = false;
		}
		FVector NewLoc = FMath::Lerp(InitialMeshLocation, PressedMeshLocation, CurrentPressAlpha);
		ButtonMesh->SetRelativeLocation(NewLoc);
	}
}

void APressureButton::SetPressed(bool NewPressed)
{
	if (bIsPressed == NewPressed)
		return;

	if (!HasAuthority())
	{
		APawn* Pawn = GetWorld()->GetFirstPlayerController()->GetPawn();
		if (AGP3_MultiplayerCharacter* Char = Cast<AGP3_MultiplayerCharacter>(Pawn))
		{
			Char->ServerPressButton(this, NewPressed);
		}
		return;
	}

	ServerSetPressed(NewPressed);
}

bool APressureButton::ServerSetPressed_Validate(bool NewPressed)
{
	//return NewPressed ? !bIsPressed : bIsPressed;
	return true;
}

void APressureButton::ServerSetPressed_Implementation(bool NewPressed)
{
	UE_LOG(LogTemp, Warning,
		TEXT("ServerSetPressed | NewPressed=%s"),
		NewPressed ? TEXT("true") : TEXT("false"));

	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC)
	{
		UE_LOG(LogTemp, Error,
			TEXT("Owner is not a PlayerController! Owner=%s"),
			GetOwner()
			? *GetOwner()->GetName()
			: TEXT("None"));
		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("Controller: %s"),
		*PC->GetName());

	APlayerState* CallerPS = PC->PlayerState;
	UE_LOG(LogTemp, Warning,
		TEXT("Caller PlayerState: %s"),
		CallerPS
		? *CallerPS->GetPlayerName()
		: TEXT("None"));

	if (NewPressed)
	{
		PressedBy = CallerPS;
		UE_LOG(LogTemp, Warning,
			TEXT("Button pressed by: %s"),
			PressedBy
			? *PressedBy->GetPlayerName()
			: TEXT("None"));
	}
	else if (PressedBy == CallerPS)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Button released by: %s"),
			PressedBy
			? *PressedBy->GetPlayerName()
			: TEXT("None"));
		PressedBy = nullptr;
	}

	bIsPressed = NewPressed;
	UE_LOG(LogTemp, Warning,
		TEXT("bIsPressed set to: %s"),
		bIsPressed ? TEXT("true") : TEXT("false"));

	OnRep_IsPressed();
}

void APressureButton::OnRep_IsPressed()
{
	OnButtonStateChanged.Broadcast(this, bIsPressed);

	if (bIsPressed)
	{
		bIsPressing = true;
		bIsReleasing = false;
		CurrentPressAlpha = 0.f;
	}
	else
	{
		bIsReleasing = true;
		bIsPressing = false;
		CurrentPressAlpha = 1.f;
	}
}

void APressureButton::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APressureButton, bIsPressed);
	DOREPLIFETIME(APressureButton, PressedBy);
}

bool APressureButton::IsInteractableBy(const AGP3_MultiplayerCharacter* Char) const
{
	if (!Char) return false;

	const UCameraComponent* Cam = Char->GetFollowCamera();
	if (!Cam) return false;

	const float DistSq = FVector::DistSquared(Cam->GetComponentLocation(), GetActorLocation());
	if (DistSq > FMath::Square(MaxInteractDistance))
		return false;

	const FVector ToBtn = (GetActorLocation() - Cam->GetComponentLocation()).GetSafeNormal();
	const float Dot = FVector::DotProduct(Cam->GetForwardVector(), ToBtn);
	const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(Dot));
	return AngleDeg <= MaxInteractAngle;
}