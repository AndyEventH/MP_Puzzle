// PressureButton.h
#pragma once

#include "CoreMinimal.h"
#include "GP3_MultiplayerCharacter.h"
#include "GameFramework/Actor.h"
#include "PressureButton.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnButtonStateChanged, APressureButton*, Button, bool, bPressed);

UCLASS()
class GP3_MULTIPLAYER_API APressureButton : public AActor
{
	GENERATED_BODY()

public:
	APressureButton();

	UPROPERTY(VisibleAnywhere, Category = "Button")
	UStaticMeshComponent* ButtonMesh;

	UPROPERTY(EditAnywhere, Category = "Button")
	float PressDuration = 0.2f;

	UPROPERTY(BlueprintAssignable, Category = "Button")
	FOnButtonStateChanged OnButtonStateChanged;

	void SetPressed(bool NewPressed);

	UFUNCTION(BlueprintCallable, Category = "Button")
	APlayerState* GetPressedBy() const { return PressedBy; }

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetPressed(bool NewPressed);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Interaction)
	float MaxInteractDistance = 400.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Interaction)
	float MaxInteractAngle = 8.f;

	bool IsInteractableBy(const AGP3_MultiplayerCharacter* Char) const;

protected:

	UPROPERTY(Replicated) APlayerState* PressedBy = nullptr;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void OnRep_IsPressed();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(ReplicatedUsing = OnRep_IsPressed)
	bool bIsPressed = false;

	bool bIsPressing = false;
	bool bIsReleasing = false;
	float CurrentPressAlpha = 0.0f;

	FVector InitialMeshLocation;
	FVector PressedMeshLocation;
};