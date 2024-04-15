#pragma once
#include "FortPlayerController.h"
#include "FortPlayerStateZone.h"

class AFortPlayerControllerZone : public AFortPlayerController
{
public:
	static inline void (*GetPlayerViewPointOriginal)(AFortPlayerControllerZone* PlayerController, FVector& Location, FRotator& Rotation);
	static inline void (*ServerReadyToStartMatchOriginal)(AFortPlayerControllerZone* PlayerController);

	AFortPlayerStateZone* GetPlayerStateZone()
	{
		return (AFortPlayerStateZone*)GetPlayerState();
	}

	static void ServerRestartPlayerHook(AFortPlayerControllerZone* Controller);
	static void ServerAcknowledgePossessionHook(APlayerController* Controller, APawn* Pawn);
	static void GetPlayerViewPointHook(AFortPlayerControllerZone* PlayerController, FVector& Location, FRotator& Rotation);
	static void ServerReadyToStartMatchHook(AFortPlayerControllerZone* PlayerController);
	static void UpdateTrackedAttributesHook(AFortPlayerControllerZone* PlayerController);
};