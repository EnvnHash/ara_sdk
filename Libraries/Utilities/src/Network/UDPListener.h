//
// Created by user on 22.06.2020.
//

#pragma once

#include <functional>

#include "Network_Common.h"
#include "ThreadedTasks/Cycler.h"

namespace ara {

class UDPListener : public Cycler {
public:
    bool StartListen(int port, std::function<void(char *, int, sockaddr_in *)> const &f);

private:
    std::function<void(char *, int, sockaddr_in *)> m_OnReceive;

protected:
    SOCKET m_Socket = 0;
    int    m_Port   = 0;

    bool         OnCycleStop() override;
    bool         OnCycle() override;
    virtual bool OnReceive(char *, int, sockaddr_in *) { return true; }
};
}  // namespace ara