#include "layouts/ConnectionStatus.h"

#include "al/Library/Layout/LayoutActionFunction.h"
#include "al/Library/Layout/LayoutActorUtil.h"
#include "al/Library/Nerve/NerveUtil.h"

#include "server/Client.hpp"

ConnectionStatus* ConnectionStatus::sInstance = nullptr;

ConnectionStatus::ConnectionStatus(const char* name, const al::LayoutInitInfo& initInfo)
    : al::LayoutActor(name) {
    al::initLayoutActor(this, initInfo, "ConnectionStatus", 0);

    al::setPaneStringFormat(this, "TxtStatus", "10/10");
    al::hidePane(this, "DotRed");
    al::hidePane(this, "DotGreen");

    initNerve(&NrvConnectionStatus.End, 0);

    kill();
}

void ConnectionStatus::appear() {
    al::startAction(this, "Appear", 0);
    al::setNerve(this, &NrvConnectionStatus.Appear);
    al::LayoutActor::appear();
}

bool ConnectionStatus::tryEnd() {
    if (!al::isNerve(this, &NrvConnectionStatus.End)) {
        al::setNerve(this, &NrvConnectionStatus.End);
        return true;
    }
    return false;
}

bool ConnectionStatus::tryStart() {
    if (!al::isNerve(this, &NrvConnectionStatus.Wait) && !al::isNerve(this, &NrvConnectionStatus.Appear)) {
        appear();
        return true;
    }
    return false;
}

void ConnectionStatus::exeAppear() {
    if (al::isActionEnd(this, 0)) {
        al::setNerve(this, &NrvConnectionStatus.Wait);
    }
}

void ConnectionStatus::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", 0);
    }
    Client::instance()->mSocket->isConnected() ? showOnline() : showOffline();
}

void ConnectionStatus::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", 0);
    }

    if (al::isActionEnd(this, 0)) {
        kill();
    }
}

void ConnectionStatus::showOnline() {
    al::setPaneStringFormat(this, "TxtStatus", "%d/%d", Client::getConnectCount() + 1,
                            Client::getMaxPlayerCount());
    al::showPane(this, "DotGreen");
    al::hidePane(this, "DotRed");
}
void ConnectionStatus::showOffline() {
    al::setPaneString(this, "TxtStatus", u"Offline");
    al::showPane(this, "DotRed");
    al::hidePane(this, "DotGreen");
}
