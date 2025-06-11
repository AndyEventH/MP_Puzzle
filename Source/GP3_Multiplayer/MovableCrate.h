// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MovableCrate.generated.h"

UCLASS()
class GP3_MULTIPLAYER_API AMovableCrate : public AActor
{
	GENERATED_BODY()

public:
	AMovableCrate();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerBeginDrag(AController* InController);

	UFUNCTION(Server, Unreliable, WithValidation)
	void ServerUpdateDrag(const FVector& TargetLocation);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerEndDrag();

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* Mesh;

	UPROPERTY(Replicated)
	AController* DraggingController = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Drag")
	float MaxDragDistance = 400.f;

	UPROPERTY(EditDefaultsOnly, Category = "Drag")
	float DragInterpSpeed = 8.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float LockedZ = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FTimerHandle GravityTimer;

	void EnableGravity();

	bool bWasSimulating = false;
};
