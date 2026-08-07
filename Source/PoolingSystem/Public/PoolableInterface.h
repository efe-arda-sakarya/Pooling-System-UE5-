// Copyright Efe Arda Sakarya. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PoolableInterface.generated.h"

UINTERFACE(Blueprintable, BlueprintType, meta = (DisplayName = "Poolable"))
class POOLINGSYSTEM_API UPoolable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Optional hook for pooled actors.
 *
 * The pool already handles visibility, collision, ticking and physics velocity.
 * Implement this interface only to reset your own state: health, target, counters, timers.
 * Actors that do not implement it still pool correctly.
 */
class POOLINGSYSTEM_API IPoolable
{
	GENERATED_BODY()

public:
	/** Called after the actor is placed and re-activated, before the caller gets it. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pooling")
	void OnPoolSpawned();
	virtual void OnPoolSpawned_Implementation() {}

	/** Called as the actor goes back into the pool. Reset your own state here. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pooling")
	void OnPoolDespawned();
	virtual void OnPoolDespawned_Implementation() {}
};
