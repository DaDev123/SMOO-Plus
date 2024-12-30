#pragma once

namespace al {
class WipeSimple;

class WipeHolder {
public:
    WipeHolder(int);
    void registerWipe(const char*, al::WipeSimple*);
    void startClose(const char*, int);
    void findWipe(const char*) const;
    void startCloseByInfo(const char*);
    void findInfo(const char*) const;
    bool tryStartClose(const char*, int);
    bool tryStartCloseByInfo(const char*);
    void startCloseEnd(const char*);
    void startOpen(int);
    void isExistInfo(const char*) const;
    bool tryFindInfo(const char*) const;
    int getCloseTimeByInfo(const char*) const;
    bool isCloseEnd() const;
    bool isOpenEnd() const;
    bool isCloseWipe(const char*) const;
};
}