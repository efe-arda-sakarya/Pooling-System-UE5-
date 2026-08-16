// Copyright Efe Arda Sakarya. All Rights Reserved.

#include "PoolingSubsystem.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/MovementComponent.h"
#include "PoolLifetimeComponent.h"
#include "PoolProfile.h"
#include "PoolableInterface.h"
#include "PoolingSystem.h"
#include "TimerManager.h"

void UPoolingSubsystem::Deinitialize()
{
	// The world is going away and it takes every actor with it, so there is nothing to destroy
	// here — only the bookkeeping to drop.
	Pools.Empty();

	Super::Deinitialize();
}

bool UPoolingSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// The engine default also creates world subsystems for the editor world, where pooling has
	// nothing to do. Restrict it to actual gameplay.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UPoolingSubsystem::SetPoolingBypassed(bool bBypassed)
{
	if (bPoolingBypassed == bBypassed)
	{
		return;
	}

	bPoolingBypassed = bBypassed;

	UE_LOG(LogPoolingSystem, Log, TEXT("Pooling is now %s."), bBypassed ? TEXT("bypassed (actors are spawned and destroyed directly)") : TEXT("active"));
}

AActor* UPoolingSubsystem::SpawnPooledActor(TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform, bool& bSuccess)
{
	bSuccess = false;

	if (!ActorClass)
	{
		UE_LOG(LogPoolingSystem, Warning, TEXT("Spawn Pooled Actor was called with no actor class."));
		return nullptr;
	}

	// Benchmark path: behave exactly like a caller that never heard of pooling. The actor still
	// receives On Pool Spawned so gameplay code does not have to branch.
	if (bPoolingBypassed)
	{
		AActor* RawActor = SpawnRawInstance(ActorClass, SpawnTransform);
		if (!RawActor)
		{
			return nullptr;
		}

		NotifySpawned(RawActor);
		bSuccess = true;
		return RawActor;
	}

	// Note: the pool reference is looked up again after anything that could spawn an actor.
	// Spawning runs user code (construction script, BeginPlay) which may call back into the pool
	// and add a map entry, and adding to a TMap can move the existing values in memory.
	AActor* Actor = nullptr;
	{
		FActorPool& Pool = Pools.FindOrAdd(ActorClass);
		while (!Actor && Pool.AvailableActors.Num() > 0)
		{
			// Skip entries that were destroyed behind the pool's back.
			Actor = Pool.AvailableActors.Pop();
			if (!IsValid(Actor))
			{
				Actor = nullptr;
			}
		}
	}

	if (!Actor)
	{
		const FActorPool& Pool = Pools.FindChecked(ActorClass);
		const bool bAtMaxSize = Pool.MaxSize > 0 && Pool.TotalCount() >= Pool.MaxSize;
		const bool bRejects = Pool.OverflowPolicy == EPoolOverflowPolicy::Reject || bAtMaxSize;

		if (!Pool.bOverflowWarningLogged)
		{
			Pools.FindChecked(ActorClass).bOverflowWarningLogged = true;

			UE_LOG(LogPoolingSystem, Warning,
				TEXT("Pool for '%s' ran out of instances (%d in use).%s Consider raising the prewarm count, ")
				TEXT("or check that Despawn Pooled Actor is being called. This is logged once per pool."),
				*ActorClass->GetName(),
				Pool.TotalCount(),
				bRejects ? TEXT(" The request was rejected.") : TEXT(" A new instance was created."));
		}

		if (bRejects)
		{
			return nullptr;
		}

		Actor = CreatePooledInstance(ActorClass);
		if (!Actor)
		{
			return nullptr;
		}
	}

	ActivateActor(Actor, SpawnTransform);
	Pools.FindChecked(ActorClass).ActiveActors.Add(Actor);

	bSuccess = true;
	return Actor;
}

bool UPoolingSubsystem::DespawnPooledActor(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return false;
	}

	FActorPool* Pool = Pools.Find(Actor->GetClass());
	const bool bBelongsToPool = Pool && Pool->ActiveActors.Remove(Actor) > 0;

	// While bypassed, anything that did not come from a pool is simply destroyed. Actors handed
	// out before the switch are still returned properly, so toggling mid-game stays safe.
	if (!bBelongsToPool)
	{
		if (bPoolingBypassed)
		{
			NotifyDespawned(Actor);
			Actor->Destroy();
			return true;
		}

		UE_LOG(LogPoolingSystem, Warning,
			TEXT("Despawn Pooled Actor was called on '%s', which is not currently checked out of a pool. ")
			TEXT("It was either never pooled or has already been despawned."),
			*Actor->GetName());
		return false;
	}

	// Removed from the active list first, so a Despawn call made from inside On Pool Despawned
	// simply fails instead of returning the actor twice.
	DeactivateActor(Actor);
	Pools.FindChecked(Actor->GetClass()).AvailableActors.Add(Actor);

	return true;
}

void UPoolingSubsystem::PrewarmPool(TSubclassOf<AActor> ActorClass, int32 Count, EPoolOverflowPolicy OverflowPolicy, int32 MaxSize)
{
	if (!ActorClass)
	{
		UE_LOG(LogPoolingSystem, Warning, TEXT("Prewarm Pool was called with no actor class."));
		return;
	}

	{
		FActorPool& Pool = Pools.FindOrAdd(ActorClass);
		Pool.OverflowPolicy = OverflowPolicy;
		Pool.MaxSize = MaxSize;
	}

	int32 Created = 0;
	while (Pools.FindChecked(ActorClass).TotalCount() < Count)
	{
		AActor* Actor = CreatePooledInstance(ActorClass);
		if (!Actor)
		{
			break;
		}

		Pools.FindChecked(ActorClass).AvailableActors.Add(Actor);
		++Created;
	}

	UE_LOG(LogPoolingSystem, Log, TEXT("Prewarmed pool for '%s': %d created, %d total."),
		*ActorClass->GetName(), Created, Pools.FindChecked(ActorClass).TotalCount());
}

