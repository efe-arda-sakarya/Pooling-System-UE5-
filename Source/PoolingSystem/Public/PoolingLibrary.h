// Copyright Efe Arda Sakarya. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PoolTypes.h"
#include "PoolingLibrary.generated.h"

class UPoolingSubsystem;

/**
 * The Blueprint-facing surface of the plugin.
 *
 * These are thin wrappers around UPoolingSubsystem so that Blueprint users can right-click
 * anywhere and call "Spawn Pooled Actor" without knowing that a subsystem exists.
 * C++ callers can use the subsystem directly.
 */
UCLASS()
class POOLINGSYSTEM_API UPoolingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Takes an actor out of the pool, places it, and activates it. Creates the pool on first use. */
	UFUNCTION(BlueprintCallable, Category = "Pooling", meta = (WorldContext = "WorldContextObject", Keywords = "get acquire take borrow pool"))
	static AActor* SpawnPooledActor(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform, bool& bSuccess);

	/** Deactivates an actor and puts it back in its pool. Returns false if it did not come from one. */
	UFUNCTION(BlueprintCallable, Category = "Pooling", meta = (WorldContext = "WorldContextObject", Keywords = "return release give back recycle pool"))
	static bool DespawnPooledActor(const UObject* WorldContextObject, AActor* Actor);

	/** Makes sure the pool holds at least Count instances, and applies the overflow settings. */
	UFUNCTION(BlueprintCallable, Category = "Pooling", meta = (WorldContext = "WorldContextObject", Keywords = "warm preallocate reserve fill pool", AdvancedDisplay = "3"))
	static void PrewarmPool(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, int32 Count, EPoolOverflowPolicy OverflowPolicy = EPoolOverflowPolicy::Grow, int32 MaxSize = 0);

	/** Destroys every idle instance of a pool. Instances still in use are left alone. */
	UFUNCTION(BlueprintCallable, Category = "Pooling", meta = (WorldContext = "WorldContextObject", Keywords = "empty destroy free release pool"))
	static void ClearPool(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass);

	/** Current size, in-use count and idle count of a pool. Useful for debug overlays. */
	UFUNCTION(BlueprintPure, Category = "Pooling", meta = (WorldContext = "WorldContextObject", Keywords = "count size debug stats pool"))
	static FPoolStats GetPoolStats(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass);

	/** The pooling subsystem of the world this object belongs to. Rarely needed from Blueprint. */
	UFUNCTION(BlueprintPure, Category = "Pooling", meta = (WorldContext = "WorldContextObject"))
	static UPoolingSubsystem* GetPoolingSubsystem(const UObject* WorldContextObject);
};
