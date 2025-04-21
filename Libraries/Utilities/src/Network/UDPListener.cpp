#include "Network/UDPListener.h"

#include "Network_Common.h"

namespace ara {

bool UDPListener::StartListen(int port, std::function<void(char *, int, sockaddr_in *)> const &f) {
    if (m_Socket) return false;

    if (port < 0) return false;

    m_OnReceive = f;
    m_Port      = port;

    if ((m_Socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        m_Socket = 0;
        return false;
    }

    sockaddr_in sa;
    int         reuseaddr = 1;

    sa.sin_family      = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port        = htons(m_Port);

    setsockopt(m_Socket, SOL_SOCKET, SO_REUSEADDR, (char *)&reuseaddr, sizeof(reuseaddr));

    if (bind(m_Socket, (struct sockaddr *)&sa, sizeof(sa))) {
        closesocket(m_Socket);
        m_Socket = 0;
        return false;
    }

    return Start();
}

bool UDPListener::OnCycleStop() {
    if (!m_Socket) return false;

    closesocket(m_Socket);

    return true;
}

bool UDPListener::OnCycle() {
    int         ret;
    char        buff[520];
    sockaddr_in sf;
    int         sfsize = sizeof(sf);

    do {
        if ((ret = recvfrom(m_Socket, (char *)buff, 512, 0, (sockaddr *)&sf, (socklen_t *)&sfsize)) > 0) {
            buff[ret] = 0;

            if (m_OnReceive) m_OnReceive(buff, ret, &sf);

            OnReceive(buff, ret, &sf);
        }

    } while (ret > 0 && (m_CycleState == CycleState::running));

    closesocket(m_Socket);
    m_Socket = 0;

    return true;
}

}
