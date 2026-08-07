// Copyright Efe Arda Sakarya. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PoolTypes.h"
#include "PoolingSubsystem.generated.h"

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
	void PrewarmPool(TSubclassOf<AActor> ActorClass, int32 Count, EPoolOverflowPolicy OverflowPolicy = EPoolOverflowPolicy::Grow, int32 MaxSize = 0);

	/** Destroys every idle instance of a pool. Instances still in use are left alone. */
	UFUNCTION(BlueprintCallable, Category = "Pooling", meta = (Keywords = "empty destroy free release pool"))
	void ClearPool(TSubclassOf<AActor> ActorClass);

	/** Current size, in-use count and idle count of a pool. Useful for debug overlays. */
	UFUNCTION(BlueprintPure, Category = "Pooling", meta = (Keywords = "count size debug stats pool"))
	FPoolStats GetPoolStats(TSubclassOf<AActor> ActorClass) const;

private:
	/** Spawns one instance and immediately puts it to sleep. Returns nullptr on failure. */
	AActor* CreatePooledInstance(TSubclassOf<AActor> ActorClass);

	/** Wakes an actor up at the given transform and fires On Pool Spawned. */
	void ActivateActor(AActor* Actor, const FTransform& SpawnTransform) const;

	/** Puts an actor to sleep and fires On Pool Despawned. */
	void DeactivateActor(AActor* Actor) const;

	/** One pool per actor class. The class itself is the key, so users never invent pool names. */
	UPROPERTY()
	TMap<TSubclassOf<AActor>, FActorPool> Pools;
};
