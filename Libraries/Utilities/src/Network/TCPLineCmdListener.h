//
// Created by user on 22.06.2020.
//

#pragma once

#include "Network/Network_Common.h"

namespace ara {

class TCPLineCmdListener : public Cycler {
public:
    bool StartListen(int port, std::function<void(SOCKET sock, std::string &cmd, sockaddr_in *)> const &f);

private:
    std::function<void(SOCKET s, std::string &cmd, sockaddr_in *)> m_OnReceive;

    void ProcessConnection(SOCKET s, sockaddr_in sa);
    void ProcessReceive(SOCKET s, std::string &cmd, sockaddr_in *);

protected:
    SOCKET m_Socket = 0;
    int    m_Port   = 0;

    virtual bool OnCycleStop();

    virtual bool OnCycle();

    virtual bool OnReceive(SOCKET s, std::string &cmd, sockaddr_in *) { return true; }
};

}  // namespace ara
