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

#include <Conditional.h>
#include <Network/NetworkCommon.h>
#include <easywsclient.hpp>
#include <json/json.hpp>

namespace ara {

class WebSocketClient {
public:
    WebSocketClient()          = default;
    virtual ~WebSocketClient() = default;
    void init();
    void stop();
    void send(const nlohmann::json &jObj) { m_sendQueue.emplace_back(jObj.dump()); }
    void setUrl(std::string url) { m_url = std::move(url); }
    void setRecvCb(std::function<void(nlohmann::json)> f) { m_recvCb = std::move(f); }
    bool isInited() const { return m_inited; }

protected:
    bool                                m_run    = true;
    bool                                m_inited = false;
    std::thread                         m_thread;
    std::mutex                          m_sendQueueMtx;
    std::string                         m_url;
    std::string                         m_err;
    nlohmann::json                      m_message;
    easywsclient::WebSocket::pointer    m_ws = nullptr;
    std::function<void(nlohmann::json)> m_recvCb;
    std::vector<std::string>            m_sendQueue;
    Conditional                         m_exitCond;
};

}  // namespace ara
