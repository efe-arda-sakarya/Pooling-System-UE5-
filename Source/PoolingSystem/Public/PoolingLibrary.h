// Copyright Efe Arda Sakarya. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PoolTypes.h"
#include "PoolingLibrary.generated.h"

class UPoolProfile;
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
	static void PrewarmPool(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, int32 Count, EPoolOverflowPolicy OverflowPolicy = EPoolOverflowPolicy::Grow, int32 MaxSize = 0, int32 PerFrame = 0);

	/** Sets a pool's overflow rules without creating anything. */
	UFUNCTION(BlueprintCallable, Category = "Pooling", meta = (WorldContext = "WorldContextObject", Keywords = "policy limit cap reject max configure pool"))
	static void ConfigurePool(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, EPoolOverflowPolicy OverflowPolicy = EPoolOverflowPolicy::Grow, int32 MaxSize = 0);

	/** How many instances are still queued to be created in the background. 0 when prewarming is done. */
	UFUNCTION(BlueprintPure, Category = "Pooling", meta = (WorldContext = "WorldContextObject", Keywords = "prewarm pending progress loading pool"))
	static int32 GetPendingPrewarmCount(const UObject* WorldContextObject);

	/** Applies every row of a Pool Profile asset. */
	UFUNCTION(BlueprintCallable, Category = "Pooling", meta = (WorldContext = "WorldContextObject", Keywords = "warm preallocate profile data asset pool"))
	static void PrewarmFromProfile(const UObject* WorldContextObject, const UPoolProfile* Profile);

	/** Destroys every idle instance of a pool. Instances still in use are left alone. */
	UFUNCTION(BlueprintCallable, Category = "Pooling", meta = (WorldContext = "WorldContextObject", Keywords = "empty destroy free release pool"))
	static void ClearPool(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass);

	/** Current size, in-use count and idle count of a pool. Useful for debug overlays. */
	UFUNCTION(BlueprintPure, Category = "Pooling", meta = (WorldContext = "WorldContextObject", Keywords = "count size debug stats pool"))
	static FPoolStats GetPoolStats(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass);

	/** The same three numbers summed across every pool in this world. */
	UFUNCTION(BlueprintPure, Category = "Pooling", meta = (WorldContext = "WorldContextObject", Keywords = "count size debug stats total all pool"))
	static FPoolStats GetAllPoolStats(const UObject* WorldContextObject);

	/**
	 * Turns pooling off without changing any calling code, so you can measure the difference.
	 * Spawn becomes a plain SpawnActor and Despawn becomes a plain Destroy.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pooling|Benchmark", meta = (WorldContext = "WorldContextObject", Keywords = "bypass disable off benchmark compare pool"))
	static void SetPoolingBypassed(const UObject* WorldContextObject, bool bBypassed);

	/** Flips the benchmark switch and returns the new state. */
	UFUNCTION(BlueprintCallable, Category = "Pooling|Benchmark", meta = (WorldContext = "WorldContextObject", Keywords = "toggle bypass benchmark compare pool"))
	static bool TogglePoolingBypassed(const UObject* WorldContextObject);

	/** Whether pooling is currently bypassed. */
	UFUNCTION(BlueprintPure, Category = "Pooling|Benchmark", meta = (WorldContext = "WorldContextObject"))
	static bool IsPoolingBypassed(const UObject* WorldContextObject);

	/** Frames per second for this frame alone. Jumps around; fine for a raw readout. */
	UFUNCTION(BlueprintPure, Category = "Pooling|Benchmark", meta = (WorldContext = "WorldContextObject", Keywords = "fps framerate performance"))
	static float GetInstantFPS(const UObject* WorldContextObject);

	/**
	 * Eases the previous reading towards this frame's, which is what you want on screen.
	 * Feed the result back in as Previous FPS next frame.
	 */
	UFUNCTION(BlueprintPure, Category = "Pooling|Benchmark", meta = (WorldContext = "WorldContextObject", Keywords = "fps framerate smooth average performance"))
	static float GetSmoothedFPS(const UObject* WorldContextObject, float PreviousFPS, float InterpSpeed = 3.0f);

	/** Milliseconds spent on this frame. The number that actually matters for a comparison. */
	UFUNCTION(BlueprintPure, Category = "Pooling|Benchmark", meta = (WorldContext = "WorldContextObject", Keywords = "frame time ms performance"))
	static float GetFrameTimeMilliseconds(const UObject* WorldContextObject);

	/** The pooling subsystem of the world this object belongs to. Rarely needed from Blueprint. */
	UFUNCTION(BlueprintPure, Category = "Pooling", meta = (WorldContext = "WorldContextObject"))
	static UPoolingSubsystem* GetPoolingSubsystem(const UObject* WorldContextObject);
};
