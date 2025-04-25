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
#include <ThreadedTasks/Cycler.h>

namespace ara {
class UDPSender : public Cycler {
public:
    bool StartBroadcast(int port, int period_ms, std::function<int(void *, int)> const &f);

private:
    // return data_size, max_size has the max possible datasize data is passed and copied into a safe buffer inside the cycle
    // m_OnEmit is called if virtual OnEmit returns 0
    std::function<int(void*, int)> m_OnEmit;

protected:
    SOCKET      m_Socket    = 0;
    int         m_Port      = 0;
    int         m_Period_ms = 1000;
    sockaddr_in m_SockAddr = {};

    bool OnCycleStop() override;
    bool OnCycle() override;

    virtual int OnEmit(void *buff, int max_size) {
        return 0;
    }  // to return data_size, data is passed and copied into a safe m_buffer inside the cycle
};
}  // namespace ara