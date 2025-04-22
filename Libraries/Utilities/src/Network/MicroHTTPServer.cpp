#include "Network/MicroHTTPServer.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <map>
#include <utility>

#include "Network/Network_Common.h"

namespace ara::microhttp {
bool Server::Start(int port, Server::c_cb const &f) {
    m_OnConnection = f;
    TCPListener::StartListen(port, nullptr);
    return TCPListener::Start();
}

bool Server::OnConnect(SOCKET sock, sockaddr_in *addr) {
    Conn conn(this);

    if (!conn.Process(sock, addr)) return false;

    if (m_OnConnection) m_OnConnection(conn);

    return true;
}

void Server::incRecvCount(int count) {
    std::unique_lock<std::mutex> lock(i_CMutex);
    i_RecvByteCount += count;
}

// -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Conn
// -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Conn::Conn(Server *m): m_Socket(0) { m_Server = m; }

bool Conn::Process(SOCKET sock, const sockaddr_in *addr) {
    m_FromAddr.assign(inet_ntoa(addr->sin_addr));
    m_Socket = sock;

    std::string line;
    int         ret;
    char        ch, lch = 0;

    std::size_t pos;

    do {
        if ((ret = sRecv(&ch, 1)) > 0) {
            if (ch >= 32)
                line += ch;
            else {
                if (ch == '\n' && lch == '\r') {
                    if (m_Request.empty()) {
                        if ((pos = line.find(" /")) != std::string::npos) {
                            std::size_t posa = line.find(" HTTP/");

                            m_Request.push_back(line.substr(0, pos));
                            m_Request.push_back(line.substr(pos + 2, posa - pos - 2));
                            m_Request.push_back(line.substr(posa + 1, std::string::npos));
                        }

                    } else {
                        if ((pos = line.find(": ")) != std::string::npos) {
                            std::string par   = line.substr(0, pos);
                            std::string value = line.substr(pos + 2, std::string::npos);

                            m_HdrMap.insert(std::make_pair(par, value));
                        }
                    }

                    if (line.empty()) break;

                    lch = 0;
                    line.clear();

                } else
                    lch = ch;
            }
        }

    } while (ret > 0);

    if (m_Request.size() < 3) {  // If the command is incomplete just return
        return false;
    }

    uint32_t                 cont_len;
    std::shared_ptr<Content> content = nullptr;

    if ((cont_len = getContentLength()) > 0) {
        // if (m_RawContent.readContentData(sock, cont_len)) {
        if (m_RawContent.readContentData(this, cont_len)) {
            std::string p = getHdrValue(static_cast<const char *>("Content-Type"));

            if (p.find("multipart/form-data") != std::string::npos) {
                size_t      bpos;

                if ((bpos = p.find("boundary=")) != std::string::npos) {
                    std::string boundary = "--" + p.substr(bpos + 9, std::string::npos);

                    m_RawContent.SetBoundary(boundary);

                    int         lpos = 0;
                    std::string linestr;
                    size_t      siaux;
                    int         iaux;

                    do {
                        ret = m_RawContent.MoveToNextBoundary();

                        if (lpos > 0 && content != nullptr) {
                            content->setData(
                                m_RawContent.getData() + lpos,
                                (iaux = (m_RawContent.getCurrentPos() - static_cast<int>(boundary.length()) - 4 - lpos)) > 0 ? iaux
                                                                                                                : 0);
                        }

                        if (ret >= 0) {
                            content = std::make_shared<Content>();

                            do {
                                if ((ret = m_RawContent.ReadLine(linestr)) >= 0) {
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
                            lpos = m_RawContent.getCurrentPos();
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
                content->setData((uint8_t *)m_RawContent.getData(), m_RawContent.getSize());

                m_Content.push_back(content);

                return true;
            }
        }

        return false;
    }

    return true;
}

bool RawContent::readContentData(Conn *conn, uint32_t count) {
    m_ContentData.reserve(count + 1024);

    uint32_t tread = 0;
    uint32_t left  = count;
    uint32_t mtr  = 16 << 10;
    uint8_t *dest = m_ContentData.data();
    int      ret;

    m_ContentData.resize(count);

    while (left > 0) {
        auto tr = std::min<uint32_t>(mtr, left);

        if ((ret = conn->sRecv(dest, static_cast<int>(tr))) > 0) {
            tread += ret;
            left -= ret;
            dest += ret;
        } else {
            return false;
        }
    }

    // CstFile file; file.Create("out.txt"); file.Write(m_ContentData.data(),
    // (uint32_t)m_ContentData.size());	file.Close();		// to output the
    // raw data

    return true;
}

bool RawContent::readContentData(SOCKET sock, uint32_t count) {
    m_ContentData.reserve(count + 1024);

    uint32_t tread = 0;
    uint32_t left  = count;
    uint32_t mtr  = 16 << 10;
    uint8_t *dest = m_ContentData.data();
    int      ret;

    m_ContentData.resize(count);

    while (left > 0) {
        uint32_t tr = std::min<uint32_t>(mtr, left);

        if ((ret = recv(sock, reinterpret_cast<char *>(dest), static_cast<int>(tr), 0)) > 0) {
            tread += ret;
            left -= ret;
            dest += ret;
        } else {
            return false;
        }
    }

    // CstFile file; file.Create("out.txt"); file.Write(m_ContentData.data(),
    // (uint32_t)m_ContentData.size());	file.Close();		// to output the
    // raw data

    return true;
}

int RawContent::MoveToNextBoundary() {
    const char *bstr = m_Boundary.c_str();
    int         blen = static_cast<int>(m_Boundary.length());
    int         lpos = static_cast<int>(m_ContentData.size());

    if (blen <= 0) {
        return -1;
    }

    lpos -= blen - 4;  // --\r\n

    if (lpos < 0) {
        return -2;
    }

    auto src = reinterpret_cast<char *>(m_ContentData.data());

    while (m_Pos < lpos) {
        if (src[m_Pos] == bstr[0] && !memcmp(&src[m_Pos], bstr,
                                             blen)) {  // check first byte and if it passes then perform
                                                       // a full check with memcmp, this increases speed
            m_Pos += blen;

            if (src[m_Pos + 0] == '-' && src[m_Pos + 1] == '-') {
                m_Pos += 2;
                return -4;  // reached the end
            }

            if (src[m_Pos + 0] == '\r' && src[m_Pos + 1] == '\n') {
                m_Pos += 2;
            }
            return m_Pos;
        }
        ++m_Pos;
    }
    return -3;
}

int RawContent::ReadLine(std::string &s, int max_size) {
    int lpos = static_cast<int>(m_ContentData.size());
    int ipos = m_Pos, len = 0;

    if (max_size <= 0 || m_Pos >= lpos) {
        return -1;
    }

    auto src = reinterpret_cast<char *>(m_ContentData.data());

    lpos -= 1;

    while (m_Pos < lpos && (len = (m_Pos - ipos)) <= max_size) {
        if (src[m_Pos + 0] == '\r' && src[m_Pos + 1] == '\n') {
            s.assign(&src[ipos], len);
            m_Pos += 2;
            return len;
        }

        m_Pos++;
    }

    s.assign(&src[ipos], len);

    return -2;
}

void RawContent::SetBoundary(const std::string &bstr) {
    m_Boundary = bstr;
}

uint32_t Conn::getContentLength() {
    std::string a;
    return !(a = getHdrValue(static_cast<const char *>("Content-Length"))).empty() ? std::atoi(a.c_str()) : 0;
}

std::string Conn::getHdrValue(const std::string &name) {
    auto pp = m_HdrMap.begin();
    return ((pp = m_HdrMap.find(name)) != m_HdrMap.end()) ? pp->second : "";
}

std::string Conn::getHdrValue(const char *name) {
    auto pp = m_HdrMap.begin();
    return ((pp = m_HdrMap.find(name)) != m_HdrMap.end()) ? pp->second : "";
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
    return send(m_Socket, s.c_str(), static_cast<int>(s.length()), 0);
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
    i_RecvByteCount += count;
    if (m_Server != nullptr) {
        m_Server->incRecvCount(count);
    }
}

int Conn::sRecv(void *dest, int count) {
    int ret;
    if ((ret = recv(m_Socket, static_cast<char *>(dest), count, 0)) > 0) {
        incRecvCount(ret);
    }
    return ret;
}

// Content

bool Content::setName(const std::string& str) {
    m_Name = str;
    return true;
}

std::string Content::getName() {
    return m_Name;
}

bool Content::setType(const std::string& str) {
    m_Type = str;
    return true;
}

std::string Content::getType() {
    return m_Type;
}

bool Content::setFileName(const std::string& str) {
    m_FileName = str;
    return true;
}

std::string Content::getFileName() {
    return m_FileName;
}

bool Content::setData(uint8_t *dataptr, int datasize) {
    m_DataPtr  = dataptr;
    m_DataSize = datasize;
    return true;
}

uint8_t *Content::getData() const {
    return m_DataPtr;
}

int Content::getSize() const {
    return m_DataSize;
}

bool Content::storeToFile(const std::filesystem::path &p) const {
    if (getData() == nullptr) {
        return false;
    }

    std::ofstream f;

    try {
        f.open(p, std::ios::out | std::ios::binary | std::ios::trunc);
    } catch (...) {
        return false;
    }

    f.write(reinterpret_cast<char *>(getData()), getSize());
    f.close();

    return true;
}

// Global

void utilParse(std::string &name, std::string &val, std::string &src_string, char delimiter) {
    size_t aux;
    if ((aux = src_string.find(delimiter)) != std::string::npos) {
        name = src_string.substr(0, aux);
        val  = src_string.substr(name.length() + 1);

    } else {
        name = src_string;
    }
}

}  // namespace ara::microhttp