void UPoolingSubsystem::PrewarmFromProfile(const UPoolProfile* Profile)
{
	if (!Profile)
	{
		UE_LOG(LogPoolingSystem, Warning, TEXT("Prewarm From Profile was called with no profile."));
		return;
	}

	for (const FPoolSpec& Spec : Profile->Pools)
	{
		if (!Spec.ActorClass)
		{
			UE_LOG(LogPoolingSystem, Warning, TEXT("Pool profile '%s' has a row with no actor class set."), *Profile->GetName());
			continue;
		}

		PrewarmPool(Spec.ActorClass, Spec.Count, Spec.OverflowPolicy, Spec.MaxSize);
	}
}

void UPoolingSubsystem::ClearPool(TSubclassOf<AActor> ActorClass)
{
	FActorPool* Pool = ActorClass ? Pools.Find(ActorClass) : nullptr;
	if (!Pool)
	{
		return;
	}

	for (AActor* Actor : Pool->AvailableActors)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
	Pool->AvailableActors.Empty();

	if (Pool->ActiveActors.Num() > 0)
	{
		UE_LOG(LogPoolingSystem, Log,
			TEXT("Cleared pool for '%s'. %d instances are still in use and were kept."),
			*ActorClass->GetName(), Pool->ActiveActors.Num());
	}
	else
	{
		Pools.Remove(ActorClass);
	}
}

FPoolStats UPoolingSubsystem::GetPoolStats(TSubclassOf<AActor> ActorClass) const
{
	FPoolStats Stats;

	if (const FActorPool* Pool = ActorClass ? Pools.Find(ActorClass) : nullptr)
	{
		Stats.Active = Pool->ActiveActors.Num();
		Stats.Available = Pool->AvailableActors.Num();
		Stats.Total = Stats.Active + Stats.Available;
	}

	return Stats;
}

FPoolStats UPoolingSubsystem::GetAllPoolStats() const
{
	FPoolStats Stats;

	for (const TPair<TSubclassOf<AActor>, FActorPool>& Entry : Pools)
	{
		Stats.Active += Entry.Value.ActiveActors.Num();
		Stats.Available += Entry.Value.AvailableActors.Num();
	}
	Stats.Total = Stats.Active + Stats.Available;

	return Stats;
}

AActor* UPoolingSubsystem::SpawnRawInstance(TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	// Pooled instances are created wherever there is room and moved into place later, so they must
	// never be refused or nudged because something is already standing there.
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* Actor = World->SpawnActor<AActor>(ActorClass, SpawnTransform, SpawnParameters);
	if (!Actor)
	{
		UE_LOG(LogPoolingSystem, Warning, TEXT("Failed to spawn an instance of '%s'."), *ActorClass->GetName());
	}

	return Actor;
}

AActor* UPoolingSubsystem::CreatePooledInstance(TSubclassOf<AActor> ActorClass)
{
	AActor* Actor = SpawnRawInstance(ActorClass, FTransform::Identity);
	if (Actor)
	{
		DeactivateActor(Actor);
	}

	return Actor;
}

void UPoolingSubsystem::ActivateActor(AActor* Actor, const FTransform& SpawnTransform) const
{
	// Move first, then switch collision on, so the actor never reports overlaps at the position it
	// was left in by its previous use.
	Actor->SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);

	Actor->SetActorHiddenInGame(false);
	Actor->SetActorEnableCollision(true);
	Actor->SetActorTickEnabled(true);

	NotifySpawned(Actor);
}

void UPoolingSubsystem::DeactivateActor(AActor* Actor) const
{
	NotifyDespawned(Actor);

	Actor->SetActorHiddenInGame(true);
	Actor->SetActorEnableCollision(false);
	Actor->SetActorTickEnabled(false);

	// A sleeping actor must not keep drifting. Physics velocity and movement components are the two
	// ways an actor keeps moving on its own.
	if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
	{
		if (RootPrimitive->IsSimulatingPhysics())
		{
			RootPrimitive->SetPhysicsLinearVelocity(FVector::ZeroVector);
			RootPrimitive->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
		}
	}

	if (UMovementComponent* MovementComponent = Actor->FindComponentByClass<UMovementComponent>())
	{
		MovementComponent->StopMovementImmediately();
	}

	// Timers started during the previous use would otherwise fire on an actor that is back in the
	// pool, or worse, on the next user of that same instance.
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(Actor);
	}
}

void UPoolingSubsystem::NotifySpawned(AActor* Actor) const
{
	if (Actor->GetClass()->ImplementsInterface(UPoolable::StaticClass()))
	{
		IPoolable::Execute_OnPoolSpawned(Actor);
	}

	if (UPoolLifetimeComponent* LifetimeComponent = Actor->FindComponentByClass<UPoolLifetimeComponent>())
	{
		if (LifetimeComponent->bAutoStart)
		{
			LifetimeComponent->StartLifetime();
		}
	}
}

void UPoolingSubsystem::NotifyDespawned(AActor* Actor) const
{
	// Cancelled before the interface event so user code can start its own countdown from inside
	// On Pool Despawned without it being wiped straight afterwards.
	if (UPoolLifetimeComponent* LifetimeComponent = Actor->FindComponentByClass<UPoolLifetimeComponent>())
	{
		LifetimeComponent->CancelLifetime();
	}

	if (Actor->GetClass()->ImplementsInterface(UPoolable::StaticClass()))
	{
		IPoolable::Execute_OnPoolDespawned(Actor);
	}
}
