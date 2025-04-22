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

#ifdef ARA_USE_CURL

#include "RestClient.h"

using namespace std;
using json = nlohmann::json;

namespace ara {

RestClient::RestClient() = default;

void RestClient::init() {
    m_procQueueThread = std::thread([&] {
        while (m_run) {
            m_procQueueCond.wait(0);

            if (!m_run) break;

            {
                // process the call queue
                std::unique_lock<mutex> l(m_procCallMtx);
                if (!m_run) break;

                for (auto it = m_callQueue.begin(); it != m_callQueue.end(); ++it) {
                    if (!it->processing) {
                        it->processing = true;

                        m_ThreadPool.push_task([this, it] {
                            if (!m_run) return;

                            RestCallResult res;
                            RestCallCbData call(*it);  // somehow curl kills the "it"-reference on android armv8

                            if (it->callType == restCallType::post || it->callType == restCallType::get) {
                                res = procCall(call);
                            } else if (it->callType == restCallType::downloadBuffer) {
                                res = procDownloadBuffer(call);
                            } else if (it->callType == restCallType::downloadFile) {
                                res = procDownloadFile(call);
                            }

                            if (res.valid) {
                                res.cb(res.msg);
                            }

                            (*call.done) = true;
                        });
                    }
                }

                for (auto it = m_callQueue.begin(); it != m_callQueue.end();) {
                    if (it->done) {
                        it = m_callQueue.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
        }

        m_loopEnd.notify();
    });

    m_procQueueThread.detach();
    m_inited = true;
}

RestCallResult RestClient::procCall(const RestCallCbData &rc) {
    if (!m_run) return {};

    std::string params;
    auto        curl = curl_easy_init();

    if (auto rcParams = rc.hasParams ? &rc.params : nullptr) {
        params = rcParams->dump();
    }

    struct curl_slist *headers = nullptr;
    headers                    = curl_slist_append(headers, "Accept: application/json");
    headers                    = curl_slist_append(headers, "Content-Type: application/json");
    headers                    = curl_slist_append(headers, "charset: utf-8");

    if (!m_authToken.empty() && rc.callType == restCallType::post) {
        string hstr = "Authorization: " + m_authToken;
        headers     = curl_slist_append(headers, hstr.c_str());
    }

    if (curl) {
        // curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_URL, rc.url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &RestClient::curlWriter);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, &RestClient::progress_callback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, static_cast<void *>(this));
        curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        if (rc.callType == restCallType::post) curl_easy_setopt(curl, CURLOPT_POST, 1L);
        if (rc.callType == restCallType::get) curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

        // strange things are happening on armv8 android in case httpData is a
        // simple std::string
        auto *httpData = new std::string();
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, static_cast<void *>(httpData));

        if (rc.callType == restCallType::post) {
            if (!params.empty()) {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, params.c_str());
            } else {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);
            }
        }

        // Perform the request, res will get the return code
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            LOGE << "Error: Curl request failed \n";
        }

        int httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (httpCode == 200 && !httpData->empty()) {
            // Response looks good - done using Curl now.  Try to parse the
            // results
            if (m_run && rc.cb) {
                auto        msg = json::parse(*httpData);
                delete httpData;

                if (!msg.empty()) {
                    return RestCallResult{true, rc.cb, msg};
                } else {
                    std::string err;
                    LOGE << err;
                }
            }
        } else {
            if (!httpData->empty()) {
                LOGE << "nmsApiCall request failed " << rc.url << " no data received";
            } else {
                LOGE << "nmsApiCall request failed " << rc.url << " " << httpData;
            }

            delete httpData;
        }
    } else {
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    return {};
}

RestCallResult RestClient::procDownloadBuffer(const RestCallCbData &rc) {
    if (!m_run) return {};

    auto curl = curl_easy_init();

    if (curl && rc.dstBuffer) {
        // curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_URL, rc.url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &RestClient::curlWriter);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, &RestClient::progress_callback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, static_cast<void *>(this));
        curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, rc.dstBuffer);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        // Perform the request, res will get the return code
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            LOGE << "Error: Curl request failed \n";
        }

        int httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        curl_easy_cleanup(curl);

        if (httpCode == 200) {
            // Response looks good - done using Curl now.  Try to parse the results
            if (rc.cb && m_run) {
                rc.cb(json());
            }
        } else {
            LOGE << "nmsApiCall request failed " << rc.url;
        }
    } else {
        curl_easy_cleanup(curl);
    }

    return {};
}

