// Copyright Efe Arda Sakarya. All Rights Reserved.

#include "PoolingDemoLibrary.h"

#include "Engine/Engine.h"
#include "Engine/World.h"

namespace
{
	/** Unscaled so that pausing or slow motion does not distort the readout. */
	float GetRealDeltaSeconds(const UObject* WorldContextObject)
	{
		const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
		return World ? World->GetDeltaSeconds() : 0.0f;
	}
}

float UPoolingDemoLibrary::GetInstantFPS(const UObject* WorldContextObject)
{
	const float DeltaSeconds = GetRealDeltaSeconds(WorldContextObject);
	return DeltaSeconds > 0.0f ? 1.0f / DeltaSeconds : 0.0f;
}

float UPoolingDemoLibrary::GetSmoothedFPS(const UObject* WorldContextObject, float PreviousFPS, float InterpSpeed)
{
	const float DeltaSeconds = GetRealDeltaSeconds(WorldContextObject);
	const float InstantFPS = DeltaSeconds > 0.0f ? 1.0f / DeltaSeconds : 0.0f;

	// First call, or a reset: snap instead of easing up from zero.
	if (PreviousFPS <= 0.0f)
	{
		return InstantFPS;
	}

	return FMath::FInterpTo(PreviousFPS, InstantFPS, DeltaSeconds, InterpSpeed);
}

float UPoolingDemoLibrary::GetFrameTimeMilliseconds(const UObject* WorldContextObject)
{
	return GetRealDeltaSeconds(WorldContextObject) * 1000.0f;
}
