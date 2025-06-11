// Fill out your copyright notice in the Description page of Project Settings.

#include "MovableCrate.h"

#include "Net/UnrealNetwork.h"

AMovableCrate::AMovableCrate()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	Mesh->SetSimulatePhysics(true);
	Mesh->SetIsReplicated(true);
	Mesh->SetCollisionProfileName("PhysicsActor");
}

void AMovableCrate::BeginPlay()
{
	Super::BeginPlay();
}

bool AMovableCrate::ServerBeginDrag_Validate(AController* InController)
{
	return InController
		&& !DraggingController
		&& InController->GetPawn() != nullptr;
}

void AMovableCrate::ServerBeginDrag_Implementation(AController* InController)
{
	if (DraggingController || !InController) return;

	APawn* Pawn = InController->GetPawn();
	if (!Pawn) return;

	const FVector CrateXY = FVector(GetActorLocation().X, GetActorLocation().Y, 0.f);
	const FVector PawnXY = FVector(Pawn->GetActorLocation().X, Pawn->GetActorLocation().Y, 0.f);

	if (FVector::DistSquared(CrateXY, PawnXY) > FMath::Square(MaxDragDistance))
		return;

	DraggingController = InController;
	SetOwner(InController);
	ForceNetUpdate();

	LockedZ = GetActorLocation().Z;
	bWasSimulating = Mesh->IsSimulatingPhysics();

	Mesh->SetSimulatePhysics(false);
	Mesh->SetEnableGravity(false);
	Mesh->SetConstraintMode(EDOFMode::SixDOF);
	Mesh->SetLinearDamping(6.f);
}

bool AMovableCrate::ServerUpdateDrag_Validate(const FVector&)
{
	return true;
}

void AMovableCrate::ServerUpdateDrag_Implementation(const FVector& TargetLoc)
{
	if (!DraggingController) return;

	FVector Clamped = TargetLoc;
	Clamped.Z = LockedZ;

	const FVector NewLoc = FMath::VInterpTo(
		GetActorLocation(), Clamped,
		GetWorld()->GetDeltaSeconds(), DragInterpSpeed);

	SetActorLocation(NewLoc, true, nullptr, ETeleportType::TeleportPhysics);
}

bool AMovableCrate::ServerEndDrag_Validate()
{
	return DraggingController != nullptr;
}

void AMovableCrate::ServerEndDrag_Implementation()
{
	if (!DraggingController) return;

	DraggingController = nullptr;
	SetOwner(nullptr);

	Mesh->SetConstraintMode(EDOFMode::Default);
	Mesh->SetLinearDamping(0.f);

	const float BlendTime = 0.25f;
	GetWorldTimerManager().SetTimer(
		GravityTimer, this, &AMovableCrate::EnableGravity,
		BlendTime, false);
}

void AMovableCrate::EnableGravity()
{
	Mesh->SetEnableGravity(true);

	if (bWasSimulating)
		Mesh->SetSimulatePhysics(true);
}

void AMovableCrate::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMovableCrate, DraggingController);
}