#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <algorithm>
#include "Network_Common.h"


namespace ara {
bool TCPLineCmdRequest(std::vector<uint8_t> &dest, const std::string &ipaddr, int port, std::string cmd) {
    SOCKET      s;
    sockaddr_in sa{};
    int         ret, psize = 1024;
    char        auxbuff[1024];

    if (cmd.empty()) {
        return false;
    }

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return false;
    }

    sa.sin_family      = AF_INET;
    sa.sin_addr.s_addr = inet_addr(ipaddr.c_str());
    sa.sin_port        = htons(port);

    if (connect(s, reinterpret_cast<sockaddr *>(&sa), sizeof(sa)) == SOCKET_ERROR) {
        closesocket(s);
        return false;
    }

    cmd += "\r\n";
    auto cstr = const_cast<char *>(cmd.c_str());
    auto left = static_cast<int>(cmd.size());

    do {
        if ((ret = send(s, cstr, std::min<int>(psize, left), 0)) > 0) {
            cstr += ret;
            left -= ret;
        }
    } while (left && ret > 0);

    if (ret <= 0) {
        closesocket(s);
        return false;
    }

    do {
        if ((ret = recv(s, auxbuff, psize, 0)) > 0) {
            dest.insert(dest.end(), auxbuff, auxbuff + ret);
        }
    } while (ret > 0);

    closesocket(s);
    return true;
}

std::string TCPLineCmdRequest(const std::string &ipaddr, int port, const std::string& cmd) {
    std::vector<uint8_t> dest;

    if (!TCPLineCmdRequest(dest, ipaddr, port, cmd)) {
        return {};
    }

    return std::string(dest.begin(), dest.end());
}
}  // namespace ara
