#ifdef _WIN32

#include "WinSockWrapper.h"

#include <iostream>
#include <list>

namespace ara::proj {

// ----------------------------------------------------------------------------------
//                               SocketAddress
// ----------------------------------------------------------------------------------

SocketAddress::SocketAddress(unsigned int addr, unsigned short port) :
    sin_port(Socket::hton(port)), sin_family(AF_INET) {
    sin_addr.s_addr = Socket::hton(addr);
}

SocketAddress::SocketAddress(char const *url, unsigned short port) {
    sin_family = AF_INET;
    do {
        if (!port) {
            char *pC = strrchr((char *)url, ':');
            if (pC && (sscanf_s(pC, ":%hu", &port) > 0)) {
                *pC             = 0x0;
                sin_port        = Socket::hton(port);
                sin_addr.s_addr = inet_addr(url);
                *pC             = ':';
                break;
            }
        }

        sin_port        = Socket::hton(port);
        sin_addr.s_addr = inet_addr(url);
    } while (0);

    if (INADDR_NONE == sin_addr.s_addr) {
        hostent far *host = gethostbyname(url);
        if (host) {
            char far *addr = host->h_addr_list[0];
            if (addr != NULL) memcpy(&sin_addr, host->h_addr_list[0], host->h_length);
        } else {
            sin_family      = AF_UNSPEC;
            sin_port        = 0;
            sin_addr.s_addr = INADDR_ANY;
        }
    }
}

const SocketAddress &SocketAddress::operator=(const sockaddr &sa) {
    memset(this, 0, sizeof(SocketAddress));
    memcpy(this, &sa, sizeof(sockaddr));
    return *this;
}

const SocketAddress &SocketAddress::operator=(const sockaddr_in &sin) {
    memset(this, 0, sizeof(SocketAddress));
    memcpy(this, &sin, sizeof(sockaddr_in));
    return *this;
}

unsigned short SocketAddress::getPort() { return Socket::ntoh(sin_port); };

void SocketAddress::setPort(unsigned short port) { sin_port = Socket::hton(port); };

SocketAddress SocketAddress::broadcast(unsigned short port) {
    sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family           = AF_INET;
    sa.sin_port             = Socket::hton(port);
    sa.sin_addr.S_un.S_addr = INADDR_BROADCAST;
    return sa;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------
//                               Socket
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------

int Socket::init() {
#if defined(WIN32) || defined(WIN64)
    WSADATA d;
    int     iErr = WSAStartup(MAKEWORD(2, 0), &d);

    if (iErr) {
        std::cerr << Socket::getErrorMessage(iErr).c_str() << std::endl;
        throw std::exception();
    } else
        bInitWinsock = true;

    return iErr;
#else
    return 0;
#endif
}

int Socket::create(int type, bool broadcast, int protocol, int bKeepAlive, DWORD qRecvBuffer) {
    int err;

    sock = ::socket(AF_INET, type, protocol);

    if (sock == INVALID_SOCKET) return WSAGetLastError();

    if (broadcast) {
        if ((SOCK_DGRAM == type) && ((sock != SOCKET_ERROR) || (sock != INVALID_SOCKET))) {
            BOOL param = 1;
            int  err   = ::setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char *)&param, sizeof(BOOL));
            if (err == SOCKET_ERROR) return err;
        } else
            return SOCKET_ERROR;
    }

    if (bKeepAlive) {
        BOOL param = 1;

        err = ::setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (const char *)&param, sizeof(BOOL));
        if (err) return err;
    }

    if (qRecvBuffer) {
        int sz = 0;

        if (!getRecvBuffSize(sz) && (sz < (int)qRecvBuffer)) {
            sz  = (int)qRecvBuffer;
            err = ::setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (const char FAR *)&sz, sizeof(sz));
            if (err) return err;
        }
    }

    return 0;
}

int Socket::setRecvBuff(int size) {
    return ::setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (const char FAR *)&size, sizeof(size));
}

