#pragma once

class DarknessTwist {
public:
    static bool sDarknessEnabled;

    static bool isDarknessEnabled() { return sDarknessEnabled; }
    static void toggleDarkness();

    static void initHooks();
};