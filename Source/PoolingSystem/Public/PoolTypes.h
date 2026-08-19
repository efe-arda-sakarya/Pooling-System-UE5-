// Copyright Efe Arda Sakarya. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Templates/SubclassOf.h"
#include "PoolTypes.generated.h"

/** What a pool does when every instance is already in use. */
UENUM(BlueprintType)
enum class EPoolOverflowPolicy : uint8
{
	/** Create a new instance. The pool never fails, but it can keep growing. */
	Grow	UMETA(DisplayName = "Grow"),

	/** Return nothing. Memory stays fixed, callers must handle the failure. */
	Reject	UMETA(DisplayName = "Reject")
};

/** One row of a Pool Prewarmer's list: which class, how many, and how it behaves when full. */
USTRUCT(BlueprintType)
struct FPoolSpec
{
	GENERATED_BODY()

	/** Actor class to pool. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling")
	TSubclassOf<AActor> ActorClass;

	/** How many instances to have ready before they are needed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling", meta = (ClampMin = "0", UIMin = "0"))
	int32 Count = 16;

	/** What happens when every instance is in use. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling")
	EPoolOverflowPolicy OverflowPolicy = EPoolOverflowPolicy::Grow;

	/** Upper bound on total instances. 0 means unlimited. Only meaningful with the Grow policy. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling", meta = (ClampMin = "0", UIMin = "0"))
	int32 MaxSize = 0;

	/**
	 * Spreads the prewarm over several frames instead of building the pool in one go.
	 * 0 creates everything immediately, which is fine for small pools and stalls the level
	 * for large ones. A few hundred per frame keeps the load screen moving.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling", meta = (ClampMin = "0", UIMin = "0", DisplayName = "Per Frame"))
	int32 PerFrame = 0;
};

/** A snapshot of one pool, for debug overlays and demo scenes. */
USTRUCT(BlueprintType)
struct FPoolStats
{
	GENERATED_BODY()

	/** Instances that exist right now (active + available). */
	UPROPERTY(BlueprintReadOnly, Category = "Pooling")
	int32 Total = 0;

	/** Instances currently handed out to callers. */
	UPROPERTY(BlueprintReadOnly, Category = "Pooling")
	int32 Active = 0;

	/** Instances sitting idle, ready to be handed out. */
	UPROPERTY(BlueprintReadOnly, Category = "Pooling")
	int32 Available = 0;
};

/**
 * Runtime state of a single pool. Internal — not exposed to Blueprint.
 * Kept as a USTRUCT so the actor references below are visible to the garbage collector.
 */
USTRUCT()
struct FActorPool
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<AActor>> AvailableActors;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> ActiveActors;

	UPROPERTY()
	EPoolOverflowPolicy OverflowPolicy = EPoolOverflowPolicy::Grow;

	UPROPERTY()
	int32 MaxSize = 0;

	/** Overflow is logged once per pool, not once per frame. */
	UPROPERTY()
	bool bOverflowWarningLogged = false;

	/**
	 * True once someone stated an intent for this pool by prewarming or configuring it.
	 * A pool that was never set up is expected to grow on demand, so growth is not worth a warning;
	 * a pool that was given a size and outgrew it anyway is.
	 */
	UPROPERTY()
	bool bWasConfigured = false;

	int32 TotalCount() const { return AvailableActors.Num() + ActiveActors.Num(); }
};

/** One pool still being filled in the background, a few instances per frame. */
USTRUCT()
struct FPendingPrewarm
{
	GENERATED_BODY()

	UPROPERTY()
	TSubclassOf<AActor> ActorClass;

	/** Instances still to create. */
	UPROPERTY()
	int32 Remaining = 0;

	/** How many of them to create each frame. */
	UPROPERTY()
	int32 PerFrame = 1;
};
