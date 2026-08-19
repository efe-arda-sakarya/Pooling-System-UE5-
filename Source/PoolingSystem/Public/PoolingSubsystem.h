// Copyright Efe Arda Sakarya. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PoolTypes.h"
#include "PoolingSubsystem.generated.h"

class UPoolProfile;

/**
 * Owns every actor pool in a world.
 *
 * Created automatically with the world and destroyed with it, which matches the lifetime of the
 * actors it holds. Blueprint users normally reach these functions through the Pooling Library
 * nodes instead of getting the subsystem directly.
 */
UCLASS()
class POOLINGSYSTEM_API UPoolingSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	//~ Begin USubsystem interface
	virtual void Deinitialize() override;
	//~ End USubsystem interface

	//~ Begin UWorldSubsystem interface
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	//~ End UWorldSubsystem interface

	/**
	 * Takes an actor out of the pool, places it, and activates it.
	 * Creates the pool on first use, so no setup is required.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pooling", meta = (Keywords = "get acquire take borrow pool"))
	AActor* SpawnPooledActor(TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform, bool& bSuccess);

	/** Deactivates an actor and puts it back in its pool. Returns false if it did not come from one. */
	UFUNCTION(BlueprintCallable, Category = "Pooling", meta = (Keywords = "return release give back recycle pool"))
	bool DespawnPooledActor(AActor* Actor);

	/**
	 * Makes sure the pool holds at least Count instances, and applies the overflow settings.
	 * Calling it twice with the same count does nothing the second time.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pooling", meta = (Keywords = "warm preallocate reserve fill pool", AdvancedDisplay = "2"))
	void PrewarmPool(TSubclassOf<AActor> ActorClass, int32 Count, EPoolOverflowPolicy OverflowPolicy = EPoolOverflowPolicy::Grow, int32 MaxSize = 0, int32 PerFrame = 0);

	/**
	 * Sets a pool's overflow rules without creating anything.
	 * Use it when you want a cap or a Reject policy on a pool you are happy to let grow on demand.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pooling", meta = (Keywords = "policy limit cap reject max configure pool"))
	void ConfigurePool(TSubclassOf<AActor> ActorClass, EPoolOverflowPolicy OverflowPolicy = EPoolOverflowPolicy::Grow, int32 MaxSize = 0);

	/** How many instances are still queued to be created in the background. */
	UFUNCTION(BlueprintPure, Category = "Pooling", meta = (Keywords = "prewarm pending progress loading pool"))
	int32 GetPendingPrewarmCount() const;

	/** Applies every row of a Pool Profile asset. */
	UFUNCTION(BlueprintCallable, Category = "Pooling", meta = (Keywords = "warm preallocate profile data asset pool"))
	void PrewarmFromProfile(const UPoolProfile* Profile);

	/** Destroys every idle instance of a pool. Instances still in use are left alone. */
	UFUNCTION(BlueprintCallable, Category = "Pooling", meta = (Keywords = "empty destroy free release pool"))
	void ClearPool(TSubclassOf<AActor> ActorClass);

	/** Current size, in-use count and idle count of a pool. Useful for debug overlays. */
	UFUNCTION(BlueprintPure, Category = "Pooling", meta = (Keywords = "count size debug stats pool"))
	FPoolStats GetPoolStats(TSubclassOf<AActor> ActorClass) const;

	/** The same three numbers summed across every pool in this world. */
	UFUNCTION(BlueprintPure, Category = "Pooling", meta = (Keywords = "count size debug stats total all pool"))
	FPoolStats GetAllPoolStats() const;

	/** How many distinct pools exist right now. */
	UFUNCTION(BlueprintPure, Category = "Pooling")
	int32 GetPoolCount() const { return Pools.Num(); }

	/**
	 * Turns pooling off without changing any calling code: Spawn goes straight to SpawnActor and
	 * Despawn goes straight to Destroy. Made for measuring what pooling is actually saving you.
	 *
	 * Actors handed out before the switch still return to their pool correctly.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pooling|Benchmark", meta = (Keywords = "bypass disable off benchmark compare pool"))
	void SetPoolingBypassed(bool bBypassed);

	/** Whether pooling is currently bypassed. */
	UFUNCTION(BlueprintPure, Category = "Pooling|Benchmark")
	bool IsPoolingBypassed() const { return bPoolingBypassed; }

private:
	/** Spawns one instance and immediately puts it to sleep. Returns nullptr on failure. */
	AActor* CreatePooledInstance(TSubclassOf<AActor> ActorClass);

	/** Plain SpawnActor at a transform, used by the bypass path and by pool growth. */
	AActor* SpawnRawInstance(TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform);

	/** Wakes an actor up at the given transform and fires On Pool Spawned. */
	void ActivateActor(AActor* Actor, const FTransform& SpawnTransform) const;

	/** Puts an actor to sleep and fires On Pool Despawned. */
	void DeactivateActor(AActor* Actor) const;

	/** Fires On Pool Spawned and starts any Pool Lifetime countdown. */
	void NotifySpawned(AActor* Actor) const;

	/** Fires On Pool Despawned and cancels any Pool Lifetime countdown. */
	void NotifyDespawned(AActor* Actor) const;

	/** One pool per actor class. The class itself is the key, so users never invent pool names. */
	UPROPERTY()
	TMap<TSubclassOf<AActor>, FActorPool> Pools;

	/** Benchmark switch. Transient: never saved, always starts off. */
	UPROPERTY(Transient)
	bool bPoolingBypassed = false;

	/** Creates one frame's worth of every queued prewarm, then reschedules itself. */
	void ProcessPendingPrewarms();

	/** Queues the next-tick callback, unless one is already waiting or there is nothing left. */
	void ScheduleNextPrewarmTick();

	/** Creates instances immediately and warns if it cost a visible hitch. */
	void FillPoolNow(TSubclassOf<AActor> ActorClass, int32 TargetCount);

	/** Pools still being filled a few instances per frame. */
	UPROPERTY(Transient)
	TArray<FPendingPrewarm> PendingPrewarms;

	/** Stops the next-tick callback being queued more than once. */
	bool bPrewarmTickScheduled = false;
};
