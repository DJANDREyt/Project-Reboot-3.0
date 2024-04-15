#include "FortGameModeZone.h"

#include "KismetStringLibrary.h"

#include "reboot.h"
#include "gui.h"
#include "FortPlayerControllerZone.h"

bool AFortGameModeZone::Zone_ReadyToStartMatchHook(AFortGameModeZone* GameMode)
{
	Globals::bHitReadyToStartMatch = true;

	auto GameState = GameMode->GetGameStateZone();

	static int LastNum2 = 1;

	if (AmountOfRestarts != LastNum2)
	{
		LastNum2 = AmountOfRestarts;

		LOG_INFO(LogDev, "Presetup!");

		/*

		static auto WorldManagerOffset = GameState->GetOffset("WorldManager", false);

		if (WorldManagerOffset != -1) // needed?
		{
			auto WorldManager = GameState->Get(WorldManagerOffset);

			if (WorldManager)
			{
				static auto WorldManagerStateOffset = WorldManager->GetOffset("WorldManagerState", false);

				if (WorldManagerStateOffset != -1) // needed?
				{
					enum class EFortWorldManagerState : uint8_t
					{
						WMS_Created = 0,
						WMS_QueryingWorld = 1,
						WMS_WorldQueryComplete = 2,
						WMS_CreatingNewWorld = 3,
						WMS_LoadingExistingWorld = 4,
						WMS_Running = 5,
						WMS_Failed = 6,
						WMS_MAX = 7
					};

					LOG_INFO(LogDev, "Old WorldManager State: {}", (int)WorldManager->Get<EFortWorldManagerState>(WorldManagerStateOffset));
					WorldManager->Get<EFortWorldManagerState>(WorldManagerStateOffset) = EFortWorldManagerState::WMS_Running; // needed? right time?
				}
			}
		}

		*/
		static auto bEnableReplicationGraphOffset = GameMode->GetOffset("bEnableReplicationGraph");
		GameMode->Get<bool>(bEnableReplicationGraphOffset) = true;
		static auto bWorldIsReadyOffset = GameMode->GetOffset("bWorldIsReady");
		SetBitfield(GameMode->GetPtr<PlaceholderBitfield>(bWorldIsReadyOffset), 1, true); // idk when we actually set this

		// Calendar::SetSnow(1000);

		LOG_INFO(LogDev, "Finished presetup!");

		Globals::bInitializedPlaylist = true;
	}

	static int LastNum = 1;

	if (AmountOfRestarts != LastNum)
	{
		LastNum = AmountOfRestarts;

		LOG_INFO(LogDev, "Initializing!");

		if (Fortnite_Version >= 3.5 && Fortnite_Version <= 4) // todo check 3.4
		{
			//SetupEverythingAI();
		}

		LOG_INFO(LogDev, "GameMode 0x{:x}", __int64(GameMode));


		LOG_INFO(LogDev, "Initialized!");
	}

	static int LastNum3 = 1;

	if (AmountOfRestarts != LastNum3)
	{
		LastNum3 = AmountOfRestarts;
		++Globals::AmountOfListens;

		LOG_INFO(LogNet, "Attempting to listen!");

		GetWorld()->Listen();

		LOG_INFO(LogNet, "WorldLevel {}", GameState->GetWorldLevel());

#ifndef ABOVE_S20
		if (Globals::AmountOfListens == 1) // we only want to do this one time.
		{
			if (bEnableRebooting)
			{
				auto GameSessionDedicatedAthenaPatch = Memcury::Scanner::FindPattern("3B 41 38 7F ? 48 8B D0 48 8B 41 30 4C 39 04 D0 75 ? 48 8D 96", false).Get(); // todo check this sig more

				if (GameSessionDedicatedAthenaPatch)
				{
					PatchBytes(GameSessionDedicatedAthenaPatch, { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });
				}
				else
				{
					auto S19Patch = Memcury::Scanner::FindPattern("74 1A 48 8D 97 ? ? ? ? 49 8B CF E8 ? ? ? ? 88 87 ? ? ? ? E9", false).Get();

					if (S19Patch)
					{
						PatchByte(S19Patch, 0x75);
					}
					else
					{
						auto S18Patch = Memcury::Scanner::FindPattern("75 02 33 F6 41 BE ? ? ? ? 48 85 F6 74 17 48 8D 93").Get();

						if (S18Patch)
						{
							PatchByte(S18Patch, 0x74);
						}
					}
				}

				LOG_INFO(LogDev, "Patched GameSession!");
			}
		}
#endif

		static auto PawnClass = FindObject<UClass>(L"/Game/Abilities/Player/Pawns/PlayerPawn_Generic.PlayerPawn_Generic_C");
		static auto DefaultPawnClassOffset = GameMode->GetOffset("DefaultPawnClass");

		static auto ReplicationDriverOffset = GetWorld()->GetNetDriver()->GetOffset("ReplicationDriver", false); // If netdriver is null the world blows up

		Globals::bShouldUseReplicationGraph = (!(ReplicationDriverOffset == -1 || Fortnite_Version >= 20))
			&& Fortnite_Version != 3.3; // RepGraph is half implemented

		LOG_INFO(LogDev, "bShouldUseReplicationGraph: {}", Globals::bShouldUseReplicationGraph);

		Globals::bStartedListening = true;
	}

	bool Ret = false;

	if (!Ret)
		Ret = Zone_ReadyToStartMatchOriginal(GameMode);

	if (Ret)
	{
		LOG_INFO(LogDev, "Zone_ReadyToStartMatchOriginal RET!"); // if u dont see this, not good
	}

	return Ret;
}

