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

#pragma once

#include <Network/NetworkCommon.h>
#include <Network/MicroHttpContent.h>
#include <Network/MicroHttpContentRaw.h>

namespace ara::microhttp {

class Server;

class Conn {
public:
    using type_hdrmap  = std::map<std::string, std::string>;
    using type_request = std::vector<std::string>;
    using type_content = std::vector<std::shared_ptr<Content>>;

    Server *m_Server = nullptr;

    type_content m_Content;

    Conn(Server *m);

    bool     Process(SOCKET sock, const sockaddr_in *addr);
    uint32_t getContentLength();

    std::string getHdrValue(const std::string &name);
    std::string getHdrValue(const char *name);
    std::string getMethod() { return m_Request.size() >= 1 ? m_Request[0] : ""; }
    std::string getURI() { return m_Request.size() >= 2 ? m_Request[1] : ""; }
    std::string getHTTP() { return m_Request.size() >= 3 ? m_Request[2] : ""; }

    static std::string getQPar(const std::string& src, const std::string& parname);
    std::string getFromAddr() { return m_FromAddr; }

    int  SendString(const std::string &s) const;
    int  SendError(int code) const;
    int  SendOK() const;
    int  SendResponse(int code, const std::string &text_body) const;
    void ParseURI(std::string &filestr, std::vector<std::string> &parameter_list);
    int  sRecv(void *dest, int count);

private:
    RawContent m_RawContent;
    type_hdrmap  m_HdrMap;
    type_request m_Request;
    std::string m_FromAddr;
    SOCKET m_Socket{};
    int64_t i_RecvByteCount = 0;

    void incRecvCount(int count);
};

}