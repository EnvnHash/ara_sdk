//
// Created by hahne on 22.04.2025.
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

#include <Network/MicroHTTPServer.h>
#include <Network/MicroHttpConnection.h>

namespace ara::microhttp {

bool Server::Start(int port, Server::c_cb const &f) {
    m_OnConnection = f;
    TCPListener::StartListen(port, nullptr);
    return TCPListener::Start();
}

bool Server::OnConnect(SOCKET sock, sockaddr_in *addr) {
    Conn conn(this);

    if (!conn.Process(sock, addr)) {
        return false;
    }

    if (m_OnConnection) {
        m_OnConnection(conn);
    }

    return true;
}

void Server::incRecvCount(int count) {
    std::unique_lock<std::mutex> lock(i_CMutex);
    i_RecvByteCount += count;
}

// Global

void utilParse(std::string &name, std::string &val, std::string &src_string, char delimiter) {
    size_t aux;
    if ((aux = src_string.find(delimiter)) != std::string::npos) {
        name = src_string.substr(0, aux);
        val  = src_string.substr(name.length() + 1);

    } else {
        name = src_string;
    }
}

}
