// Copyright Epic Games, Inc. All Rights Reserved.

#include "GP3_MultiplayerCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "PressureButton.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AGP3_MultiplayerCharacter

AGP3_MultiplayerCharacter::AGP3_MultiplayerCharacter()
{
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 0.f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom);
	FollowCamera->bUsePawnControlRotation = true;
	FollowCamera->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight));

	bReplicates = true;
	SetReplicateMovement(true);
	CharacterScale = 1.f;
}

void AGP3_MultiplayerCharacter::BeginPlay()
{
	// Call the base class
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void AGP3_MultiplayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGP3_MultiplayerCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AGP3_MultiplayerCharacter::Look);

		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AGP3_MultiplayerCharacter::OnInteractStarted);
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Completed, this, &AGP3_MultiplayerCharacter::OnInteractEnded);

		EnhancedInputComponent->BindAction(GrabAction, ETriggerEvent::Started,
			this, &AGP3_MultiplayerCharacter::TryGrabOrRelease);

		EnhancedInputComponent->BindAction(GrabAction, ETriggerEvent::Completed,
			this, &AGP3_MultiplayerCharacter::TryGrabOrRelease);

		EnhancedInputComponent->BindAction(
			RestartAction,
			ETriggerEvent::Started,
			this, &AGP3_MultiplayerCharacter::OnRestartInput);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AGP3_MultiplayerCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AGP3_MultiplayerCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AGP3_MultiplayerCharacter::ChangeCharacterSize(float NewScale)
{
	NewScale = FMath::Clamp(NewScale, 0.2f, 1.0f);

	if (HasAuthority())
	{
		CharacterScale = NewScale;
		MulticastChangeCharacterSize(NewScale);
	}
	else
	{
		ServerChangeCharacterSize(NewScale);
	}

	SetActorScale3D(FVector(NewScale));

	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = 0.f;
	}
}

bool AGP3_MultiplayerCharacter::ServerChangeCharacterSize_Validate(float NewScale)
{
	return NewScale >= 0.2f && NewScale <= 1.0f;
}

void AGP3_MultiplayerCharacter::ServerChangeCharacterSize_Implementation(float NewScale)
{
	ChangeCharacterSize(NewScale);
}

void AGP3_MultiplayerCharacter::MulticastChangeCharacterSize_Implementation(float NewScale)
{
	SetActorScale3D(FVector(NewScale));

	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = 0.f;
	}
}

void AGP3_MultiplayerCharacter::OnRep_CharacterScale()
{
	SetActorScale3D(FVector(CharacterScale));

	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = 0.f;
	}
}

void AGP3_MultiplayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGP3_MultiplayerCharacter, CharacterScale);
	DOREPLIFETIME_CONDITION(
		AGP3_MultiplayerCharacter,
		HeadLook,
		COND_SkipOwner);
}

void AGP3_MultiplayerCharacter::OnInteractStarted(const FInputActionValue& Value)
{
	FVector Start = FollowCamera->GetComponentLocation();
	FVector End = Start + FollowCamera->GetForwardVector() * 400.f;
	const float SphereRadius = 30.f;

	FHitResult Hit;
	FCollisionQueryParams Params(NAME_None, false, this);
	bool bHit = GetWorld()->SweepSingleByChannel(
		Hit,
		Start, End,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(SphereRadius),
		Params
	);

	DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 1.0f);
	DrawDebugSphere(GetWorld(), End, SphereRadius, 12, FColor::Green, false, 1.0f);

	if (bHit && Hit.GetActor())
	{
		if (APressureButton* Button = Cast<APressureButton>(Hit.GetActor()))
		{
			UE_LOG(LogTemplateCharacter, Log, TEXT("InteractStarted hit %s"), *Button->GetName());
			if (GEngine)
				GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Green,
					FString::Printf(TEXT("Pressing %s"), *Button->GetName()));

			if (Button->IsInteractableBy(this))
			{
				CurrentPressedButton = Button;
				ServerPressButton(Button, true);
			}
			else
			{
				ServerPressButton(Button, false);
			}
			return;
		}
	}

	UE_LOG(LogTemplateCharacter, Verbose, TEXT("InteractStarted nothing hit"));
}

void AGP3_MultiplayerCharacter::OnInteractEnded(const FInputActionValue& Value)
{
	if (CurrentPressedButton)
	{
		UE_LOG(LogTemplateCharacter, Log, TEXT("InteractEnded releasing %s"), *CurrentPressedButton->GetName());
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Red,
				FString::Printf(TEXT("Releasing %s"), *CurrentPressedButton->GetName()));

		ServerPressButton(CurrentPressedButton, false);

		CurrentPressedButton = nullptr;
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Verbose, TEXT("InteractEnded no button to release"));
	}
}

bool AGP3_MultiplayerCharacter::ServerPressButton_Validate(APressureButton* Button, bool bPressed)
{
	if (Button == nullptr)
		return false;

	return true;
}

