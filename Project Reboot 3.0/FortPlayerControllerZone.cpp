#include "FortPlayerControllerZone.h"
#include "FortGadgetItemDefinition.h"
#include "FortPlayerPawn.h"
#include "hooking.h"

void AFortPlayerControllerZone::ServerAcknowledgePossessionHook(APlayerController* Controller, APawn* Pawn)
{
	static auto AcknowledgedPawnOffset = Controller->GetOffset("AcknowledgedPawn");

	const APawn* OldAcknowledgedPawn = Controller->Get<APawn*>(AcknowledgedPawnOffset);
	Controller->Get<APawn*>(AcknowledgedPawnOffset) = Pawn;

	auto ControllerAsFort = Cast<AFortPlayerController>(Controller);
	auto PawnAsFort = Cast<AFortPlayerPawn>(Pawn);
	auto PlayerStateAsFort = Cast<AFortPlayerState>(Pawn->GetPlayerState());

	if (!PawnAsFort)
		return;

	if (Globals::bNoMCP)
	{
		return;
	}

	//PlayerStateAsFort->//TODO: On CaracterPartsReinitialized
}

void AFortPlayerControllerZone::GetPlayerViewPointHook(AFortPlayerControllerZone* PlayerController, FVector& Location, FRotator& Rotation)
{
	// I don't know why but GetActorEyesViewPoint only works on some versions.
	/* static auto GetActorEyesViewPointFn = FindObject<UFunction>(L"/Script/Engine.Actor.GetActorEyesViewPoint");
	static auto GetActorEyesViewPointIndex = GetFunctionIdxOrPtr(GetActorEyesViewPointFn) / 8;

	void (*GetActorEyesViewPointOriginal)(AActor* Actor, FVector* OutLocation, FRotator* OutRotation) = decltype(GetActorEyesViewPointOriginal)(PlayerController->VFTable[GetActorEyesViewPointIndex]);
	return GetActorEyesViewPointOriginal(PlayerController, &Location, &Rotation); */

	if (auto MyFortPawn = PlayerController->GetMyFortPawn())
	{
		Location = MyFortPawn->GetActorLocation();
		Rotation = PlayerController->GetControlRotation();
		return;
	}

	return AFortPlayerControllerZone::GetPlayerViewPointOriginal(PlayerController, Location, Rotation);
}

void AFortPlayerControllerZone::ServerReadyToStartMatchHook(AFortPlayerControllerZone* PlayerController)
{
	LOG_INFO(LogDev, "ServerReadyToStartMatch!");

	//if (Fortnite_Version <= 2.5) // techinally we should do this at the end of OnReadyToStartMatch
	{
		static auto QuickBarsOffset = PlayerController->GetOffset("QuickBars", false);

		if (QuickBarsOffset != -1)
		{
			auto& QuickBars = PlayerController->Get<AActor*>(QuickBarsOffset);

			// LOG_INFO(LogDev, "QuickBarsOld: {}", __int64(QuickBars));

			if (QuickBars)
				return ServerReadyToStartMatchOriginal(PlayerController);

			static auto FortQuickBarsClass = FindObject<UClass>("/Script/FortniteGame.FortQuickBars");

			QuickBars = GetWorld()->SpawnActor<AActor>(FortQuickBarsClass);

			// LOG_INFO(LogDev, "QuickBarsNew: {}", __int64(QuickBars));

			if (!QuickBars)
				return ServerReadyToStartMatchOriginal(PlayerController);

			PlayerController->Get<AActor*>(QuickBarsOffset)->SetOwner(PlayerController);
		}
	}

	return ServerReadyToStartMatchOriginal(PlayerController);
}

