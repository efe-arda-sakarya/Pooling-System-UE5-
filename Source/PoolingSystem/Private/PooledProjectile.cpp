// Copyright Efe Arda Sakarya. All Rights Reserved.

#include "PooledProjectile.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "PoolLifetimeComponent.h"
#include "PoolingSubsystem.h"

APooledProjectile::APooledProjectile()
{
	// Movement is handled by the movement component, so the actor itself never needs to tick.
	PrimaryActorTick.bCanEverTick = false;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	CollisionComponent->InitSphereRadius(8.0f);
	CollisionComponent->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	RootComponent = CollisionComponent;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComponent->SetupAttachment(CollisionComponent);
	// The sphere above is the only collider; a second one would double the physics work.
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	MovementComponent->SetUpdatedComponent(CollisionComponent);
	MovementComponent->ProjectileGravityScale = 0.0f;
	MovementComponent->bRotationFollowsVelocity = true;
	MovementComponent->bShouldBounce = false;
	// Velocity is set on every spawn instead, so the component must not launch itself on creation —
	// a pooled instance is created long before it is first fired.
	MovementComponent->InitialSpeed = 0.0f;
	MovementComponent->MaxSpeed = 0.0f;
	MovementComponent->bAutoActivate = false;

	LifetimeComponent = CreateDefaultSubobject<UPoolLifetimeComponent>(TEXT("Lifetime"));
	LifetimeComponent->Lifetime = 3.0f;
}

void APooledProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Bound once per instance. Pooled actors run BeginPlay only when they are first created, which
	// is exactly the behaviour we want for a delegate binding.
	if (MovementComponent)
	{
		MovementComponent->OnProjectileStop.AddDynamic(this, &APooledProjectile::HandleProjectileStop);
	}
}

void APooledProjectile::OnPoolSpawned_Implementation()
{
	bImpacted = false;

	if (MovementComponent)
	{
		MovementComponent->SetUpdatedComponent(CollisionComponent);
		MovementComponent->Velocity = GetActorForwardVector() * LaunchSpeed;
		MovementComponent->Activate(true);
		MovementComponent->SetComponentTickEnabled(true);
	}
}

void APooledProjectile::OnPoolDespawned_Implementation()
{
	if (MovementComponent)
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->SetComponentTickEnabled(false);
	}
}

void APooledProjectile::HandleProjectileStop(const FHitResult& ImpactResult)
{
	if (bImpacted)
	{
		return;
	}
	bImpacted = true;

	if (ImpactEffectClass)
	{
		if (UPoolingSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UPoolingSubsystem>() : nullptr)
		{
			// Face the effect along the surface normal so decals and flashes sit flat on the wall.
			const FTransform ImpactTransform(ImpactResult.ImpactNormal.Rotation(), ImpactResult.ImpactPoint);

			bool bSuccess = false;
			Subsystem->SpawnPooledActor(ImpactEffectClass, ImpactTransform, bSuccess);
		}
	}

	ReceiveProjectileImpact(ImpactResult);

	if (bDespawnOnImpact)
	{
		ReturnToPool();
	}
}

void APooledProjectile::ReturnToPool()
{
	if (UPoolingSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UPoolingSubsystem>() : nullptr)
	{
		Subsystem->DespawnPooledActor(this);
	}
}
