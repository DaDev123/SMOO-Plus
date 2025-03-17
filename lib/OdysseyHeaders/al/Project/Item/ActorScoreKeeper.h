#pragma once

#include <basis/seadTypes.h>

namespace al {
class ByamlIter;

class ActorScoreKeeper {
public:
    struct Entry {
        const char* factorName;
        const char* categoryName;
    };

    ActorScoreKeeper();

    void init(const ByamlIter& iter);
    void getCategoryName();  // unknown return type
    const char* tryGetCategoryName(const char* a1);

public:
    inline void allocArray();
    inline void putEntry(s32 index, const ByamlIter& iter);

    Entry* mArray = nullptr;
    s32 mSize = 0;
};
}  // namespace al
