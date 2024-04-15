#pragma once

#include "FortGameMode.h"
#include "FortGameStateZone.h"
#include "FortItemDefinition.h"

#include "Engine.h"

class AFortGameModeZone : public AFortGameMode
{
public:
	static inline bool (*Zone_ReadyToStartMatchOriginal)(AFortGameModeZone* GameMode);
	static inline void (*Zone_HandleStartingNewPlayerOriginal)(AFortGameModeZone* GameMode, AActor* NewPlayer);

	AFortGameStateZone* GetGameStateZone()
	{
		return (AFortGameStateZone*)GetGameState();
	}

	TArray<FItemAndCount>& GetStartingItems() // really in zone
	{
		static auto StartingItemsOffset = GetOffset("StartingItems");
		return Get<TArray<FItemAndCount>>(StartingItemsOffset);
	}

	static bool Zone_ReadyToStartMatchHook(AFortGameModeZone* GameMode);
	static void Zone_HandleStartingNewPlayerHook(AFortGameModeZone* GameMode, AActor* NewPlayerActor);

	static UClass* StaticClass();
};