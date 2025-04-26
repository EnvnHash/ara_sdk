
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

#include "Cycler.h"

namespace ara {

bool Cycler::Start() {
    if (!(m_CycleState == CycleState::none || m_CycleState == CycleState::finished ||
          m_CycleState >= CycleState::err_failed)) {
        return false;
    }

    m_CycleState = CycleState::starting;

    m_Cycle_Thread = std::thread(&Cycler::Cycle, this);
    m_Cycle_Thread.detach();

    return true;
}

bool Cycler::Stop(bool asynch) {
    if (GetCycleState() != CycleState::running) {
        return false;
    }

    if (asynch) {
        auto sth = std::thread(&Cycler::Cycle_Stop, this);
        sth.detach();
    } else {
        Cycle_Stop();
    }

    return true;
}

void Cycler::Cycle() {
    SetCycleState(CycleState::running);
    SetCycleState(OnCycle() ? CycleState::finished : CycleState::err_failed);
    m_stopCondition.notify();
}

void Cycler::Cycle_Stop() {
    if (!OnCycleStop()) {
        return;
    }

    SetCycleState(CycleState::stopping);
    m_stopCondition.wait();
}

}