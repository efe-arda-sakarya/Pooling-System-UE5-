// Copyright Efe Arda Sakarya. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PoolLifetimeComponent.generated.h"

/**
 * Returns its owner to the pool after a fixed time.
 *
 * Add it to anything short-lived — impact flashes, tracers, pickups that expire — and the actor
 * despawns itself with no Blueprint wiring. The countdown starts when the pool hands the actor out
 * and is cancelled if the actor is returned early.
 */
UCLASS(ClassGroup = (Pooling), Blueprintable, meta = (BlueprintSpawnableComponent, DisplayName = "Pool Lifetime"))
class POOLINGSYSTEM_API UPoolLifetimeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPoolLifetimeComponent();

	/** Seconds the owner stays active before returning itself. 0 disables the countdown. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Lifetime = 2.0f;

	/** Start counting as soon as the pool hands the owner out. Turn off to start it yourself. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling")
	bool bAutoStart = true;

	/** Begins (or restarts) the countdown. */
	UFUNCTION(BlueprintCallable, Category = "Pooling")
	void StartLifetime();

	/** Stops the countdown without returning the owner. */
	UFUNCTION(BlueprintCallable, Category = "Pooling")
	void CancelLifetime();

	/** Seconds left before the owner returns itself, or 0 when no countdown is running. */
	UFUNCTION(BlueprintPure, Category = "Pooling")
	float GetRemainingLifetime() const;

private:
	void HandleLifetimeExpired();

	FTimerHandle LifetimeTimer;
};