void AGP3_MultiplayerCharacter::ServerPressButton_Implementation(APressureButton* Button, bool bPressed)
{
	UE_LOG(LogTemplateCharacter, Warning, TEXT("[%s][Server] ServerPressButton_Implementation: %s %s"),
		*GetName(),
		*GetNameSafe(Button),
		bPressed ? TEXT(" Press") : TEXT(" Release")
	);

	if (!Button) return;

	Button->SetOwner(GetController());

	Button->SetPressed(bPressed);
}

void AGP3_MultiplayerCharacter::Tick(float DeltaTime)
{
	if (IsLocallyControlled())
	{
		const FRotator Control = Controller->GetControlRotation();
		const FRotator Body = GetActorRotation();
		const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(
			Control, Body);

		FRotator NewLook;
		NewLook.Yaw = FMath::ClampAngle(-Delta.Pitch, -60.f, 60.f);

		HeadLook = NewLook;

		if (!HasAuthority())
			Server_UpdateHeadLook(NewLook);
	}

	Super::Tick(DeltaTime);
	if (CurrentPressedButton && !CurrentPressedButton->IsInteractableBy(this))
	{
		ServerPressButton(CurrentPressedButton, false);
		CurrentPressedButton = nullptr;
	}

	UpdateHeldCrate();
}

void AGP3_MultiplayerCharacter::TryGrabOrRelease(const FInputActionInstance& Instance)
{
	const bool bPressed = Instance.GetTriggerEvent() == ETriggerEvent::Started;

	if (bPressed)
	{
		if (bHoldingCrate) return;

		FHitResult Hit;
		const FVector Start = FollowCamera->GetComponentLocation();
		const FVector End = Start + FollowCamera->GetForwardVector() * 800.f;

		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility))
		{
			if (AMovableCrate* Crate = Cast<AMovableCrate>(Hit.GetActor()))
			{
				ServerRequestGrab(Crate);
				HoldingCrate = Crate;
				bHoldingCrate = true;
			}
		}
	}
	else
	{
		if (bHoldingCrate && HoldingCrate.IsValid())
		{
			ServerRequestRelease(HoldingCrate.Get());
			HoldingCrate = nullptr;
			bHoldingCrate = false;
		}
	}
}

void AGP3_MultiplayerCharacter::UpdateHeldCrate()
{
	if (bHoldingCrate && HoldingCrate.IsValid()
		&& HoldingCrate->GetOwner() == GetController())
	{
		const FVector Target = GetActorLocation()
			+ GetFollowCamera()->GetForwardVector() * 200.f;

		ServerRequestDragMove(HoldingCrate.Get(), Target);
	}
}

void AGP3_MultiplayerCharacter::OnRestartInput()
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
			FString::Printf(TEXT("Client %s pressed <R>"), *GetNameSafe(this)));

	if (HasAuthority())
	{
		UE_LOG(LogTemplateCharacter, Log,
			TEXT("[%s] I am the server – restarting level"), *GetName());
		ServerRequestLevelRestart_Implementation();
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Log,
			TEXT("[%s] Sending restart request to server"), *GetName());
		ServerRequestLevelRestart();
	}
}

bool AGP3_MultiplayerCharacter::ServerRequestLevelRestart_Validate() { return true; }

void AGP3_MultiplayerCharacter::ServerRequestLevelRestart_Implementation()
{
	UWorld* World = GetWorld();
	if (!World)
		return;

	if (World->GetTimerManager().IsTimerActive(RestartHandle))
	{
		UE_LOG(LogTemplateCharacter, Warning,
			TEXT("Restart ignored: request already queued"));
		return;
	}

	const FString LevelName =
		UGameplayStatics::GetCurrentLevelName(World, true);

	UE_LOG(LogTemplateCharacter, Log,
		TEXT("Server travelling to %s"), *LevelName);

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
			FString::Printf(TEXT("SERVER: restarting %s"), *LevelName));

	World->GetTimerManager().SetTimer(
		RestartHandle,
		[World, LevelName]()
		{
			World->ServerTravel(LevelName + TEXT("?listen"));
		},
		0.10f,
		false);
}
bool AGP3_MultiplayerCharacter::ServerRequestDragMove_Validate(
	AMovableCrate* Crate, FVector)
{
	return Crate != nullptr;
}
void AGP3_MultiplayerCharacter::ServerRequestDragMove_Implementation(
	AMovableCrate* Crate, FVector Target)
{
	Crate->ServerUpdateDrag_Implementation(Target);
}

bool AGP3_MultiplayerCharacter::ServerRequestRelease_Validate(
	AMovableCrate* Crate)
{
	return Crate != nullptr;
}
void AGP3_MultiplayerCharacter::ServerRequestRelease_Implementation(
	AMovableCrate* Crate)
{
	Crate->ServerEndDrag_Implementation();
}

bool AGP3_MultiplayerCharacter::ServerRequestGrab_Validate(AMovableCrate* Crate)
{
	return Crate != nullptr;
}

void AGP3_MultiplayerCharacter::ServerRequestGrab_Implementation(AMovableCrate* Crate)
{
	Crate->ServerBeginDrag_Implementation(GetController());
}

void AGP3_MultiplayerCharacter::Server_UpdateHeadLook_Implementation(
	const FRotator& NewLook)
{
	HeadLook = NewLook;
}