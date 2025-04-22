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

#include <Network/MicroHttpConnection.h>
#include <Network/MicroHttpServer.h>

namespace ara::microhttp {

Conn::Conn(Server *m): m_socket(0), m_server(m) {
}

bool Conn::Process(SOCKET sock, const sockaddr_in *addr) {
    m_fromAddr.assign(inet_ntoa(addr->sin_addr));
    m_socket = sock;

    std::string line;
    int         ret;
    char        ch, lch = 0;
    std::size_t pos;

    do {
        if ((ret = sRecv(&ch, 1)) > 0) {
            if (ch >= 32) {
                line += ch;
            } else {
                if (ch == '\n' && lch == '\r') {
                    if (m_request.empty()) {
                        if ((pos = line.find(" /")) != std::string::npos) {
                            auto posa = line.find(" HTTP/");
                            m_request.emplace_back(line.substr(0, pos));
                            m_request.emplace_back(line.substr(pos + 2, posa - pos - 2));
                            m_request.emplace_back(line.substr(posa + 1, std::string::npos));
                        }

                    } else {
                        if ((pos = line.find(": ")) != std::string::npos) {
                            auto par   = line.substr(0, pos);
                            auto value = line.substr(pos + 2, std::string::npos);
                            m_hdrMap.insert(std::make_pair(par, value));
                        }
                    }

                    if (line.empty()) {
                        break;
                    }

                    lch = 0;
                    line.clear();
                } else {
                    lch = ch;
                }
            }
        }

    } while (ret > 0);

    if (m_request.size() < 3) {  // If the command is incomplete just return
        return false;
    }

    return parseContent();
}

bool Conn::parseContent() {
    uint32_t                 cont_len;
    std::shared_ptr<Content> content = nullptr;

    if ((cont_len = getContentLength()) > 0) {
        if (m_rawContent.readContentData(this, cont_len)) {
            std::string p = getHdrValue(static_cast<const char *>("Content-Type"));

            if (p.find("multipart/form-data") != std::string::npos) {
                size_t      bpos;

                if ((bpos = p.find("boundary=")) != std::string::npos) {
                    std::string boundary = "--" + p.substr(bpos + 9, std::string::npos);

                    m_rawContent.setBoundary(boundary);

                    int         lpos = 0;
                    std::string linestr;
                    size_t      siaux;
                    int         iaux;
                    int         ret=0;

                    do {
                        ret = m_rawContent.moveToNextBoundary();

                        if (lpos > 0 && content != nullptr) {
                            content->setData(
                                m_rawContent.getData() + lpos,
                                (iaux = (m_rawContent.getCurrentPos() - static_cast<int>(boundary.length()) - 4 - lpos)) > 0 ? iaux
                                                                                                                : 0);
                        }

                        if (ret >= 0) {
                            content = std::make_shared<Content>();

                            do {
                                if ((ret = m_rawContent.readLine(linestr)) >= 0) {
                                    if (linestr.find("Content-Disposition: "
                                                     "form-data; ") != std::string::npos) {
                                        content->setName(getQPar(linestr, "name=\""));
                                        content->setFileName(getQPar(linestr, "filename=\""));

                                    } else if ((siaux = linestr.find("Content-Type: ")) != std::string::npos) {
                                        content->setType(linestr.substr(siaux + 14));
                                    }
                                }
                            } while (ret > 0);

                            m_Content.push_back(content);
                            lpos = m_rawContent.getCurrentPos();
                        }

                    } while (ret >= 0);

                    if (ret == -4) {
                        return true;  // [OK] Finish
                    }

                    return false;  // [ERROR] Parsing unsuccessfully
                }

            } else if (p.find("application/x-www-form-urlencoded") != std::string::npos) {
                // m_glbase->appmsg("application/x-www-form-urlencoded");

            } else {
                content = std::make_shared<Content>();
                content->setType(p);
                content->setData((uint8_t *)m_rawContent.getData(), m_rawContent.getSize());
                m_Content.push_back(content);
                return true;
            }
        }
        return false;
    }
}

uint32_t Conn::getContentLength() {
    std::string a;
    return !(a = getHdrValue(static_cast<const char *>("Content-Length"))).empty() ? std::atoi(a.c_str()) : 0;
}

std::string Conn::getHdrValue(const std::string &name) {
    auto pp = m_hdrMap.begin();
    return ((pp = m_hdrMap.find(name)) != m_hdrMap.end()) ? pp->second : "";
}

std::string Conn::getHdrValue(const char *name) {
    auto pp = m_hdrMap.begin();
    return ((pp = m_hdrMap.find(name)) != m_hdrMap.end()) ? pp->second : "";
}

std::string Conn::getQPar(const std::string& src, const std::string &parname) {
    size_t aux[3];

    if (!((aux[2] = parname.length()))) {
        return {};
    }

    if ((aux[0] = src.find(parname)) == std::string::npos) {
        return {};
    }

    aux[0] += aux[2];

    return src.substr(aux[0], (aux[1] = (src.find('\"', aux[0]) - aux[0])) > 0 ? aux[1] : 0);
}

int Conn::SendString(const std::string &s) const {
    return send(m_socket, s.c_str(), static_cast<int>(s.length()), 0);
}

int Conn::SendError(int code) const {
    std::string r = "\r\n\r\nHTTP/1.1 " + std::to_string(code) + " \r\n";
    r += "Server: ARA_Remote_Player/0.1\r\nCache-Control: no-cache\r\nPragma: "
         "no-cache\r\nContent-Type: text/HTML\r\n";
    r += "Content-Length: 0\r\n\r\n";

    return SendString(r);
}

int Conn::SendOK() const {
    std::string r = "\r\n\r\nHTTP/1.1 " + std::to_string(200) + " \r\n";
    r += "Server: ARA_Remote_Player/0.1\r\nCache-Control: no-cache\r\nPragma: "
         "no-cache\r\nContent-Type: text/HTML\r\n";
    r += "Content-Length: 0\r\n\r\n";

    return SendString(r);
}

int Conn::SendResponse(int code, const std::string &text_body) const {
    int         ret = 0;

    std::string r = "\r\n\r\nHTTP/1.1 " + std::to_string(code) + " \r\n";
    r += "Server: ARA_Remote_Player/0.1\r\nCache-Control: no-cache\r\nPragma: "
         "no-cache\r\nContent-Type: text/HTML\r\n";
    r += "Content-Length: " + std::to_string(text_body.length()) + "\r\n\r\n";

    ret += SendString(r);
    return ret + SendString(text_body);
}

void Conn::ParseURI(std::string &filestr, std::vector<std::string> &parameter_list) {
    size_t aux = 0, aux2 = 0;

    filestr = getURI().substr(0, (aux = getURI().find('?')));

    if (aux == std::string::npos) {
        return;
    }

    auto psrc = getURI().substr(aux + 1);

    do {
        aux = aux2 ? aux2 + 1 : aux2;
        parameter_list.push_back(psrc.substr(aux, (aux2 = psrc.find('&', aux)) - aux));
    } while (aux2 != std::string::npos);
}

void Conn::incRecvCount(int count) {
    m_recvByteCount += count;
    if (m_server != nullptr) {
        m_server->incRecvCount(count);
    }
}

int Conn::sRecv(void *dest, int count) {
    int ret;
    if ((ret = recv(m_socket, static_cast<char *>(dest), count, 0)) > 0) {
        incRecvCount(ret);
    }
    return ret;
}

}