#pragma once
#include "GameState.h"

class AFortGameStateZone : public AGameState
{
public:

	static UClass* StaticClass();

	int& GetWorldLevel() // Actually in AFortGameState
	{
		static auto WorldLevelOffset = GetOffset("WorldLevel");
		return Get<int>(WorldLevelOffset);
	}
};
