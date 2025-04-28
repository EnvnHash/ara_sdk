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

#include <UtilityUnitTestCommon.h>
#include <Network/UDPReceiver.h>
#include <Network/UDPSender.h>
#include "string_utils.h"

using namespace std;

namespace ara {

TEST(UdpSignalTest, SendReceive) {
    Conditional gotMsg;
    bool check = false;
    std::string str = "Hello World";

    UDPReceiver receiver;
    UDPSender sender;

    receiver.StartListen(1234, [&](char* data, int datalen, sockaddr_in* a) {
        auto ret = std::string(data);
        if (ret == str){
            check = true;
            gotMsg.notify();
        }
    });

    sender.StartBroadcast(1234, 1000, [&str](void* data, int max_size) {
        std::copy(str.begin(), str.end(), static_cast<char*>(data));
        return str.length();
    });

    gotMsg.wait();
    EXPECT_TRUE(check);
    receiver.Stop();
    sender.Stop();
}

}