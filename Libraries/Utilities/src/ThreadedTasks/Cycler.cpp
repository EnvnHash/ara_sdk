#include "Cycler.h"

namespace ara {

bool Cycler::Start() {
    std::unique_lock<std::mutex> lck(m_Cycle_Mutex);

    if (!(m_CycleState == CycleState::none || m_CycleState == CycleState::finished ||
          m_CycleState >= CycleState::err_failed))
        return false;

    m_CycleState = CycleState::starting;

    m_Cycle_Thread = std::thread(&Cycler::Cycle, this);
    m_Cycle_Thread.detach();

    return true;
}

bool Cycler::Stop(bool asynch) {
    if (GetCycleState() != CycleState::running) return false;

    std::thread sth = std::thread(&Cycler::Cycle_Stop, this);

    if (asynch)
        sth.detach();
    else
        sth.join();

    return true;
}

Cycler::CycleState Cycler::GetCycleState() {
    std::unique_lock<std::mutex> lck(m_Cycle_Mutex);
    return m_CycleState;
}

void Cycler::SetCycleState(CycleState nst) {
    std::unique_lock<std::mutex> lck(m_Cycle_Mutex);
    m_CycleState = nst;
}

void Cycler::Cycle() {
    SetCycleState(CycleState::running);
    SetCycleState(OnCycle() ? CycleState::finished : CycleState::err_failed);
    m_Cycle_StopCondition.notify_all();
}

void Cycler::Cycle_Stop() {
    std::unique_lock<std::mutex> lck(m_Cycle_StopMutex);

    if (!OnCycleStop()) return;

    SetCycleState(CycleState::stopping);
    m_Cycle_StopCondition.wait(lck, [this] { return m_CycleState == CycleState::finished; });
}

}