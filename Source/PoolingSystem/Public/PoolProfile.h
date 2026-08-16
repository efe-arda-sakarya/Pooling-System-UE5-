// Copyright Efe Arda Sakarya. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PoolTypes.h"
#include "PoolProfile.generated.h"

/**
 * A reusable set of pool definitions.
 *
 * Create one in the Content Browser, list the classes you pool and how many of each, then hand it
 * to a Pool Prewarmer or call Prewarm From Profile. The same profile can be shared by several
 * levels, so pool sizes live in one place instead of being retyped per level.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Pool Profile"))
class POOLINGSYSTEM_API UPoolProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Pools to create, one row per actor class. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pooling", meta = (TitleProperty = "ActorClass"))
	TArray<FPoolSpec> Pools;
};