static AActor* FindPlayerStart()
{
	TArray<AActor*> Actors;
	static auto PlayerStartClass = FindObject<UClass>(L"/Script/Engine.PlayerStart");
	static auto PlayerSpawnPlacementActorClass = FindObject<UClass>(L"/Game/Building/ObjectivePlacementActors/PlayerSpawnPlacementActor.PlayerSpawnPlacementActor_C");
	Actors = UGameplayStatics::GetAllActorsOfClass(GetWorld(), Globals::bFarmstead ? PlayerStartClass : PlayerSpawnPlacementActorClass);
	if (Actors.Num() == 0)
	{
		std::cout << "FindPlayerStart: actors num is 0!\n";
		return nullptr;
	}
	auto randIndex = rand() % Actors.Num();
	auto Spawn = Globals::bFarmstead ? Actors.at(0) : Actors.IsValidIndex(randIndex) ? Actors.at(rand() % Actors.Num()) : Actors.at(0);
	Actors.Free();
	if (Spawn)
	{
		return Spawn;
	}
	else
	{
		std::cout << "FindPlayerStart: SpawnActor is null!\n";
		return nullptr;
	}
}

static FVector FindSpawnLocation()
{
	auto PS = FindPlayerStart();
	return PS ? PS->GetActorLocation() : FVector(0, 0, 10000);
}

