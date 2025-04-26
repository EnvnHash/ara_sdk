
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

#pragma once

#include <Conditional.h>
#include <util_common.h>

namespace ara {

class Cycler {
public:
    virtual ~Cycler() = default;

    enum class CycleState { none=0, starting, running, stopping, finished, err_failed };

    CycleState GetCycleState()  { return m_CycleState; }
    bool       IsRunning() { return (GetCycleState() == CycleState::running); }
    bool       Start();
    bool       Stop(bool async = false);
    void       Cycle();

private:
    void Cycle_Stop();

protected:
    std::atomic<CycleState> m_CycleState = CycleState::none;
    std::thread             m_Cycle_Thread;
    Conditional             m_stopCondition;

    void SetCycleState(CycleState nst) { m_CycleState = nst; }

    virtual bool OnCycleStop() {
        return true;
    }  // Called once Stop enters, setup parameters for cycle to stop, return
       // true if the operation should continue
    virtual bool OnCycle() { return true; }
};
}  // namespace ara