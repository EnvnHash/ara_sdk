
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

    Conn(Server *m);

    bool        Process(SOCKET sock, const sockaddr_in *addr);
    bool        parseContent();
    int         parseLine(int& ret, Content* content);

    uint32_t    getContentLength();
    std::string getHdrValue(const std::string &name);
    std::string getHdrValue(const char *name);
    std::string getMethod() { return m_request.size() >= 1 ? m_request[0] : ""; }
    std::string getURI() { return m_request.size() >= 2 ? m_request[1] : ""; }
    std::string getHTTP() { return m_request.size() >= 3 ? m_request[2] : ""; }

    static std::string getQPar(const std::string& src, const std::string& parname);
    std::string getFromAddr() { return m_fromAddr; }

    [[nodiscard]] int32_t  SendString(const std::string &s) const;
    [[nodiscard]] int32_t  SendError(int code) const;
    [[nodiscard]] int32_t  SendOK() const;
    [[nodiscard]] int32_t  SendResponse(int code, const std::string &text_body) const;
    void ParseURI(std::string &filestr, std::vector<std::string> &parameter_list);
    int  sRecv(void *dest, int count);

    Server*         m_server = nullptr;
    type_content    m_Content;

private:
    RawContent      m_rawContent;
    type_hdrmap     m_hdrMap;
    type_request    m_request;
    std::string     m_fromAddr;
    SOCKET          m_socket{};
    int64_t         m_recvByteCount = 0;

    void incRecvCount(int count);
};

}