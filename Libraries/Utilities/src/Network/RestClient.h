//
// Created by user on 08.08.2021.
//

#pragma once

#ifdef ARA_USE_CURL

#include <Conditional.h>
#include <curl/curl.h>
#include <util_common.h>

#include <threadpool/thread_pool.hpp>

#include <json/json.hpp>

namespace ara {

class RestCall {
public:
    std::string                         url;
    restCallType                        callType;
    std::function<void(nlohmann::json)> cb        = nullptr;
    bool                                hasParams = false;
    nlohmann::json                      params    = nullptr;
    std::string                        *dstBuffer = nullptr;
    std::string                         dstPath;
    bool                                processing = false;
    bool                                done       = false;
};

class RestCallCbData {
public:
    RestCallCbData(RestCall &rc) {
        done      = &rc.done;
        callType  = rc.callType;
        cb        = std::move(rc.cb);
        url       = std::string(rc.url);
        hasParams = rc.hasParams;
        params    = std::move(rc.params);
        dstBuffer = rc.dstBuffer;
        dstPath   = std::move(rc.dstPath);
    }

    bool                                hasParams = false;
    bool                                *done      = nullptr;
    restCallType                        callType;
    std::function<void(nlohmann::json)> cb = nullptr;
    std::string                         url;
    nlohmann::json                      params    = nullptr;
    std::string                         *dstBuffer = nullptr;
    std::string                         dstPath;
};

class RestCallResult {
public:
    bool                                valid = false;
    std::function<void(nlohmann::json)> cb    = nullptr;
    nlohmann::json                      msg;
};

class RestClient {
public:
    RestClient();
    virtual ~RestClient() = default;
    void           init();
    RestCallResult procCall(const RestCallCbData &rc);
    RestCallResult procDownloadBuffer(const RestCallCbData &rc);
    RestCallResult procDownloadFile(const RestCallCbData &rc);

    static int    curlWriter(char *data, size_t size, size_t nmemb, std::string *buffer_in);
    static size_t curlWriteFile(void *ptr, size_t size, size_t nmemb, FILE *stream);
    static int    progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal,
                                    curl_off_t ulnow);

    void stop();
    void downloadToBuffer(std::string url, std::string *dstBuffer, std::function<void(nlohmann::json)> cb);
    void downloadToFile(std::string url, std::string dstPath, std::function<void(nlohmann::json)> cb);
    void post(std::string url, std::function<void(nlohmann::json)> cb);
    void post(nlohmann::json &&params, std::string url, std::function<void(nlohmann::json)> cb);
    void get(std::string url, std::function<void(nlohmann::json)> cb);
    void get(nlohmann::json &&params, std::string url, std::function<void(nlohmann::json)> cb);
    void call(nlohmann::json &&params, std::string url, restCallType callType, std::function<void(nlohmann::json)> cb);
    void call(std::string url, restCallType callType, std::function<void(nlohmann::json)> cb);
    void call(std::string url, std::string *dstBuffer, restCallType callType, std::function<void(nlohmann::json)> cb);
    void call(std::string url, std::string dstPath, restCallType callType, std::function<void(nlohmann::json)> cb);

    void         pause() { m_run = false; }
    bool         isInited() { return m_inited; }
    void         setAuthToken(std::string str) { m_authToken = std::move(str); }
    std::string &getAuthToken() { return m_authToken; }
    void         setDownFolder(std::filesystem::path &p) { m_downPath = p; }

protected:
    std::atomic<bool> m_run    = true;
    bool              m_inited = true;
    thread_pool       m_ThreadPool;
    std::thread       m_procQueueThread;

    std::string           m_authToken;
    std::filesystem::path m_downPath;

    Conditional         m_procQueueCond;
    std::list<RestCall> m_callQueue;
    Conditional         m_exitCond;
    Conditional         m_loopEnd;
    std::mutex          m_procCallMtx;
};

}  // namespace ara
#endif