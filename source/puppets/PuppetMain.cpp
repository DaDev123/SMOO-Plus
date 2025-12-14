#include "server/Client.hpp"
#include "logger.hpp"
#include "main.hpp"

al::LiveActor *createPuppetActorFromFactory(al::ActorInitInfo const &rootInitInfo, 
                                           al::PlacementInfo const &rootPlacementInfo, 
                                           bool isDebug) {
    al::ActorInitInfo actorInitInfo = al::ActorInitInfo();
    actorInitInfo.initViewIdSelf(&rootPlacementInfo, rootInitInfo);

    al::createActor createActor = actorInitInfo.mActorFactory->getCreator("PuppetActor");
    
    if(createActor) {
        PuppetActor *newActor = (PuppetActor*)createActor("PuppetActor");

        if(!isDebug) {
            if(Client::tryAddPuppet(newActor)) {
                PuppetInfo *curInfo = Client::getLatestInfo();
                if(!curInfo) {
                    Logger::log("[Factory] ERROR: Puppet Info is Null!\n");
                    delete newActor;
                    return nullptr;
                } else {
                    Logger::log("[Factory] Creating puppet for player: %s\n", curInfo->puppetName);

                    // set puppet info first before calling init so we can get costume info from the info
                    newActor->initOnline(curInfo); 
                    newActor->init(actorInitInfo);
                    
                    Logger::log("[Factory] Puppet initialized successfully for %s\n", curInfo->puppetName);
                }
            } else {
                Logger::log("[Factory] ERROR: Failed to add puppet to client\n");
                delete newActor;
                return nullptr;
            }
        } else {
            Logger::log("[Factory] Creating Debug/Test Puppet\n");

            PuppetInfo* debugInfo = Client::getDebugPuppetInfo();
            if (!debugInfo) {
                Logger::log("[Factory] ERROR: Debug puppet info is null!\n");
                delete newActor;
                return nullptr;
            }

            newActor->initOnline(debugInfo); 
            newActor->init(actorInitInfo);
            newActor->mIsDebug = true;
            newActor->makeActorAlive();

            if (Client::tryAddDebugPuppet(newActor)) {
                Logger::log("[Factory] Debug Puppet Created Successfully!\n");
            } else {
                Logger::log("[Factory] WARNING: Failed to register debug puppet\n");
            }
        }

        return newActor;
    } else {
        Logger::log("[Factory] ERROR: Could not find PuppetActor creator in factory!\n");
        return nullptr;
    }
}

// Hooks

void initPuppetActors(al::Scene *scene, al::ActorInitInfo const &rootInfo, char const *listName) {
    Logger::log("[Init] Starting puppet actor initialization\n");

    al::StageInfo *stageInfo = al::getStageInfoMap(scene, 0);

    int placementCount = 0;
    al::PlacementInfo rootPlacement = al::PlacementInfo();
    
    // These functions return void, not bool - just call them
    al::tryGetPlacementInfoAndCount(&rootPlacement, &placementCount, stageInfo, "PlayerList");
    
    if (placementCount == 0) {
        Logger::log("[Init] WARNING: PlayerList placement count is 0\n");
    }

    // check if default placement count for PlayerList is greater than 0 
    // (so we can use the placement info of the player to init our custom puppet actors)
    if(placementCount > 0) {
        al::PlacementInfo playerPlacement = al::PlacementInfo();
        
        // This function also returns void
        al::getPlacementInfoByIndex(&playerPlacement, rootPlacement, 0);

        int maxPlayers = Client::getMaxPlayerCount();
        Logger::log("[Init] Creating %d puppet actors\n", maxPlayers);

        int successCount = 0;
        for (size_t i = 0; i < maxPlayers; i++)
        {
            al::LiveActor* puppet = createPuppetActorFromFactory(rootInfo, playerPlacement, false);
            if (puppet) {
                successCount++;
            } else {
                Logger::log("[Init] WARNING: Failed to create puppet %zu/%d\n", i+1, maxPlayers);
            }
        }
        
        Logger::log("[Init] Successfully created %d/%d puppets\n", successCount, maxPlayers);
        
        // create a debug puppet for testing purposes (uncomment if needed)
        // Logger::log("[Init] Creating debug puppet\n");
        // al::LiveActor* debugPuppet = createPuppetActorFromFactory(rootInfo, playerPlacement, true);
        // if (!debugPuppet) {
        //     Logger::log("[Init] WARNING: Failed to create debug puppet\n");
        // }
    } else {
        Logger::log("[Init] ERROR: No player placements found (count = 0)\n");
    }

    Logger::log("[Init] Initializing placement object map for %s\n", listName);
    // run init for ObjectList after we init our puppet actors 
    al::initPlacementObjectMap(scene, rootInfo, listName);
    
    Logger::log("[Init] Puppet actor initialization complete\n");
}