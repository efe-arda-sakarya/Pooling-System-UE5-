// Copyright Efe Arda Sakarya. All Rights Reserved.

#include "PooledSpawner.h"

#include "Engine/World.h"
#include "PoolingSubsystem.h"
#include "PoolingSystem.h"
#include "TimerManager.h"

#if WITH_EDITORONLY_DATA
#include "Components/ArrowComponent.h"
#endif

APooledSpawner::APooledSpawner()
{
	// Bursts run off a timer, so there is nothing to do per frame.
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;

#if WITH_EDITORONLY_DATA
	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	if (ArrowComponent)
	{
		ArrowComponent->SetupAttachment(SceneRoot);
		ArrowComponent->ArrowSize = 1.5f;
		ArrowComponent->bIsEditorOnly = true;
	}
#endif
}

void APooledSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (PrewarmProfile)
	{
		if (UPoolingSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UPoolingSubsystem>() : nullptr)
		{
			Subsystem->PrewarmFromProfile(PrewarmProfile);
		}
	}

	if (bAutoStart)
	{
		StartSpawning();
	}
}

void APooledSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopSpawning();

	Super::EndPlay(EndPlayReason);
}

void APooledSpawner::StartSpawning()
{
	UWorld* World = GetWorld();
	if (!World || IsSpawning())
	{
		return;
	}

	if (!Pattern.ActorClass)
	{
		UE_LOG(LogPoolingSystem, Warning, TEXT("'%s' cannot start: its pattern has no actor class."), *GetName());
		return;
	}

	World->GetTimerManager().SetTimer(BurstTimer, this, &APooledSpawner::FireBurst, Pattern.Interval, true);
}

void APooledSpawner::StopSpawning()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BurstTimer);
	}
}

bool APooledSpawner::ToggleSpawning()
{
	if (IsSpawning())
	{
		StopSpawning();
		return false;
	}

	StartSpawning();
	return IsSpawning();
}

bool APooledSpawner::IsSpawning() const
{
	const UWorld* World = GetWorld();
	return World && World->GetTimerManager().IsTimerActive(BurstTimer);
}

void APooledSpawner::FireBurst()
{
	UPoolingSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UPoolingSubsystem>() : nullptr;
	if (!Subsystem || !Pattern.ActorClass)
	{
		return;
	}

	for (int32 Index = 0; Index < Pattern.CountPerBurst; ++Index)
	{
		bool bSuccess = false;
		Subsystem->SpawnPooledActor(Pattern.ActorClass, BuildSpawnTransform(Index), bSuccess);

		++TotalRequested;
		if (!bSuccess)
		{
			++TotalRejected;
		}
	}
}

FTransform APooledSpawner::BuildSpawnTransform(int32 Index) const
{
	const FTransform& ActorTransform = GetActorTransform();

	if (Pattern.Pattern == EPooledSpawnPattern::Radial)
	{
		// Spread the burst evenly around the circle and turn each actor to face the middle, which
		// is what an enemy wave closing in on the player looks like.
		const float StepDegrees = 360.0f / FMath::Max(1, Pattern.CountPerBurst);
		const FVector Offset = FRotator(0.0f, StepDegrees * Index, 0.0f).Vector() * Pattern.Radius;
		const FVector Location = ActorTransform.GetLocation() + Offset;

		return FTransform((-Offset).Rotation(), Location);
	}

	// Forward.
	const FVector Location = ActorTransform.TransformPosition(Pattern.MuzzleOffset);
	const FVector Direction = Pattern.SpreadDegrees > 0.0f
		? FMath::VRandCone(ActorTransform.GetUnitAxis(EAxis::X), FMath::DegreesToRadians(Pattern.SpreadDegrees))
		: ActorTransform.GetUnitAxis(EAxis::X);

	return FTransform(Direction.Rotation(), Location);
}
