#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include "ThreadedTasks/Cycler.h"

#if defined(_WIN32) || defined(WIN32) || defined(WIN64)

// #define NOMINMAX
#include <WS2tcpip.h>
#include <WinSock2.h>
// #undef NOMINMAX

#pragma comment(lib, "ws2_32.lib")

#define ERRINTR (WSAEINTR)
#define lastNetError WSAGetLastError()
typedef long suseconds_t;
typedef int  socklen_t;

#define ifStartSockets() for (WSADATA d = {0}; 0 == d.wVersion && 0 == WSAStartup(0x0002, &d);)
#define closeSockets WSACleanup

#else /* WIN32 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define SOCKET int
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR (-1)
#define ERRINTR (10004)  // can native bsd socket's be interrupted?
#define lastNetError errno

#define ifStartSockets() if (1)
#define closeSockets (0)
typedef struct WSADATA {
    int i;
} WSADATA;

#define closesocket(s)  \
    {                   \
        shutdown(s, 2); \
        close(s);       \
    }

#endif /* WIN32 */
