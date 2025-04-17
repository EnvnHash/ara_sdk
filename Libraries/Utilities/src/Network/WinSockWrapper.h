#ifdef _WIN32

#pragma once

#include <windows.h>
#include <sstream>


namespace ara::proj {
static const double c_tmOutSocket_Infinite = 1.7976931348623158e+308;
///<   defines the default time out time for socket requests

typedef long suseconds_t;
typedef int  socklen_t;


// ---------------------------------------------------------------------------------------------------------------------------------------------------
// //                               SocketAddress
// -----------------------------------------------------------------------------------------------------------------------
// struct SocketAddress : sockaddr_in
class SocketAddress {
public:
    // construction
    // use default constructor to create empty address
    SocketAddress() {
        sin_family      = AF_INET;
        sin_port        = 0;
        sin_addr.s_addr = 0;
    }

    // copy constructor
    SocketAddress(sockaddr const& sa) { memcpy(this, &sa, sizeof(sockaddr)); }

    // construct object from sockaddr_in struct
    SocketAddress(sockaddr_in const& sin) { memcpy(this, &sin, sizeof(sockaddr_in)); }

    // construct AF_INET address using UINT-address and port in host-byte-order.
    SocketAddress(unsigned int addr, unsigned short port);

    // construct AF_INET address using url string and port number in host byte
    // order.
    SocketAddress(char const* url, unsigned short port = 0);

    // public methods

    // returns ip in dotted decimal m_format i.e. "10.1.1.1".
    std::string getDottedDecimal() const { return inet_ntoa(sin_addr); };

    // gets / sets port number in host byte order
    unsigned short getPort();

    void setPort(unsigned short port);

    // resets address to zero
    void zero() { memset(this, 0, sizeof(SocketAddress)); }

    // public operators for conversion

    operator sockaddr() const { return *((sockaddr*)this); }
    operator sockaddr*() const { return (sockaddr*)this; }
    operator sockaddr*() { return (sockaddr*)this; }
    operator sockaddr&() const { return *((sockaddr*)this); }
    operator const sockaddr&() const { return (const sockaddr&)*((sockaddr*)this); }
    operator sockaddr_in() { return *((sockaddr_in*)this); }
    operator sockaddr_in*() { return (sockaddr_in*)this; }
    operator sockaddr_in&() { return *((sockaddr_in*)this); }

    // public asignement and compare operators

    const SocketAddress& operator=(const sockaddr& sa);

    const SocketAddress& operator=(const sockaddr_in& sin);

    bool operator==(const SocketAddress& sa) const { return 0 == memcmp(this, &sa, sizeof(SocketAddress)); }

    // public static methods

    // constructs a broadcast address to a specific port in host byte order.
    static SocketAddress broadcast(unsigned short port);
    USHORT               sin_port{};
    IN_ADDR              sin_addr{};
    short                sin_family{};
};

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// //                               Socket
// -----------------------------------------------------------------------------------------------------------------------
// class Socket
class Socket {
#ifdef WIN32

#define ioctl IOCtl
#define IFNAMSIZ 16

    struct ifreq {
        char ifr_name[IFNAMSIZ];
        union {
            sockaddr ifru_addr;
            sockaddr ifru_dstaddr;
            char     ifru_oname[IFNAMSIZ];
            sockaddr ifru_broadaddr;
            short    ifru_flags;
            int      ifru_metric;
            char     ifru_data[1];
            char     ifru_enaddr[6];
        };
    };

#endif // WIN32

protected:
    SOCKET sock;
    bool   bInitWinsock = false;

public:
    // construction

    // construct empty socket
    Socket() { sock = 0; }

    // copy constructor s
    Socket(SOCKET s) { sock = s; }
    Socket(SOCKET& s) { sock = s; }
    // Socket(Socket& s) { sock=s.sock; }
    Socket(const Socket& s) { sock = s.sock; }

    // construct and create socket of type i.e. SOCK_DGRAM or SOCK_STREAM
    Socket(int type, bool broadcast, int protocol, int bKeepAlive, DWORD qRecvBuffer) {
        if (SOCKET_ERROR == create(type, broadcast, protocol, bKeepAlive, qRecvBuffer)) throw SOCKET_ERROR;
    };

    // desructor
    ~Socket() {
        if (bInitWinsock) WSACleanup();
    }

    // public methods
    // intializes WINSOCK
    int init();

    // creates socket handle
    int create(int type, bool broadcast, int protocol, int bKeepAlive, DWORD qRecvBuffer);

    // set size of receive buffer
    int setRecvBuff(int size);

    // set size of receive buffer
    int getRecvBuffSize(int& size);

    // close socket
    int close();

