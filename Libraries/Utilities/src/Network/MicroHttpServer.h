
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


#pragma once

#include <Network/NetworkCommon.h>
#include <Network/TCPListener.h>

namespace ara::microhttp {

class Server;

class Conn;

class Server : public TCPListener {
public:
    using c_cb = std::function<void(Conn &conn)>;

    bool Start(int port, Server::c_cb const &f);
    void incRecvCount(int count);

    int64_t getRecvCount() { return i_RecvByteCount; }

private:
    std::mutex i_CMutex;
    c_cb m_OnConnection = nullptr;
    int64_t i_RecvByteCount = 0;

protected:
    virtual bool OnConnect(SOCKET sock, sockaddr_in *);
};

void utilParse(std::string &name, std::string &val, std::string &src_string, char delimiter = '=');

}  // namespace ara
