// GameStateFlags.h
#pragma once

namespace GameStateFlags {
    // This flag indicates whether the player wants their time recorded
    // Set in LevelSelection when player chooses Y/N in the record prompt
    inline bool shouldRecordTime = false;

    // Optional: Current level being played (set when loading level)
    inline std::string currentLevelName = "";

    // Reset all flags (call when leaving gameplay)
    inline void Reset() {
        shouldRecordTime = false;
        currentLevelName.clear();
    }
}