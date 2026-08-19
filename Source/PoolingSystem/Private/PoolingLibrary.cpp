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

void UPoolingLibrary::PrewarmPool(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, int32 Count, EPoolOverflowPolicy OverflowPolicy, int32 MaxSize, int32 PerFrame)
{
	if (UPoolingSubsystem* Subsystem = GetPoolingSubsystem(WorldContextObject))
	{
		Subsystem->PrewarmPool(ActorClass, Count, OverflowPolicy, MaxSize, PerFrame);
	}
}

void UPoolingLibrary::ConfigurePool(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, EPoolOverflowPolicy OverflowPolicy, int32 MaxSize)
{
	if (UPoolingSubsystem* Subsystem = GetPoolingSubsystem(WorldContextObject))
	{
		Subsystem->ConfigurePool(ActorClass, OverflowPolicy, MaxSize);
	}
}

int32 UPoolingLibrary::GetPendingPrewarmCount(const UObject* WorldContextObject)
{
	const UPoolingSubsystem* Subsystem = GetPoolingSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->GetPendingPrewarmCount() : 0;
}

namespace
{
	float GetContextDeltaSeconds(const UObject* WorldContextObject)
	{
		const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
		return World ? World->GetDeltaSeconds() : 0.0f;
	}
}

float UPoolingLibrary::GetInstantFPS(const UObject* WorldContextObject)
{
	const float DeltaSeconds = GetContextDeltaSeconds(WorldContextObject);
	return DeltaSeconds > 0.0f ? 1.0f / DeltaSeconds : 0.0f;
}

float UPoolingLibrary::GetSmoothedFPS(const UObject* WorldContextObject, float PreviousFPS, float InterpSpeed)
{
	const float DeltaSeconds = GetContextDeltaSeconds(WorldContextObject);
	const float InstantFPS = DeltaSeconds > 0.0f ? 1.0f / DeltaSeconds : 0.0f;

	// First call, or a reset: snap instead of easing up from zero.
	if (PreviousFPS <= 0.0f)
	{
		return InstantFPS;
	}

	return FMath::FInterpTo(PreviousFPS, InstantFPS, DeltaSeconds, InterpSpeed);
}

float UPoolingLibrary::GetFrameTimeMilliseconds(const UObject* WorldContextObject)
{
	return GetContextDeltaSeconds(WorldContextObject) * 1000.0f;
}

void UPoolingLibrary::PrewarmFromProfile(const UObject* WorldContextObject, const UPoolProfile* Profile)
{
	if (UPoolingSubsystem* Subsystem = GetPoolingSubsystem(WorldContextObject))
	{
		Subsystem->PrewarmFromProfile(Profile);
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

FPoolStats UPoolingLibrary::GetAllPoolStats(const UObject* WorldContextObject)
{
	const UPoolingSubsystem* Subsystem = GetPoolingSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->GetAllPoolStats() : FPoolStats();
}

void UPoolingLibrary::SetPoolingBypassed(const UObject* WorldContextObject, bool bBypassed)
{
	if (UPoolingSubsystem* Subsystem = GetPoolingSubsystem(WorldContextObject))
	{
		Subsystem->SetPoolingBypassed(bBypassed);
	}
}

bool UPoolingLibrary::TogglePoolingBypassed(const UObject* WorldContextObject)
{
	UPoolingSubsystem* Subsystem = GetPoolingSubsystem(WorldContextObject);
	if (!Subsystem)
	{
		return false;
	}

	const bool bNewState = !Subsystem->IsPoolingBypassed();
	Subsystem->SetPoolingBypassed(bNewState);
	return bNewState;
}

bool UPoolingLibrary::IsPoolingBypassed(const UObject* WorldContextObject)
{
	const UPoolingSubsystem* Subsystem = GetPoolingSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->IsPoolingBypassed() : false;
}
