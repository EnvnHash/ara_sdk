//
// Created by user on 22.06.2020.
//

#pragma once

#include "Network/Network_Common.h"

namespace ara {

bool TCPLineCmdRequest(std::vector<uint8_t> &dest, std::string &ipaddr, int port, std::string cmd);

std::string TCPLineCmdRequest(std::string &ipaddr, int port, const std::string& cmd);

}  // namespace ara