void AFortGameModeZone::Zone_HandleStartingNewPlayerHook(AFortGameModeZone* GameMode, AActor* NewPlayerActor)
{
	if (NewPlayerActor == GetLocalPlayerController()) // we dont really need this but it also functions as a nullptr check usually
		return;

	auto GameState = GameMode->GetGameStateZone();

	LOG_INFO(LogPlayer, "HandleStartingNewPlayer!");

	// if (Engine_Version < 427)
	{
		static int LastNum69 = 19451;

		if (LastNum69 != Globals::AmountOfListens)
		{
			LastNum69 = Globals::AmountOfListens;

		}
	}

	auto NewPlayer = (AFortPlayerControllerZone*)NewPlayerActor;

	static auto PawnClass = FindObject<UClass>(L"/Game/Abilities/Player/Pawns/PlayerPawn_Generic.PlayerPawn_Generic_C");
	LOG_DEBUG(LogDev, "PawnClass {}", PawnClass->GetName());

	FTransform SpawnTransform;
	SpawnTransform.Translation = FindSpawnLocation();
	LOG_DEBUG(LogDev, "FindSpawnLocation: X {} Y {} Z {}", SpawnTransform.Translation.X, SpawnTransform.Translation.Y, SpawnTransform.Translation.Z);

	auto Pawn = GetWorld()->SpawnActor<AFortPlayerPawn>(PawnClass, SpawnTransform, CreateSpawnParameters(ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn));

	NewPlayer->Possess(Pawn);
	NewPlayer->GetPawn() = Pawn;
	Pawn->SetOwner(NewPlayer);
	NewPlayer->GetMyFortPawn() = Pawn;

	auto PlayerStateZone = NewPlayer->GetPlayerStateZone();

	if (!PlayerStateZone)
		return Zone_HandleStartingNewPlayerOriginal(GameMode, NewPlayerActor);

	auto ASC = PlayerStateZone->GetAbilitySystemComponent();

	auto PlayerAbilitySet = GetPlayerAbilitySet(); // Apply default gameplay effects // We need to move maybe?

	if (PlayerAbilitySet && ASC)
	{
		PlayerAbilitySet->ApplyGrantedGameplayAffectsToAbilitySystem(ASC);
	}

	auto WorldInventory = NewPlayer->GetWorldInventory();

	if (WorldInventory->IsValidLowLevel())
	{
		if (!WorldInventory->GetPickaxeInstance())
		{
			// TODO Check Playlist->bRequirePickaxeInStartingInventory

			auto& StartingItems = ((AFortGameModeAthena*)GameMode)->GetStartingItems();

		
			NewPlayer->AddPickaxeToInventory();

			for (int i = 0; i < StartingItems.Num(); ++i)
			{
				auto& StartingItem = StartingItems.at(i);

				WorldInventory->AddItem(StartingItem.GetItem(), nullptr, StartingItem.GetCount());
			}

			/* if (Globals::bLateGame)
			{
				auto SpawnIslandTierGroup = UKismetStringLibrary::Conv_StringToName(L"Loot_AthenaFloorLoot_Warmup");

				for (int i = 0; i < 5; ++i)
				{
					auto LootDrops = PickLootDrops(SpawnIslandTierGroup);

					for (auto& LootDrop : LootDrops)
					{
						WorldInventory->AddItem(LootDrop.ItemDefinition, nullptr, LootDrop.Count, LootDrop.LoadedAmmo);
					}
				}
			} */

		}

		const auto& ItemInstances = WorldInventory->GetItemList().GetItemInstances();
		const auto& ReplicatedEntries = WorldInventory->GetItemList().GetReplicatedEntries();

		for (int i = 0; i < ItemInstances.Num(); ++i)
		{
			auto ItemInstance = ItemInstances.at(i);

			if (!ItemInstance) continue;

			auto WeaponItemDefinition = Cast<UFortWeaponItemDefinition>(ItemInstance->GetItemEntry()->GetItemDefinition());

			if (!WeaponItemDefinition) continue;

			ItemInstance->GetItemEntry()->GetLoadedAmmo() = WeaponItemDefinition->GetClipSize();
			WorldInventory->GetItemList().MarkItemDirty(ItemInstance->GetItemEntry());
		}

		for (int i = 0; i < ReplicatedEntries.Num(); ++i)
		{
			auto ReplicatedEntry = ReplicatedEntries.AtPtr(i, FFortItemEntry::GetStructSize());

			auto WeaponItemDefinition = Cast<UFortWeaponItemDefinition>(ReplicatedEntry->GetItemDefinition());

			if (!WeaponItemDefinition) continue;

			ReplicatedEntry->GetLoadedAmmo() = WeaponItemDefinition->GetClipSize();
			WorldInventory->GetItemList().MarkItemDirty(ReplicatedEntry);
		}

		WorldInventory->Update();
	}

	Pawn->OnCharacterPartsReinitialized();
	PlayerStateZone->OnRep_CharacterParts();


	if (Fortnite_Version < 14) // Fixes LS not dropping // Probably not needed on any build
	{
		static auto bHasServerFinishedLoadingOffset = NewPlayer->GetOffset("bHasServerFinishedLoading");
		NewPlayer->Get<bool>(bHasServerFinishedLoadingOffset) = true;

		static auto OnRep_bHasServerFinishedLoadingFn = FindObject<UFunction>(L"/Script/FortniteGame.FortPlayerController.OnRep_bHasServerFinishedLoading");
		NewPlayer->ProcessEvent(OnRep_bHasServerFinishedLoadingFn);

		static auto bHasStartedPlayingOffset = PlayerStateZone->GetOffset("bHasStartedPlaying");
		static auto bHasStartedPlayingFieldMask = GetFieldMask(PlayerStateZone->GetProperty("bHasStartedPlaying"));
		PlayerStateZone->SetBitfieldValue(bHasStartedPlayingOffset, bHasStartedPlayingFieldMask, true);

		static auto OnRep_bHasStartedPlayingFn = FindObject<UFunction>(L"/Script/FortniteGame.FortPlayerState.OnRep_bHasStartedPlaying");
		PlayerStateZone->ProcessEvent(OnRep_bHasStartedPlayingFn);
	}



	PlayerStateZone->GetWorldPlayerId() = PlayerStateZone->GetPlayerID();

	static auto PlayerCameraManagerOffset = NewPlayer->GetOffset("PlayerCameraManager");
	auto PlayerCameraManager = NewPlayer->Get(PlayerCameraManagerOffset);

	if (PlayerCameraManager)
	{
		static auto ViewRollMinOffset = PlayerCameraManager->GetOffset("ViewRollMin");
		PlayerCameraManager->Get<float>(ViewRollMinOffset) = 0;

		static auto ViewRollMaxOffset = PlayerCameraManager->GetOffset("ViewRollMax");
		PlayerCameraManager->Get<float>(ViewRollMaxOffset) = 0;
	}

	//NewPlayer->ServerReadyToStartMatchHook(NewPlayer);

	static auto QuickBarsOffset = NewPlayer->GetOffset("QuickBars", false);

	if (QuickBarsOffset != -1)
	{
		auto& QuickBars = NewPlayer->Get<AActor*>(QuickBarsOffset);

		LOG_INFO(LogDev, "QuickBarsOld: {}", __int64(QuickBars));

		if (QuickBars) LOG_INFO(LogDev, "Quickbars already valid!");

		static auto FortQuickBarsClass = FindObject<UClass>("/Script/FortniteGame.FortQuickBars");

		QuickBars = GetWorld()->SpawnActor<AActor>(FortQuickBarsClass);

		LOG_INFO(LogDev, "QuickBarsNew: {}", __int64(QuickBars));

		if (!QuickBars)

		NewPlayer->Get<AActor*>(QuickBarsOffset)->SetOwner(NewPlayer);

		static auto OnRep_QuickBar = FindObject<UFunction>("/Script/FortniteGame.FortPlayerController.OnRep_QuickBar");
		NewPlayer->ProcessEvent(OnRep_QuickBar, nullptr);
	}

	LOG_INFO(LogDev, "HandleStartingNewPlayer end");


	return Zone_HandleStartingNewPlayerOriginal(GameMode, NewPlayerActor);
}

UClass* AFortGameModeZone::StaticClass()
{
	static auto Class = FindObject<UClass>(L"/Script/FortniteGame.FortGameModeZone");
	return Class;
}