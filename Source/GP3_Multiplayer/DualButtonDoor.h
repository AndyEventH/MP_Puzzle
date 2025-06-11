#pragma once

#include "CoreMinimal.h"
#include "PressureButton.h"
#include "GameFramework/Actor.h"
#include "DualButtonDoor.generated.h"

UCLASS()
class GP3_MULTIPLAYER_API ADualButtonDoor : public AActor
{
	GENERATED_BODY()

public:
	ADualButtonDoor();

	UPROPERTY(EditInstanceOnly, Category = "Setup")  APressureButton* ButtonA = nullptr;
	UPROPERTY(EditInstanceOnly, Category = "Setup")  APressureButton* ButtonB = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Door")   UStaticMeshComponent* DoorMesh = nullptr;
	UPROPERTY(VisibleAnywhere, Category = "Door")   USceneComponent* HingeComponent = nullptr;

	UPROPERTY(EditAnywhere, Category = "Door")    float DoorOpenDuration = 1.0f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION() void HandleButtonStateChanged(APressureButton* Button, bool bPressed);
	void UpdateDoor();

	UPROPERTY(ReplicatedUsing = OnRep_DoorState) bool bDoorIsOpen = false;
	UFUNCTION() void OnRep_DoorState();

	UFUNCTION(Server, Reliable, WithValidation) void ServerRequestOpen();
	UFUNCTION(Server, Reliable, WithValidation) void ServerRequestClose();

	void PlayDoor(bool bOpen);
	void OpenDoor();
	void CloseDoor();

private:
	bool bA_Pressed = false;
	bool bB_Pressed = false;

	bool  bIsOpening = false;
	bool  bIsClosing = false;
	float CurrentDoorAlpha = 0.f;

	FRotator ClosedRotation;
	FRotator TargetOpenRotation;

	UPROPERTY(EditAnywhere, Category = "Door") bool bOpenBackward = false;
	UPROPERTY(EditAnywhere, Category = "Door", meta = (EditCondition = "bOpenBackward"))  FRotator BackwardOffset = FRotator(0.f, -90.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Door", meta = (EditCondition = "!bOpenBackward")) FRotator ForwardOffset = FRotator(0.f, 90.f, 0.f);
};
