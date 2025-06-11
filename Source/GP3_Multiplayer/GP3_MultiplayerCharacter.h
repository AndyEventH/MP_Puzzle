// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "MovableCrate.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "GP3_MultiplayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config = Game)
class AGP3_MultiplayerCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	UPROPERTY()
	class APressureButton* CurrentPressedButton = nullptr;

	UPROPERTY()
	TWeakObjectPtr<AMovableCrate> HoldingCrate;
	bool bHoldingCrate = false;

	void TryGrabOrRelease(const FInputActionInstance& Instance);

	void UpdateHeldCrate();

public:
	AGP3_MultiplayerCharacter();

	UFUNCTION(BlueprintCallable, Category = "Size")
	void ChangeCharacterSize(float NewScale);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerPressButton(class APressureButton* Button, bool bPressed);

protected:

	UFUNCTION(Server, Unreliable)
	void Server_UpdateHeadLook(const FRotator& NewLook);

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Aim")
	FRotator HeadLook;
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CharacterScale)
	float CharacterScale = 1.f;

	UFUNCTION()
	void OnRep_CharacterScale();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerChangeCharacterSize(float NewScale);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastChangeCharacterSize(float NewScale);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void OnInteractStarted(const FInputActionValue& Value);

	void OnInteractEnded(const FInputActionValue& Value);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* GrabAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* RestartAction;

	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void OnRestartInput();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRequestLevelRestart();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRequestDragMove(AMovableCrate* Crate, FVector Target);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRequestRelease(AMovableCrate* Crate);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRequestGrab(class AMovableCrate* Crate);

	FTimerHandle RestartHandle;

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// To add mapping context
	virtual void BeginPlay();

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
