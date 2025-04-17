//
// Created by user on 22.06.2020.
//

#pragma once

#include "Network_Common.h"
#include "ThreadedTasks/Cycler.h"

namespace ara {
class UDPSignaler : public Cycler {
public:
    bool StartBroadcast(int port, int period_ms, std::function<int(void *, int)> const &f);

private:
    std::function<int(void *data, int max_size)> m_OnEmit;  // return data_size, max_size has the max possible datasize
                                                            // data is passed and copied into a safe m_buffer inside the
                                                            // cycle m_OnEmit is called if virtual OnEmit retuns 0
protected:
    SOCKET      m_Socket    = 0;
    int         m_Port      = 0;
    int         m_Period_ms = 1000;
    sockaddr_in m_SockAddr;

    bool OnCycleStop() override;
    bool OnCycle() override;

    virtual int OnEmit(void *buff, int max_size) {
        return 0;
    }  // return data_size, data is passed and copied into a safe m_buffer
       // inside the cycle
};
}  // namespace ara