#include "Network/UDPSignaler.h"

#include "Network/Network_Common.h"

namespace ara {
bool UDPSignaler::StartBroadcast(int port, int period_ms, std::function<int(void *, int)> const &f) {
    if (m_Socket) {
        return false;
    }

    if (port < 0) {
        return false;
    }

    if (period_ms <= 0) {
        return false;
    }

    m_OnEmit    = f;
    m_Port      = port;
    m_Period_ms = period_ms;

    if ((m_Socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        m_Socket = 0;
        return false;
    }

    m_SockAddr.sin_family      = AF_INET;
    m_SockAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    m_SockAddr.sin_port        = htons(m_Port);

    int broadcast = 1;

    setsockopt(m_Socket, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<char *>(&broadcast), sizeof(broadcast));

    return Start();
}

bool UDPSignaler::OnCycleStop() {
    if (!m_Socket) {
        return false;
    }

    closesocket(m_Socket);
    return true;
}

bool UDPSignaler::OnCycle() {
    int  ret = 0;
    int  data_size;
    char buff[512];
    int  max_size = 512;

    do {
        if ((data_size = OnEmit(buff, max_size)) <= 0) {
            if (m_OnEmit) {
                data_size = m_OnEmit(buff, max_size);
            }
        }

        if (data_size > 0) {
            data_size = data_size < max_size ? data_size : max_size;
            if (data_size < 512) {
                buff[data_size] = 0;
            }

            if ((ret = sendto(m_Socket, buff, data_size, 0, reinterpret_cast<sockaddr *>(&m_SockAddr), sizeof(m_SockAddr))) > 0) {
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(m_Period_ms));

    } while ((ret > 0 || !data_size) && (m_CycleState == CycleState::running));

    closesocket(m_Socket);
    m_Socket = 0;
    return true;
}
}  // namespace ara
