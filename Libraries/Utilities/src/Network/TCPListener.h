//
// Created by user on 22.06.2020.
//

#pragma once

#include "Network/Network_Common.h"

namespace ara {

class TCPListener : public Cycler {
public:
    bool StartListen(int port, std::function<void(SOCKET sock, sockaddr_in *)> const &f);

private:
    std::function<void(SOCKET s, sockaddr_in *)> m_OnConnect;
    void ProcessConnection(SOCKET s, sockaddr_in sa);

protected:
    SOCKET m_Socket = 0;
    int    m_Port   = 0;

    virtual bool OnCycleStop();
    virtual bool OnCycle();
    virtual bool OnConnect(SOCKET s, sockaddr_in *) { return true; }
};

}  // namespace ara
