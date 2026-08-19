// Copyright Efe Arda Sakarya. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PooledSpawner.generated.h"

class UArrowComponent;
class UPoolProfile;

/** How a burst is arranged in space. */
UENUM(BlueprintType)
enum class EPooledSpawnPattern : uint8
{
	/** Along the spawner's forward axis, optionally scattered inside a cone. A turret. */
	Forward		UMETA(DisplayName = "Forward"),

	/** Evenly spaced on a circle around the spawner, all facing the middle. An enemy wave. */
	Radial		UMETA(DisplayName = "Radial")
};

/**
 * Everything about what a spawner emits, in one editable block.
 *
 * Kept as a struct so the numbers live in the Details panel (or in a Blueprint variable) rather
 * than in node graphs — change the demo by typing values, not by rewiring.
 */
USTRUCT(BlueprintType)
struct FPooledSpawnPattern
{
	GENERATED_BODY()

	/** What to spawn. Anything poolable; usually a Pooled Projectile subclass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	TSubclassOf<AActor> ActorClass;

	/** How the burst is arranged. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	EPooledSpawnPattern Pattern = EPooledSpawnPattern::Forward;

	/** Seconds between bursts. Lower means more pressure on the pool. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float Interval = 0.1f;

	/** How many actors go out per burst. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = "1", UIMin = "1"))
	int32 CountPerBurst = 1;

	/** Forward only: half-angle of the cone the direction is scattered in. 0 fires dead straight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = "0.0", ClampMax = "180.0", EditCondition = "Pattern == EPooledSpawnPattern::Forward", EditConditionHides))
	float SpreadDegrees = 3.0f;

	/** Forward only: where the muzzle sits relative to the spawner. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (EditCondition = "Pattern == EPooledSpawnPattern::Forward", EditConditionHides))
	FVector MuzzleOffset = FVector(60.0f, 0.0f, 0.0f);

	/** Radial only: distance from the spawner that actors appear at. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = "0.0", EditCondition = "Pattern == EPooledSpawnPattern::Radial", EditConditionHides))
	float Radius = 800.0f;
};

/**
 * A configurable emitter for stress-testing and demonstrating pooling.
 *
 * Place it, fill in the pattern, press play. Forward makes it a turret; Radial makes it an enemy
 * wave spawner. It always goes through the pooling subsystem, so the benchmark bypass switch
 * applies to it automatically — no branching anywhere in the spawner itself.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Pooled Spawner"))
class POOLINGSYSTEM_API APooledSpawner : public AActor
{
	GENERATED_BODY()

public:
	APooledSpawner();

	/** What, how many, how often and in what shape. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	FPooledSpawnPattern Pattern;

	/** Begin firing as soon as the level starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	bool bAutoStart = true;

	/** Optional pools to prepare on begin play, so the first burst costs nothing extra. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	TObjectPtr<UPoolProfile> PrewarmProfile;

	/** Total actors this spawner has requested since the level started. */
	UPROPERTY(BlueprintReadOnly, Category = "Spawner")
	int32 TotalRequested = 0;

	/** Total requests the pool refused, which only happens under a Reject policy or a Max Size cap. */
	UPROPERTY(BlueprintReadOnly, Category = "Spawner")
	int32 TotalRejected = 0;

	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void StartSpawning();

	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void StopSpawning();

	/** Flips between running and stopped, and returns the new state. */
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	bool ToggleSpawning();

	UFUNCTION(BlueprintPure, Category = "Spawner")
	bool IsSpawning() const;

	/** Emits one burst immediately, whether or not the spawner is running. */
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void FireBurst();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Where and which way the Index-th actor of a burst should appear. */
	FTransform BuildSpawnTransform(int32 Index) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner")
	TObjectPtr<USceneComponent> SceneRoot;

#if WITH_EDITORONLY_DATA
	/** Editor-only aiming aid; shows which way Forward actually points. */
	UPROPERTY()
	TObjectPtr<UArrowComponent> ArrowComponent;
#endif

private:
	FTimerHandle BurstTimer;
};
