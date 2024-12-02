#pragma once

#include "types.h"

// enum for defining game mode types
enum GameMode : s8 {
    NONE        = -1, // == 15 in u4
    LEGACY      =  0,
    HIDEANDSEEK =  1,
    SARDINE     =  2,
    FREEZETAG   =  3,
    /**
     * Don't use values 14 or higher before refactoring the GameModeInf packet.
     * This is necessary because currently there are only 4 bits in the packet for the game mode.
     * 15 is there for -1 (None)
     * and 14 could then indicate that it continues to count up in an extra automatically added byte.
     */
};
