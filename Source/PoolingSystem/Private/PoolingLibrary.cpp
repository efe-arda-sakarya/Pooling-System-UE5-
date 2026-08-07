// Copyright Efe Arda Sakarya. All Rights Reserved.

#include "PoolingLibrary.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "PoolingSubsystem.h"

UPoolingSubsystem* UPoolingLibrary::GetPoolingSubsystem(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	return World ? World->GetSubsystem<UPoolingSubsystem>() : nullptr;
}

AActor* UPoolingLibrary::SpawnPooledActor(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform, bool& bSuccess)
{
	if (UPoolingSubsystem* Subsystem = GetPoolingSubsystem(WorldContextObject))
	{
		return Subsystem->SpawnPooledActor(ActorClass, SpawnTransform, bSuccess);
	}

	bSuccess = false;
	return nullptr;
}

bool UPoolingLibrary::DespawnPooledActor(const UObject* WorldContextObject, AActor* Actor)
{
	UPoolingSubsystem* Subsystem = GetPoolingSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->DespawnPooledActor(Actor) : false;
}

void UPoolingLibrary::PrewarmPool(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, int32 Count, EPoolOverflowPolicy OverflowPolicy, int32 MaxSize)
{
	if (UPoolingSubsystem* Subsystem = GetPoolingSubsystem(WorldContextObject))
	{
		Subsystem->PrewarmPool(ActorClass, Count, OverflowPolicy, MaxSize);
	}
}

void UPoolingLibrary::ClearPool(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass)
{
	if (UPoolingSubsystem* Subsystem = GetPoolingSubsystem(WorldContextObject))
	{
		Subsystem->ClearPool(ActorClass);
	}
}

FPoolStats UPoolingLibrary::GetPoolStats(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass)
{
	const UPoolingSubsystem* Subsystem = GetPoolingSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->GetPoolStats(ActorClass) : FPoolStats();
}