int Socket::getRecvBuffSize(int &size) {
    int len = sizeof(size);
    int err = ::getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char FAR *)&size, &len);

    if (err || (size < 0)) {
        size = -1;
        return SOCKET_ERROR;
    }

    return 0;
}

int Socket::close() {
    if (sock == NULL || sock == INVALID_SOCKET) return SOCKET_ERROR;
    int ret = 0;

#ifdef WIN32
    ret = ::closesocket(sock);
#else  // def WIN32
    ret = ::close(sock);
#endif // def WIN32
    sock = 0;
    return ret;
}

int Socket::tryRead(const double timeout) const {
    fd_set  fd = {1, sock};
    timeval t, *pt = NULL;
    if (timeout < c_tmOutSocket_Infinite) {
        t.tv_sec  = (long)timeout;
        t.tv_usec = suseconds_t((timeout - t.tv_sec) * 1000000.0);
        pt        = &t;
    }
    return ::select(0, &fd, NULL, NULL, pt);
}

int Socket::tryWrite(const double timeout) const {
    fd_set  fd = {1, sock};
    timeval t, *pt = NULL;
    if (timeout < c_tmOutSocket_Infinite) {
        t.tv_sec  = (long)timeout;
        t.tv_usec = suseconds_t((timeout - t.tv_sec) * 1000000.0);
        pt        = &t;
    }
    return ::select(0, NULL, &fd, NULL, pt);
}

int Socket::test(const double timeout) const {
    fd_set  fd = {1, sock};
    timeval t, *pt = NULL;
    if (timeout < c_tmOutSocket_Infinite) {
        t.tv_sec  = (long)timeout;
        t.tv_usec = suseconds_t((timeout - t.tv_sec) * 1000000.0);
        pt        = &t;
    }
    return ::select(0, NULL, NULL, &fd, pt);
}

int Socket::send(const char *pch, const int iSize, const double timeout) {
    int iBytesSend     = 0;
    int iBytesThisTime = 0;
    int s              = tryWrite(timeout);

    if (s == 1) do {
            iBytesThisTime = ::send(sock, pch, iSize - iBytesSend, 0);
            if (iBytesThisTime <= 0) break;
            iBytesSend += iBytesThisTime;
            if (iBytesSend >= iSize) break;
            pch += iBytesThisTime;
        } while (1);

    return iBytesSend;
}

int Socket::sendDirect(const char *pch, const int iSize) {
    int iBytesSend     = 0;
    int iBytesThisTime = 0;

    do {
        if ((iBytesThisTime = ::send(sock, pch, iSize - iBytesSend, 0)) == 0) break;
        if (iBytesThisTime == SOCKET_ERROR) break;
        if ((iBytesSend += iBytesThisTime) >= iSize) break;
        pch += iBytesThisTime;
    } while (iBytesSend < iSize);

    return iBytesSend;
}

int Socket::recv(char *pch, const int iSize, const double timeout) {
    if (tryRead(timeout) == 1) {
        return ::recv(sock, pch, iSize, 0);
    }

    return 0;
}

int Socket::recvDirect(char *pch, const int iSize) { return ::recv(sock, pch, iSize, 0); }

SocketAddress Socket::getpeeraddr() {
    sockaddr a;
    int      addLen = sizeof(sockaddr);
    if (::getpeername(sock, &a, &addLen) == SOCKET_ERROR) a.sa_family = AF_UNSPEC;
    return a;
}

SocketAddress Socket::getsockaddr() {
    sockaddr a;
    int      addLen = sizeof(sockaddr);

    ZeroMemory(&a, addLen);
    if (::getsockname(sock, &a, &addLen) == SOCKET_ERROR) a.sa_family = AF_UNSPEC;

    return a;
}

int Socket::getsockaddr(sockaddr_in &addr, int &qAddr) {
    qAddr = sizeof(sockaddr_in);
    ZeroMemory(&addr, qAddr);
    return ::getsockname(sock, (sockaddr *)&addr, &qAddr);
}