    // bind socket to local address
    int bind(SocketAddress const& sa) { return ::bind(sock, sa, sizeof(sockaddr)); };

    // listen on socket
    int listen(DWORD qConnMax = 0) { return ::listen(sock, (qConnMax) ? qConnMax : SOMAXCONN); };

    // connect socket to peer address
    int connect(SocketAddress const& sa) { return ::connect(sock, sa, sizeof(sockaddr)); };

    // accepts connections; socket needs to be stream oriented
    Socket accept() { return (SOCKET)::accept(sock, NULL, NULL); };

    // check socket to be ready to read from, operation blocks
    int tryRead(const double timeout = c_tmOutSocket_Infinite) const;

    // check socket to be ready to write to, operation blocks
    int tryWrite(const double timeout = c_tmOutSocket_Infinite) const;

    // check for any exceptions on socket
    int test(const double timeout = c_tmOutSocket_Infinite) const;

    // send some data to a conected socket, operation blocks
    int send(const char* pch, const int iSize, const double timeout = c_tmOutSocket_Infinite);

    // send some data to a concected socket direct, without select
    int sendDirect(const char* pch, const int iSize);

    // receive some date from connected socket, operation blocks
    int recv(char* pch, const int iSize, const double timeout = c_tmOutSocket_Infinite);

    // receive some date from connected socket direct, witout select
    int recvDirect(char* pch, const int iSize);

    // sends datagram to peer
    int sendDatagram(const char* buf, const int iSize, SocketAddress& sa, bool dontRoute = FALSE);

    // receives datagram from socket, peer address will be stored in 'sa' if
    // specified
    int recvDatagram(char* buf, const int iSize, SocketAddress* sa = NULL);

    // returns connected peer address
    SocketAddress getpeeraddr();

    // returns bound local address
    SocketAddress getsockaddr();
    int           getsockaddr(sockaddr_in& addr, int& qAddr);

    // public static methods

    // these are wrapper functions for os native operations derived from
    // berkeley sockets
    static SocketAddress gethostbyname(const std::string& name, const unsigned short port = 0);
    static std::string   gethostbyaddr(const SocketAddress& sa);
    static int           getLastError();
    static std::string   getErrorMessage(int err);

    // the ntoh and hton functions are now polymorph
    static unsigned short   ntoh(unsigned short netshort) { return ::ntohs(netshort); }
    static unsigned short   hton(unsigned short hostshort) { return ::htons(hostshort); }
    static unsigned int     ntoh(unsigned int netint) { return ::ntohl(netint); }
    static unsigned int     hton(unsigned int hostint) { return ::htonl(hostint); }
    static unsigned __int64 ntoh(unsigned __int64 netlong);
    static unsigned __int64 hton(unsigned __int64 hostlong);

    // public asignmet and compare operators
    const Socket& operator=(const Socket& s) {
        sock = s.sock;
        return *this;
    };
    const Socket& operator=(const SOCKET s) {
        sock = s;
        return *this;
    };
    // void operator=(const Socket& s) { sock=s.sock; };
    // void operator=(const SOCKET s) { sock=s; };
    operator SOCKET() { return sock; };
    bool        operator==(const Socket& s) const { return sock == s.sock; };
    bool        operator==(const SOCKET s) const { return sock == s; };
    friend bool operator==(const SOCKET s, const Socket& ss) { return s == ss.sock; };
};

/** Tries to send file content to pc, specified through its ip.\n
 * @param  [IN] szHost					the destination host
 name
 * @param  [IN_OPT] iPort				the port number to use,
 set to 0 to use standard http port 80
 * @param  [IN_OPT] szURI				zero terminated string,
 conatinig the request, if NULL or "" it is set to "/"
 * @param  [IN_OPT] szName				zero terminated string
 with the name of the post variable
 * @param  [IN_OPT] content				stream containing the
 content, this might be file content or the value of the post variable
 * @param  [IN_OPT] szMime				zero terminated string
 containing the mime, if empty it will be set to x-form/octett-stream
 * @param  [IN_OPT] szFileName			the name of the file
 * @param  [IN_OPT] szFileClass         if set, another post variable is
 "upload" is added with contents of szFileclass as value; this is only valid, if
 content is not NULL
 * @param  [OUT_OPT] response			a string to receive the server's
 response

 * @remark the file is aways named "file.xml" use URI and
 * @see    ESPCommonError
 * @return TRUE file was send successfully
 * @return FALSE  wrong parameter or error */
BOOL SendHTTP(LPCSTR szHost, unsigned short iPort, LPCSTR szURI, LPCSTR szName, std::istream* content, LPCSTR szMime,
              LPCSTR szFileName, LPCSTR szFileClass, std::string* response);

}  // namespace ara::proj
#endif
