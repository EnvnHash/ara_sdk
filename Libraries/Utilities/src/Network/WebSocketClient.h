//
// Created by user on 08.08.2021.
//

#pragma once

#include <Conditional.h>
#include <Network/Network_Common.h>
#include <util_common.h>

#include "easywsclient.hpp"
#include <json/json.hpp>

namespace ara {

class WebSocketClient {
public:
    WebSocketClient()          = default;
    virtual ~WebSocketClient() = default;
    void init();
    void stop();
    void send(nlohmann::json jObj) { m_sendQueue.emplace_back(jObj.dump()); }
    void setUrl(std::string url) { m_url = url; }
    void setRecvCb(std::function<void(nlohmann::json)> f) { m_recvCb = f; }
    bool isInited() { return m_inited; }

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
