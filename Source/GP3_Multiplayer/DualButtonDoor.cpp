#include "DualButtonDoor.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerState.h"

ADualButtonDoor::ADualButtonDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	HingeComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));
	RootComponent = HingeComponent;

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(HingeComponent);
}

void ADualButtonDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADualButtonDoor, bDoorIsOpen);
}

void ADualButtonDoor::BeginPlay()
{
	Super::BeginPlay();

	ClosedRotation = DoorMesh->GetRelativeRotation();
	TargetOpenRotation = ClosedRotation + (bOpenBackward ? BackwardOffset : ForwardOffset);

	if (!ButtonA || !ButtonB)
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] ButtonA or ButtonB is null!"), *GetName());
		return;
	}

	if (HasAuthority())
	{
		ButtonA->OnButtonStateChanged.AddDynamic(this, &ADualButtonDoor::HandleButtonStateChanged);
		ButtonB->OnButtonStateChanged.AddDynamic(this, &ADualButtonDoor::HandleButtonStateChanged);
	}
	else
	{
		PlayDoor(bDoorIsOpen);
	}
}

void ADualButtonDoor::HandleButtonStateChanged(APressureButton* Button, bool bPressed)
{
	if (Button == ButtonA)      bA_Pressed = bPressed;
	else if (Button == ButtonB) bB_Pressed = bPressed;
	UpdateDoor();
}

void ADualButtonDoor::UpdateDoor()
{
	if (!HasAuthority()) return;

	if (bA_Pressed && bB_Pressed)
	{
		auto* A = ButtonA->GetPressedBy();
		auto* B = ButtonB->GetPressedBy();

		if (A) UE_LOG(LogTemp, Log, TEXT("ButtonA pressed by %s"), *A->GetPlayerName());
		if (B) UE_LOG(LogTemp, Log, TEXT("ButtonB pressed by %s"), *B->GetPlayerName());

		if (A && B && A != B)
			OpenDoor();
		/*else
			CloseDoor();*/
	}
	/*else
	{
		CloseDoor();
	}*/
}

void ADualButtonDoor::OpenDoor()
{
	if (bDoorIsOpen) return;
	bDoorIsOpen = true;
	PlayDoor(true);
}

void ADualButtonDoor::CloseDoor()
{
	if (!bDoorIsOpen) return;
	bDoorIsOpen = false;
	PlayDoor(false);
}

bool ADualButtonDoor::ServerRequestOpen_Validate()
{
	return !bDoorIsOpen;//&& bA_Pressed&& bB_Pressed;
}
void ADualButtonDoor::ServerRequestOpen_Implementation() { OpenDoor(); }

bool ADualButtonDoor::ServerRequestClose_Validate()
{
	return  bDoorIsOpen; //&& (!bA_Pressed || !bB_Pressed);
}
void ADualButtonDoor::ServerRequestClose_Implementation() { CloseDoor(); }

void ADualButtonDoor::OnRep_DoorState() { PlayDoor(bDoorIsOpen); }

void ADualButtonDoor::PlayDoor(bool bOpen)
{
	bIsOpening = bOpen;
	bIsClosing = !bOpen;

	const FRotator CurrentRot = DoorMesh->GetRelativeRotation();
	const float    TotalAngle = (TargetOpenRotation - ClosedRotation).GetDenormalized().Yaw;
	const float    CurrentAngle = (CurrentRot - ClosedRotation).GetDenormalized().Yaw;

	if (FMath::IsNearlyZero(TotalAngle))
	{
		CurrentDoorAlpha = 0.f;
		return;
	}

	CurrentDoorAlpha = FMath::Clamp(CurrentAngle / TotalAngle, 0.f, 1.f);
}

void ADualButtonDoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsOpening)
	{
		CurrentDoorAlpha += DeltaTime / DoorOpenDuration;
		if (CurrentDoorAlpha >= 1.f)
		{
			CurrentDoorAlpha = 1.f;
			bIsOpening = false;
		}
	}
	else if (bIsClosing)
	{
		CurrentDoorAlpha -= DeltaTime / DoorOpenDuration;
		if (CurrentDoorAlpha <= 0.f)
		{
			CurrentDoorAlpha = 0.f;
			bIsClosing = false;
		}
	}
	else
	{
		return;
	}

	const FRotator NewRot = FMath::Lerp(ClosedRotation, TargetOpenRotation, CurrentDoorAlpha);
	DoorMesh->SetRelativeRotation(NewRot);
}