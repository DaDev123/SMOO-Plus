#pragma once

#include <string>

class PlayerActorHakoniwa;

void handleNoclip(PlayerActorHakoniwa* hakoniwa, bool gNoclip, bool isYukimaru);
void handleInfiniteCapBounce(PlayerActorHakoniwa* playerBase, bool gInfiniteCapBounce);
void giveLifeUpHeart(PlayerActorHakoniwa* hakoniwa);
void setOutfit(PlayerActorHakoniwa* hakoniwa, std::string body, std::string cap);

// External function declarations
extern void sendCostumeInfPacket(const char* body, const char* cap);