void AFortPlayerControllerZone::ServerRestartPlayerHook(AFortPlayerControllerZone* Controller)
{
	static auto FortPlayerControllerZoneDefault = FindObject<UClass>(L"/Script/FortniteGame.Default__FortPlayerControllerZone");
	static auto ServerRestartPlayerFn = FindObject<UFunction>(L"/Script/Engine.PlayerController.ServerRestartPlayer");
	static auto ZoneServerRestartPlayer = __int64(FortPlayerControllerZoneDefault->VFTable[GetFunctionIdxOrPtr(ServerRestartPlayerFn) / 8]);
	static void (*ZoneServerRestartPlayerOriginal)(AFortPlayerController*) = decltype(ZoneServerRestartPlayerOriginal)(__int64(ZoneServerRestartPlayer));

	// auto NAME_Spectating = UKismetStringLibrary::Conv_StringToName(L"NAME_Spectating");

	// LOG_INFO(LogDev, "ISplayerwaiting: {}", Controller->IsPlayerWaiting());

	// Controller->GetStateName() = NAME_Spectating;
	// Controller->SetPlayerIsWaiting(true);

	LOG_INFO(LogDev, "ServerRestartPlayerHook Call 0x{:x} returning with 0x{:x}!", ZoneServerRestartPlayer - __int64(_ReturnAddress()), __int64(ZoneServerRestartPlayerOriginal) - __int64(GetModuleHandleW(0)));
	return ZoneServerRestartPlayerOriginal(Controller);
}

void AFortPlayerControllerZone::UpdateTrackedAttributesHook(AFortPlayerControllerZone* PlayerController)
{
	LOG_INFO(LogDev, "UpdateTrackedAttributesHook Return: 0x{:x}", __int64(_ReturnAddress()) - __int64(GetModuleHandleW(0)));

	// IDK IF GADGET IS A PARAM OR WHAT

	auto PlayerState = Cast<AFortPlayerStateZone>(PlayerController->GetPlayerState()); // really we only need zone

	if (!PlayerState)
		return;

	auto ASC = PlayerState->GetAbilitySystemComponent();

	if (!ASC)
		return;

	auto WorldInventory = PlayerController->GetWorldInventory();

	if (!WorldInventory)
		return;

	auto& ItemInstances = WorldInventory->GetItemList().GetItemInstances();

	std::vector<UFortItem*> ItemInstancesToRemove;

	for (int i = 0; i < ItemInstances.Num(); ++i)
	{
		auto ItemInstance = ItemInstances.at(i);
		auto GadgetItemDefinition = Cast<UFortGadgetItemDefinition>(ItemInstance->GetItemEntry()->GetItemDefinition());

		if (!GadgetItemDefinition)
			continue;

		if (!GadgetItemDefinition->ShouldDestroyGadgetWhenTrackedAttributesIsZero())
			continue;

		bool bIsTrackedAttributesZero = true;

		for (int i = 0; i < GadgetItemDefinition->GetTrackedAttributes().Num(); ++i)
		{
			auto& CurrentTrackedAttribute = GadgetItemDefinition->GetTrackedAttributes().at(i);

			int CurrentAttributeValue = -1;

			for (int i = 0; i < ASC->GetSpawnedAttributes().Num(); ++i)
			{
				auto CurrentSpawnedAttribute = ASC->GetSpawnedAttributes().at(i);

				if (CurrentSpawnedAttribute->IsA(CurrentTrackedAttribute.AttributeOwner))
				{
					auto PropertyOffset = CurrentSpawnedAttribute->GetOffset(CurrentTrackedAttribute.GetAttributePropertyName());

					if (PropertyOffset != -1)
					{
						if (CurrentSpawnedAttribute->GetPtr<FFortGameplayAttributeData>(PropertyOffset)->GetCurrentValue() > 0)
						{
							bIsTrackedAttributesZero = false;
							break; // hm
						}
					}
				}
			}
		}

		if (bIsTrackedAttributesZero)
		{
			ItemInstancesToRemove.push_back(ItemInstance);
		}
	}

	for (auto ItemInstanceToRemove : ItemInstancesToRemove)
	{
		auto GadgetItemDefinition = Cast<UFortGadgetItemDefinition>(ItemInstanceToRemove->GetItemEntry()->GetItemDefinition());

		WorldInventory->RemoveItem(ItemInstanceToRemove->GetItemEntry()->GetItemGuid(), nullptr, ItemInstanceToRemove->GetItemEntry()->GetCount(), true);

		static auto MulticastTriggerOnGadgetTrackedAttributeDestroyedFXFn = FindObject<UFunction>(L"/Script/FortniteGame.FortPlayerStateZone.MulticastTriggerOnGadgetTrackedAttributeDestroyedFX");
		PlayerState->ProcessEvent(MulticastTriggerOnGadgetTrackedAttributeDestroyedFXFn, &GadgetItemDefinition);
	}

	if (ItemInstancesToRemove.size() > 0)
		WorldInventory->Update();
}