// Copyright Efe Arda Sakarya. All Rights Reserved.

#include "PoolLifetimeComponent.h"

#include "Engine/World.h"
#include "PoolingSubsystem.h"
#include "PoolingSystem.h"
#include "TimerManager.h"

UPoolLifetimeComponent::UPoolLifetimeComponent()
{
	// Everything here runs off a timer, so the component never needs to tick.
	PrimaryComponentTick.bCanEverTick = false;
}

void UPoolLifetimeComponent::StartLifetime()
{
	CancelLifetime();

	UWorld* World = GetWorld();
	if (!World || Lifetime <= 0.0f)
	{
		return;
	}

	World->GetTimerManager().SetTimer(LifetimeTimer, this, &UPoolLifetimeComponent::HandleLifetimeExpired, Lifetime, false);
}

void UPoolLifetimeComponent::CancelLifetime()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LifetimeTimer);
	}
}

float UPoolLifetimeComponent::GetRemainingLifetime() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}

	const float Remaining = World->GetTimerManager().GetTimerRemaining(LifetimeTimer);
	return Remaining > 0.0f ? Remaining : 0.0f;
}

void UPoolLifetimeComponent::HandleLifetimeExpired()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!IsValid(Owner) || !World)
	{
		return;
	}

	if (UPoolingSubsystem* Subsystem = World->GetSubsystem<UPoolingSubsystem>())
	{
		Subsystem->DespawnPooledActor(Owner);
	}
	else
	{
		UE_LOG(LogPoolingSystem, Warning, TEXT("'%s' has a Pool Lifetime component but there is no pooling subsystem in this world."), *Owner->GetName());
	}
}
