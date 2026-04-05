#include "PCH.h"
#include "logger.h"
#include "external/simpleini/SimpleIni.h"


/*
 * @param: RE::FormID containerID - The FormID of the container or actor whose name you want to retrieve
 * @return: "Unknown Container" if the container or actor cannot be found, the name of the actor if the FormID to an actor, or the name of the container if it is not an actor
 */
std::string GetContainerOrActorName(RE::FormID containerID) {
    auto container = RE::TESForm::LookupByID(containerID);
    if (!container) {
        return "Unknown Container";
    }

    if (auto *actor = container->As<RE::Actor>()) {
        return actor->GetDisplayFullName();
    }

    return container->GetName();

}

/*
 * @param: void
 * @return: true if the player is currently detected by any NPCs, false otherwise
 */
bool isPlayerDetected() {
        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player) return true;
    
        auto* process = player->GetActorRuntimeData().currentProcess;
        if (!process) return true;
    
        auto *high = process->high;
        if (!high) return true;
    
        auto *processLists = RE::ProcessLists::GetSingleton();

        std::uint32_t actorsWithLOS = 0;
        auto level = RE::ProcessLists::GetSingleton()->RequestHighestDetectionLevelAgainstActor(player, actorsWithLOS);
        
        logger::info("Player detection level: {} with {} actors w/ LOS", level, actorsWithLOS);

        return (level > 0);
    }   

/*
* @param: RE::InventoryEntryData* itemEntry - Pointer to the inventory entry data of the item whose ownership want to change
* @param: RE::TESForm* newOwner - Pointer to the new owner of the item. Set to nullptr to remove ownership
* @return: void
*/
void changeItemOwner(RE::InventoryEntryData* itemEntry) {

    if (!itemEntry) {
        logger::info("Inventory entry is null");
        return;
    }

    if (itemEntry->extraLists) {
        logger::info("Removing ownership from item...");
        for (auto *xList : *itemEntry->extraLists) {
            if (xList) {
                if (xList->HasType(RE::ExtraDataType::kOwnership)) {
                    logger::info("Found ownership data for item, removing...");
                    auto *extraOwnership = xList->GetByType<RE::ExtraOwnership>();
                    extraOwnership->owner = nullptr;  // Set owner to 0 to remove ownership
                }
            }
        }
    }

    return;
}

struct OurEventSink : public RE::BSTEventSink<RE::TESContainerChangedEvent> {
    RE::BSEventNotifyControl ProcessEvent(const RE::TESContainerChangedEvent *event, RE::BSTEventSource<RE::TESContainerChangedEvent> *source){ 

        if (!event) {
            return RE::BSEventNotifyControl::kContinue;
        }

        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player) return RE::BSEventNotifyControl::kContinue;

        if (event->newContainer != player->GetFormID()) 
            return RE::BSEventNotifyControl::kContinue;


        auto item = RE::TESForm::LookupByID(event->baseObj);
            if (!item) return RE::BSEventNotifyControl::kContinue;

        auto itemBoundObject = item ? item->As<RE::TESBoundObject>() : nullptr;
            if (!itemBoundObject) return RE::BSEventNotifyControl::kContinue;


        auto playerInventory = player->GetInventory();
            if (playerInventory.empty()) {
                logger::info("Player inventory is empty or not populated");
                return RE::BSEventNotifyControl::kContinue;
            }


        auto itemInInventory = playerInventory.find(itemBoundObject);
        if (itemInInventory == playerInventory.end()) {
            logger::info("Item not found in inventory map yet");
            return RE::BSEventNotifyControl::kContinue;
        }

        const auto &[count, entryptr] = itemInInventory->second;
        if (!entryptr) {
            logger::info("Entry pointer is null");
            return RE::BSEventNotifyControl::kContinue;
        }
        
        auto* entry = itemInInventory->second.second.get();
        if (!entry) {
            logger::info("Inventory entry is null");
            return RE::BSEventNotifyControl::kContinue;
        }

        auto itemName = item ? item->GetName() : "Unknown Item";
        auto itemValue = item ? item->GetGoldValue() : 0;
        bool isOwnedByPlayer = entry->IsOwnedBy(player, true);
        bool isStolen = !isOwnedByPlayer;

        logger::info("Player {} {} {} from {} worth {} septims. {}"
            , isPlayerDetected() ? "openly" : "quietly"
            ,isOwnedByPlayer ? "took" : "stole"
            , itemName
            , GetContainerOrActorName(event->oldContainer)
            , itemValue
            , isOwnedByPlayer ? "[LEGAL]" : "[STOLEN]"
        );

        if (!isPlayerDetected() && !isOwnedByPlayer) {
            changeItemOwner(entry);
            logger::info("Player now owns {} from {} worth {} septims.", itemName,
                            GetContainerOrActorName(event->oldContainer), itemValue);
        }

        return RE::BSEventNotifyControl::kContinue;

    }
};

void OnMessage(SKSE::MessagingInterface::Message *message) {
    if (message->type == SKSE::MessagingInterface::kNewGame) {
        logger::info("New Game started");
    } else if (message->type == SKSE::MessagingInterface::kSaveGame) {
        logger::info("Game Saved");
    } else if (message->type == SKSE::MessagingInterface::kPostLoadGame) {
        logger::info("Traceless Loot is now active.");
        auto eventSink = new OurEventSink();
        auto *eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
        eventSourceHolder->AddEventSink(eventSink);
    }
}

SKSEPluginLoad(const SKSE::LoadInterface *skse) {
    SKSE::Init(skse);

    SetupLog();

    /*
        Debug levels in order:
        Trace
        debug
        info
        warn
        error
        critical
    */

    logger::trace("TRACE");
    logger::debug("DEBUG");
    logger::warn("WARN");
    logger::error("ERROR");
    logger::critical("CRITICAL");

    // Once all plugins and mods are loaded, then the ~ console is ready and can
    // be printed to

    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    
    

    return true;
}