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
