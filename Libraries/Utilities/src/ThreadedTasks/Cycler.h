#pragma once

#include <util_common.h>

namespace ara {

class Cycler {
public:
    virtual ~Cycler() = default;

    enum class CycleState { none, starting, running, stopping, finished, err_failed };

    CycleState GetCycleState();
    bool       IsRunning() { return (GetCycleState() == CycleState::running); }
    bool       Start();
    bool       Stop(bool async = false);
    void       Cycle();

private:
    void Cycle_Stop();

protected:
    CycleState m_CycleState = CycleState::none;

    std::thread             m_Cycle_Thread;
    std::mutex              m_Cycle_Mutex;
    std::mutex              m_Cycle_StopMutex;
    std::condition_variable m_Cycle_StopCondition;

    void SetCycleState(CycleState nst);

    virtual bool OnCycleStop() {
        return true;
    }  // Called once Stop enters, setup parameters for cycle to stop, return
       // true if the operation should continue
    virtual bool OnCycle() { return true; }
};
}  // namespace ara