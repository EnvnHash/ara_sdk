#include "Network/TCPListener.h"

#include <iostream>

#include "Network/Network_Common.h"

using namespace std;

namespace ara {

bool TCPListener::StartListen(int port, std::function<void(SOCKET s, sockaddr_in *)> const &f) {
    if (m_Socket) return false;

    if (port < 0) return false;

    m_OnConnect = f;
    m_Port      = port;

    if ((m_Socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << "TCPListener> Cannot create socket" << endl;

        m_Socket = 0;

        return false;
    }

    sockaddr_in sa;

    sa.sin_family      = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port        = htons(m_Port);

    if (::bind(m_Socket, (struct sockaddr *)&sa, sizeof(sa))) {
        cerr << "TCPListener> Cannot bind to " << m_Port << endl;

        closesocket(m_Socket);
        m_Socket = 0;

        return false;
    }

    int max_conn = 16;

    if (listen(m_Socket, max_conn)) {
        cerr << "TCPListener> Cannot listen on " << m_Port << endl;

        closesocket(m_Socket);
        m_Socket = 0;

        return false;
    }

    return Start();
}

bool TCPListener::OnCycleStop() {
    if (!m_Socket) return false;

    closesocket(m_Socket);

    return true;
}

bool TCPListener::OnCycle() {
    sockaddr_in sf;
    int         sfsize = sizeof(sf);
    SOCKET      ns;
    sockaddr_in addr;
    int         addr_size = sizeof(sockaddr_in);

    do {
        if ((ns = accept(m_Socket, (sockaddr *)&addr, (socklen_t *)&addr_size)) != INVALID_SOCKET) {
            std::thread(&TCPListener::ProcessConnection, this, ns, addr).detach();
        }

    } while (ns != INVALID_SOCKET && (m_CycleState == CycleState::running));

    closesocket(m_Socket);

    m_Socket = 0;

    return true;
}

void TCPListener::ProcessConnection(SOCKET s, sockaddr_in sa) {
    if (m_OnConnect) m_OnConnect(s, &sa);

    OnConnect(s, &sa);

    closesocket(s);
}

}  // namespace ara