#include "Network/TCPLineCmdListener.h"

#include "Network/Network_Common.h"

namespace ara {

bool TCPLineCmdListener::StartListen(int                                                                   port,
                                     std::function<void(SOCKET s, std::string &cmd, sockaddr_in *)> const &f) {
    if (m_Socket) return false;

    if (port < 0) return false;

    m_OnReceive = f;
    m_Port      = port;

    if ((m_Socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        m_Socket = 0;
        return false;
    }

    sockaddr_in sa;

    sa.sin_family      = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port        = htons(m_Port);

    if (bind(m_Socket, (struct sockaddr *)&sa, sizeof(sa))) {
        closesocket(m_Socket);
        m_Socket = 0;
        return false;
    }

    int max_conn = 16;

    if (listen(m_Socket, max_conn)) {
        closesocket(m_Socket);
        m_Socket = 0;

        return false;
    }

    return Start();
}

bool TCPLineCmdListener::OnCycleStop() {
    if (!m_Socket) return false;

    closesocket(m_Socket);

    return true;
}

bool TCPLineCmdListener::OnCycle() {
    sockaddr_in sf;
    int         sfsize = sizeof(sf);
    SOCKET      ns;
    sockaddr_in addr;
    int         addr_size = sizeof(sockaddr_in);

    do {
        if ((ns = accept(m_Socket, (sockaddr *)&addr, (socklen_t *)&addr_size)) != INVALID_SOCKET) {
            std::thread(&TCPLineCmdListener::ProcessConnection, this, ns, addr).detach();
        }
    } while (ns != INVALID_SOCKET && (m_CycleState == CycleState::running));

    closesocket(m_Socket);
    m_Socket = 0;
    return true;
}

void TCPLineCmdListener::ProcessConnection(SOCKET s, sockaddr_in sa) {
    std::string cmd;
    int         ret;
    char        ch, lch = 0;

    do {
        if ((ret = recv(s, &ch, 1, 0)) > 0) {
            if (ch >= 32)
                cmd += ch;
            else {
                if (ch == '\n' && lch == '\r') {
                    ProcessReceive(s, cmd, &sa);

                    lch = 0;
                    cmd.clear();

                } else
                    lch = ch;
            }
        }

    } while (ret > 0);

    closesocket(s);
}

void TCPLineCmdListener::ProcessReceive(SOCKET s, std::string &cmd, sockaddr_in *sa) {
    if (m_OnReceive) {
        m_OnReceive(s, cmd, sa);
    }

    OnReceive(s, cmd, sa);
    closesocket(s);
}
}  // namespace ara