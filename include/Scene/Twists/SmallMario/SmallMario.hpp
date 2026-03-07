#pragma once

class SmallMario {
public:
    static bool sSmallMarioEnabled;

    static bool isSmallMarioEnabled() { return sSmallMarioEnabled; }
    static void toggleSmallMario();
};