int Socket::sendDatagram(const char *pch, const int iSize, SocketAddress &sa, bool dontRoute) {
    // fd_set fd={1,sock};

    // if(::select( 0, NULL, &fd, NULL, NULL)==1)
    return ::sendto(sock, pch, iSize, (dontRoute) ? MSG_DONTROUTE : 0, sa, sizeof(SOCKADDR));

    // return 0;
}

int Socket::recvDatagram(char *pch, const int iSize, SocketAddress *sa) {
    fd_set    fd   = {1, sock};
    socklen_t size = sizeof(SocketAddress);
    int       res  = ::select(0, &fd, NULL, NULL, 0);

    if ((res != SOCKET_ERROR) && (res > 0)) {
        res = ::recvfrom(sock, pch, iSize, 0, (struct sockaddr *)sa, &size);
    }

    return res;
}

// static
int Socket::getLastError() { return h_errno; }

// static
std::string Socket::getErrorMessage(int err) {
    char *mesg;
    switch (err) {
#ifdef WIN32
        case WSAEINTR: mesg = (char *)"Operation wegen Unterbrechungsanforderung abgebrochen"; break;
        case WSAEACCES: mesg = (char *)"Zugriff verweigert"; break;
        case WSAEFAULT: mesg = (char *)"Fehler in Adresse"; break;
        case WSAEINVAL: mesg = (char *)"Falscher Parameter"; break;
        case WSAEMFILE: mesg = (char *)"Zu viele geoeffnete Dateien"; break;
        case WSAEWOULDBLOCK: mesg = (char *)"Adapter blockiert"; break;
        case WSAEINPROGRESS: mesg = (char *)"Adapter wartet auf andere Operation"; break;
        case WSAEALREADY: mesg = (char *)"Andere Anfrage wird gerade bearbeitet"; break;
        case WSAENOTSOCK: mesg = (char *)"Parameter ist kein Socket"; break;
        case WSAEDESTADDRREQ: mesg = (char *)"Zieladresse wird benoetigt"; break;
        case WSAEMSGSIZE: mesg = (char *)"Nachricht zu lang"; break;
        case WSAEPROTOTYPE: mesg = (char *)"Falscher Protokoll-Typ"; break;
        case WSAENOPROTOOPT: mesg = (char *)"Fehlerhafte Protokoll-Option"; break;
        case WSAEPROTONOSUPPORT: mesg = (char *)"Protokoll nicht unterstuetzt"; break;
        case WSAESOCKTNOSUPPORT: mesg = (char *)"Socket-Typ nicht unterstuetzt"; break;
        case WSAEOPNOTSUPP: mesg = (char *)"Operation nicht unterstuetzt"; break;
        case WSAEPFNOSUPPORT: mesg = (char *)"Protokoll-Familie nicht unterstuetzt"; break;
        case WSAEAFNOSUPPORT: mesg = (char *)"Adress-Familie nicht unterstuetzt"; break;
        case WSAEADDRINUSE: mesg = (char *)"Adresse wird schon verwendet"; break;
        case WSAEADDRNOTAVAIL: mesg = (char *)"Adresse nicht verfuegbar"; break;
        case WSAENETDOWN: mesg = (char *)"Netzwerkfehler"; break;
        case WSAENETUNREACH: mesg = (char *)"Zielnetz nicht erreichbar"; break;
        case WSAENETRESET: mesg = (char *)"Netzwerk zurueckgesetzt"; break;
        case WSAECONNABORTED: mesg = (char *)"Verbindung wurde getrennt"; break;
        case WSAECONNRESET: mesg = (char *)"Verbindung wurde zurueckgesetzt"; break;
        case WSAENOBUFS: mesg = (char *)"Kein Pufferspeicher verfuegbar"; break;
        case WSAEISCONN: mesg = (char *)"Socket ist schon verbunden"; break;
        case WSAENOTCONN: mesg = (char *)"Socket ist nicht verbunden"; break;
        case WSAESHUTDOWN: mesg = (char *)"Verbindung wurde getrennt"; break;
        case WSAETIMEDOUT: mesg = (char *)"Zeitueberschreitung"; break;
        case WSAECONNREFUSED: mesg = (char *)"Verbindung Abgelehnt"; break;
        case WSAENAMETOOLONG: mesg = (char *)"URI zu lang"; break;
        case WSAEHOSTDOWN: mesg = (char *)"Zielcomputer nicht am Netz"; break;
        case WSAEHOSTUNREACH: mesg = (char *)"Zielcomputer nicht erreichbar"; break;
        case WSAEPROCLIM:
            mesg = (char *)"Zu veile Prozesse";
            break;

#else  // ndef WIN32
#endif // ndef WIN32
        default: mesg = (char *)"Fehler nicht bestimmbar";
    }
    return mesg;
}

