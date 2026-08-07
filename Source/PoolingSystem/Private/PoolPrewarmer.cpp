// Copyright Efe Arda Sakarya. All Rights Reserved.

#include "PoolPrewarmer.h"

#include "Engine/World.h"
#include "PoolingSubsystem.h"
#include "PoolingSystem.h"

APoolPrewarmer::APoolPrewarmer()
{
	PrimaryActorTick.bCanEverTick = false;

	// A plain scene component so the actor can be placed and moved in the level like any other.
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void APoolPrewarmer::BeginPlay()
{
	Super::BeginPlay();

	UPoolingSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UPoolingSubsystem>() : nullptr;
	if (!Subsystem)
	{
		return;
	}

	for (const FPoolSpec& Spec : PoolsToPrewarm)
	{
		if (!Spec.ActorClass)
		{
			UE_LOG(LogPoolingSystem, Warning, TEXT("'%s' has a prewarm entry with no actor class set."), *GetName());
			continue;
		}

		Subsystem->PrewarmPool(Spec.ActorClass, Spec.Count, Spec.OverflowPolicy, Spec.MaxSize);
	}
}