RestCallResult RestClient::procDownloadFile(const RestCallCbData &rc) {
    if (!m_run) return {};

    auto curl = curl_easy_init();

    if (curl && !rc.dstPath.empty()) {
        // curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_URL, rc.url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &RestClient::curlWriteFile);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, &RestClient::progress_callback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, static_cast<void *>(this));
        curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 8L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        if (FILE *fp = fopen(rc.dstPath.c_str(), "wb")) {
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

            // Perform the request, res will get the return code
            CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                LOGE << "Error: Curl request failed \n";
            }

            fclose(fp);
            int httpCode = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
            curl_easy_cleanup(curl);

            if (httpCode == 200) {
                // Response looks good - Try to parse the results
                if (rc.cb != nullptr && m_run) {
                    rc.cb(json());
                }
            } else {
                LOGE << "nmsApiCall request failed " << rc.url;
            }
        } else {
            LOGE << "RestClient::procDownloadFile could not open " << rc.dstPath;
        }
    } else {
        curl_easy_cleanup(curl);
    }

    return {};
}

int RestClient::curlWriter(const char *data, size_t size, size_t nmemb, std::string *buffer_in) {
    if (!buffer_in) {
        return 0;
    }

    size_t realsize = size * nmemb;
    buffer_in->append(data, realsize);

    return static_cast<int>(realsize);
}

size_t RestClient::curlWriteFile(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

int RestClient::progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    return static_cast<RestClient *>(clientp)->m_run ? CURL_PROGRESSFUNC_CONTINUE : -1;
}

void RestClient::downloadToBuffer(std::string url, std::string *dstBuffer, std::function<void(json)> cb) {
    if (!m_run) return;
    call(std::move(url), dstBuffer, restCallType::downloadBuffer, std::move(cb));
}

void RestClient::downloadToFile(std::string url, std::string dstPath, std::function<void(json)> cb) {
    if (!m_run) return;
    call(std::move(url), std::move(dstPath), restCallType::downloadFile, std::move(cb));
}

void RestClient::post(std::string url, std::function<void(json)> cb) {
    if (!m_run) return;
    call(std::move(url), restCallType::post, std::move(cb));
}

void RestClient::post(json &&params, std::string url, std::function<void(json)> cb) {
    if (!m_run) return;
    call(std::move(params), std::move(url), restCallType::post, std::move(cb));
}

void RestClient::get(std::string url, std::function<void(json)> cb) {
    if (!m_run) return;
    call(std::move(url), restCallType::get, std::move(cb));
}

void RestClient::get(json &&params, std::string url, std::function<void(json)> cb) {
    if (!m_run) return;
    call(std::move(params), std::move(url), restCallType::get, std::move(cb));
}

void RestClient::call(json &&params, std::string url, restCallType callType, std::function<void(json)> cb) {
    std::unique_lock<std::mutex> l(m_procCallMtx);
    m_callQueue.push_back(RestCall{std::move(url), callType, std::move(cb), true, std::move(params)});
    m_procQueueCond.notify();
}

void RestClient::call(std::string url, restCallType callType, std::function<void(json)> cb) {
    std::unique_lock<std::mutex> l(m_procCallMtx);
    if (std::find_if(m_callQueue.begin(), m_callQueue.end(), [url](const RestCall &rc) { return rc.url == url; }) ==
        m_callQueue.end())
        m_callQueue.emplace_back(RestCall{std::move(url), callType, std::move(cb)});

    m_procQueueCond.notify();
}

void RestClient::call(std::string url, std::string *dstBuffer, restCallType callType, std::function<void(json)> cb) {
    std::unique_lock<std::mutex> l(m_procCallMtx);
    if (std::find_if(m_callQueue.begin(), m_callQueue.end(), [url](const RestCall &rc) { return rc.url == url; }) ==
        m_callQueue.end())
        m_callQueue.emplace_back(RestCall{std::move(url), callType, std::move(cb), false, nullptr, dstBuffer});

    m_procQueueCond.notify();
}

void RestClient::call(std::string url, std::string dstPath, restCallType callType, std::function<void(json)> cb) {
    std::unique_lock<std::mutex> l(m_procCallMtx);
    if (std::find_if(m_callQueue.begin(), m_callQueue.end(), [url](const RestCall &rc) { return rc.url == url; }) ==
        m_callQueue.end())
        m_callQueue.emplace_back(RestCall{std::move(url), callType, std::move(cb), false, nullptr, nullptr, std::move(dstPath)});

    m_procQueueCond.notify();
}

void RestClient::stop() {
    m_run = false;
    m_procQueueCond.notify();
    m_loopEnd.wait(0);
}

}  // namespace ara

#endif