// static
SocketAddress Socket::gethostbyname(const std::string &name, const unsigned short port) {
    hostent *pHostEnt = ::gethostbyname(name.c_str());
    if (pHostEnt == NULL) {
        sockaddr a;
        a.sa_family = AF_UNSPEC;
        return a;
    }
    unsigned int *pulAddr = (unsigned int *)pHostEnt->h_addr_list[0];
    sockaddr_in   sTmp;
    sTmp.sin_family      = AF_INET;
    sTmp.sin_port        = ::htons(port);
    sTmp.sin_addr.s_addr = *pulAddr;
    return sTmp;
}

// static
std::string Socket::gethostbyaddr(const SocketAddress &sa) {
    hostent *pHostEnt = ::gethostbyaddr((char *)&((sockaddr_in *)&sa)->sin_addr.s_addr, 4, PF_INET);
    if (pHostEnt == NULL) return NULL;
    return pHostEnt->h_name;
}

// static
unsigned __int64

Socket::ntoh(unsigned __int64 netlong) {
    if (htons(1) == 1) {
        return netlong;
    } else {
        char *c = reinterpret_cast<char *>(&netlong);
        char  c2[8];
        c2[0] = c[7];
        c2[1] = c[6];
        c2[2] = c[5];
        c2[3] = c[4];
        c2[4] = c[3];
        c2[5] = c[2];
        c2[6] = c[1];
        c2[7] = c[0];
        return reinterpret_cast<unsigned __int64 &>(*c2);
    }
}

// static
unsigned __int64 Socket::hton(unsigned __int64 hostlong) {
    if (htons(1) == 1) {
        return hostlong;
    } else {
        char *c = reinterpret_cast<char *>(&hostlong);
        char  c2[8];
        c2[0] = c[7];
        c2[1] = c[6];
        c2[2] = c[5];
        c2[3] = c[4];
        c2[4] = c[3];
        c2[5] = c[2];
        c2[6] = c[1];
        c2[7] = c[0];
        return reinterpret_cast<unsigned __int64 &>(*c2);
    }
}

// return
// response code
int ParseStdResponseCode(char *pStr, DWORD qStr) {
    if (!(pStr && qStr)) return 500;

    int   i;
    DWORD q;
    char *pLnEnd;
    char *pTotEnd = pStr + qStr;

    for (pLnEnd = pStr; (pLnEnd < pTotEnd) && (*pLnEnd != '\n'); pLnEnd++)
        ;
    q = (DWORD)(pLnEnd - pStr);
    if ((q > 11) && !_strnicmp(pStr, "http/", 5)) {
        for (pStr += 5; (pStr < pLnEnd) && (*pStr != ' '); pStr++)
            ;
        for (pStr++; (pStr < pLnEnd) && (*pStr == ' '); pStr++)
            ;
        if (pStr < pLnEnd) {
            i = atoi(pStr);
            if (i >= 0) return i;
        }
    } else if (q) {
        char strTmp[32];

        if (q > 31) q = 31;
        memcpy(strTmp, pStr, q);
        strTmp[q] = 0x0;
        i         = atoi(strTmp);
        if (i > 0) return 200;
    }

    return 500;
}

