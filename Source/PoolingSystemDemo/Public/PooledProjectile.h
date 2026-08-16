// Copyright Efe Arda Sakarya. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PoolableInterface.h"
#include "PooledProjectile.generated.h"

class UPoolLifetimeComponent;
class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

/**
 * A projectile that lives in a pool.
 *
 * Derive a Blueprint from this, pick a mesh and an impact effect class, and it handles the rest:
 * it launches itself when the pool hands it out, spawns a pooled impact effect where it lands, and
 * returns itself either on impact or when its lifetime runs out.
 */
UCLASS(Abstract, Blueprintable, meta = (DisplayName = "Pooled Projectile"))
class POOLINGSYSTEMDEMO_API APooledProjectile : public AActor, public IPoolable
{
	GENERATED_BODY()

public:
	APooledProjectile();

	/** Speed applied along the actor's forward axis each time it leaves the pool. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile", meta = (ClampMin = "0.0"))
	float LaunchSpeed = 3000.0f;

	/** Spawned at the impact point. Pooled like everything else, so give it a Pool Lifetime component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	TSubclassOf<AActor> ImpactEffectClass;

	/** Returns the projectile to the pool as soon as it hits something. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	bool bDespawnOnImpact = true;

	//~ Begin IPoolable interface
	virtual void OnPoolSpawned_Implementation() override;
	virtual void OnPoolDespawned_Implementation() override;
	//~ End IPoolable interface

	/** Hands the projectile back to its pool. Safe to call more than once. */
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void ReturnToPool();

protected:
	virtual void BeginPlay() override;

	/** Runs on impact, before the projectile is returned. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Projectile", meta = (DisplayName = "On Projectile Impact"))
	void ReceiveProjectileImpact(const FHitResult& ImpactResult);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> MovementComponent;

	/** Backstop for projectiles that never hit anything. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UPoolLifetimeComponent> LifetimeComponent;

private:
	UFUNCTION()
	void HandleProjectileStop(const FHitResult& ImpactResult);

	/** A fast projectile can register two impacts in one frame; only the first one counts. */
	bool bImpacted = false;
};
