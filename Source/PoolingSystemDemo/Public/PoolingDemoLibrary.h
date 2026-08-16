// Copyright Efe Arda Sakarya. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PoolingDemoLibrary.generated.h"

/**
 * Small helpers for building a benchmark overlay.
 *
 * Deliberately stateless: no globals, no ticking singleton. The smoothing function takes the
 * previous value back as a parameter, so a HUD keeps its own average in a Blueprint variable.
 */
UCLASS()
class POOLINGSYSTEMDEMO_API UPoolingDemoLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Frames per second for this frame alone. Jumps around; fine for a raw readout. */
	UFUNCTION(BlueprintPure, Category = "Pooling|Demo", meta = (WorldContext = "WorldContextObject", Keywords = "fps framerate performance"))
	static float GetInstantFPS(const UObject* WorldContextObject);

	/**
	 * Eases the previous reading towards this frame's, which is what you want on screen.
	 * Feed the result back in as PreviousFPS next frame.
	 */
	UFUNCTION(BlueprintPure, Category = "Pooling|Demo", meta = (WorldContext = "WorldContextObject", Keywords = "fps framerate smooth average performance"))
	static float GetSmoothedFPS(const UObject* WorldContextObject, float PreviousFPS, float InterpSpeed = 3.0f);

	/** Milliseconds spent on this frame. The number that actually matters for a comparison. */
	UFUNCTION(BlueprintPure, Category = "Pooling|Demo", meta = (WorldContext = "WorldContextObject", Keywords = "frame time ms performance"))
	static float GetFrameTimeMilliseconds(const UObject* WorldContextObject);
};