BOOL SendHTTP(LPCSTR szHost, unsigned short iPort, LPCSTR szURI, LPCSTR szName, std::istream *content, LPCSTR szMime,
              LPCSTR szFileName, LPCSTR szFileClass, std::string *response) {
    if ((nullptr == szHost || 0 == szHost[0])) return FALSE;
    if (0 == iPort) iPort = 80;
    if (nullptr == szURI) szURI = "/";

    Socket sock;
    if (sock.create(SOCK_STREAM, FALSE, IPPROTO_TCP, FALSE, 0)) return FALSE;

    SocketAddress dstAddr = SocketAddress(szHost, iPort);
    if (!sock.connect(dstAddr)) {
        std::list<std::istream *> chunks;
        LPCSTR                    boundary = "---------------------------ARASPCALIBRATOR";
        BOOL                      res      = FALSE;
        if (nullptr != content && !content->bad()) {
            if (NULL == szMime || 0 == szMime[0]) szMime = "application/octet-stream";

            std::stringstream *chunk = new std::stringstream();
            // send file
            *chunk << "\015\012--" << boundary << "\015\012"
                   << "Content-Disposition: form-data; name=\"" << szName << "\"";
            if (nullptr != szFileName && 0 != szFileName[0]) *chunk << "; filename=\"" << szFileName << "\"";
            *chunk << "\015\012" << "Content-Type: " << szMime << "\015\012"
                   << "\015\012";
            chunks.push_back(chunk);
            chunks.push_back(content);

            if (NULL != szFileClass && 0 != szFileClass[0]) {
                std::stringstream *chunk = new std::stringstream();
                *chunk << "\015\012--" << boundary << "\015\012"
                       << "Content-Disposition: form-data; name=\"upload\"\015\012"
                       << "Content-Type: text/xml\015\012" << "\015\012" << szFileClass;
                chunks.push_back(chunk);
            }
        }
        if (!chunks.empty()) {
            // add closing boundary
            std::stringstream *chunk = new std::stringstream();
            *chunk << "\015\012--" << boundary << "--";
            chunks.push_back(chunk);

            size_t l = 0;
            try {
                for (auto &chunk : chunks) {
                    std::streampos oldGet = chunk->tellg();
                    chunk->seekg(0, std::ios_base::end);
                    l += (size_t)chunk->tellg();
                    chunk->seekg(oldGet);
                }
            } catch (...) {
                l = 0;
            }

            // finally the actual header
            chunk = new std::stringstream();
            *chunk << "POST " << szURI << " HTTP/1.1\015\012"
                   << "Host: " << szHost << ":" << iPort << "\015\012"
                   << "User-Agent: ARA SPCalibrator\015\012"
                   << "Content-Type: multipart/form-data; boundary=" << boundary << "\015\012";

            // add content length, if it can be determined
            if (0 < l) *chunk << "Content-Length: " << l << "\015\012";

            *chunk << "\015\012";  // empty header line, to finish header

            // this  goes first
            chunks.push_front(chunk);
        } else {
            std::stringstream *chunk = new std::stringstream();
            *chunk << "GET " << szURI << " HTTP/1.1\015\012"
                   << "Host: " << szHost << ":" << iPort << "\015\012"
                   << "User-Agent: ARA SPCalibrator\015\012" << "\015\012";
            chunks.push_back(chunk);
        }

        int packetSize = 2048;
        sock.getRecvBuffSize(packetSize);
        char  *sndBuf = new char[packetSize];
        size_t nSent  = 0;
        for (auto &chunk : chunks) {
            while (chunk->read(sndBuf, packetSize)) nSent += sock.send(sndBuf, packetSize, c_tmOutSocket_Infinite);
            nSent += sock.send(sndBuf, (int)chunk->gcount(), c_tmOutSocket_Infinite);
        }
        delete[] sndBuf;

        char lBuf[8192];
        int  nRead = (__int64)sock.recv(lBuf, 8192, c_tmOutSocket_Infinite);
        if (nRead > 0) {
            if (nullptr != response) response->assign(lBuf, (size_t)nRead);
            if (400 > ParseStdResponseCode(lBuf, nRead)) res = TRUE;
        }

        for (auto &chunk : chunks) {
            if (content != chunk) delete chunk;
        }
        sock.close();
        return res;
    }
    return FALSE;
}

}  // namespace ara::proj

#endif