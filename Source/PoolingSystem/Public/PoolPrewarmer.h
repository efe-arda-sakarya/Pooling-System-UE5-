// Copyright Efe Arda Sakarya. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PoolTypes.h"
#include "PoolPrewarmer.generated.h"

class UPoolProfile;

/**
 * Optional. Drop one into a level and fill in the list to have those pools ready before they are
 * first needed. Without it everything still works — pools are created the first time they are used.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Pool Prewarmer"))
class POOLINGSYSTEM_API APoolPrewarmer : public AActor
{
	GENERATED_BODY()

public:
	APoolPrewarmer();

	/**
	 * A shared set of pool definitions. Applied first, so the list below can override single rows
	 * for this level without editing the asset.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling")
	TObjectPtr<UPoolProfile> Profile;

	/** Pools to prepare when the level starts, on top of anything the profile already defines. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling", meta = (TitleProperty = "ActorClass"))
	TArray<FPoolSpec> PoolsToPrewarm;

protected:
	virtual void BeginPlay() override;
};
