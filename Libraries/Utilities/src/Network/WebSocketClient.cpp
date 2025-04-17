//
// Created by user on 08.08.2021.
//

#include "WebSocketClient.h"

using easywsclient::WebSocket;
using namespace std;
using json = nlohmann::json;

namespace ara {

void WebSocketClient::init() {
    m_thread = thread([this] {

#ifdef _WIN32
        int     rc;
        WSADATA wsaData;

        rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (rc) {
            printf("WSAStartup Failed.\n");
            return 1;
        }
#endif
        if (m_url.empty()) {
            LOGE << "WebSocketClient::init Error: no url set";
            return 0;
        }

        m_ws = WebSocket::from_url(m_url);
        if (m_ws) {
            LOG << "WebSocket is starting " << m_url;

            while (m_run && m_ws->getReadyState() != WebSocket::CLOSED) {
                m_ws->poll();

                // process the send queue
                {
                    std::unique_lock<mutex> l(m_sendQueueMtx);
                    for (auto &it : m_sendQueue) m_ws->send(it);
                    m_sendQueue.clear();
                }

                m_ws->dispatch([this](const std::string &message) {
                    m_message = json::parse(message);

                    if (m_message.empty()) {
                        LOGE << m_err;
                    } else {
                        if (m_recvCb) m_recvCb(m_message);
                    }
                });

                std::this_thread::sleep_for(500us);
            }
        } else {
            LOGE << "could not connect to " << m_url;
        }

#ifdef _WIN32
        WSACleanup();
#endif
        m_exitCond.notify();
        return 0;
    });

    m_thread.detach();
    m_inited = true;
}

void WebSocketClient::stop() {
    m_run = false;
    if (m_ws) m_ws->close();
    m_exitCond.wait();
}

}  // namespace ara