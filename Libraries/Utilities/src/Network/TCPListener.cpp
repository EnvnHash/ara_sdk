
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <Network/TCPListener.h>
#include <Network/NetworkCommon.h>

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

    sockaddr_in sa{};

    sa.sin_family      = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port        = htons(m_Port);

    if (::bind(m_Socket, reinterpret_cast<struct sockaddr *>(&sa), sizeof(sa))) {
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
    if (!m_Socket) {
        return false;
    }

    closesocket(m_Socket);
    return true;
}

bool TCPListener::OnCycle() {
    [[maybe_unused]] sockaddr_in sf{};
    SOCKET      ns;
    sockaddr_in addr{};
    int         addr_size = sizeof(sockaddr_in);

    do {
        if ((ns = accept(m_Socket, reinterpret_cast<sockaddr *>(&addr), (socklen_t *)&addr_size)) != INVALID_SOCKET) {
            std::thread(&TCPListener::ProcessConnection, this, ns, addr).detach();
        }
    } while (ns != INVALID_SOCKET && (m_CycleState == CycleState::running));

    closesocket(m_Socket);
    m_Socket = 0;

    return true;
}

void TCPListener::ProcessConnection(SOCKET s, sockaddr_in sa) {
    if (m_OnConnect) {
        m_OnConnect(s, &sa);
    }

    OnConnect(s, &sa);
    closesocket(s);
}

}  // namespace ara