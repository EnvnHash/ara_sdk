//  GUIAnimVal.h
//  tav_core
//
//  Created by Sven Hahne on 30/9/15.
//  last edit 24.09.2021
//

#pragma once

#include <util_common.h>

namespace ara {

enum class AnimFunc : int { RAMP_LIN_UP = 0, TRI_ONE_P };

class AnimValBase {
public:
    virtual void update() = 0;
    virtual void stop() = 0;
    virtual bool stopped() = 0;
    virtual AnimFunc getAnimFunc() = 0;
    virtual void setEndFunc(const std::function<void()>& _endFunc) = 0;
};

template <typename T>
class AnimVal : public AnimValBase {
public:
    AnimVal() : m_run(false), m_delay(0.0) {}

    explicit AnimVal(AnimFunc func, std::function<void()> endFunc = nullptr)
        : m_func(func), m_endFunc(std::move(endFunc)), m_run(false), m_delay(0.0) {}

    void update() override {
        if (m_run) {
            m_actTime = std::chrono::duration<double, std::milli>(std::chrono::system_clock::now() - m_startTp).count() * 0.001;
            if (m_dur != 0.0) {
                m_perc = static_cast<float>(std::min<double>(std::max<double>(m_actTime - m_delay, 0.0) / m_dur, 1.0));
                switch (m_func) {
                    case AnimFunc::RAMP_LIN_UP:
                        m_calcPerc = m_perc;
                        break;
                    case AnimFunc::TRI_ONE_P:
                        m_calcPerc = m_perc * 2.f;
                        if (m_calcPerc > 1.f) {
                            m_calcPerc = 2.f - m_calcPerc;
                        }
                        break;
                    default: 
                        break;
                }

                if (m_updtCb){
                    m_updtCb(getVal());
                }

                if (m_perc >= 1.0) {
                    m_run = false;

                    if (m_endFunc) {
                        m_endFunc();
                    }

                    if (m_loop) {
                        m_perc    = 0;
                        m_startTp = std::chrono::system_clock::now();
                        m_run     = true;
                    }
                }
            } else {
                LOG << "AnimVal Error: duration is 0";
                m_run = false;
            }
        }
    }

    void start(T startVal, T endVal, double dur, bool loop, std::function<void(T&)> updtCb = nullptr) {
        m_run      = true;
        m_startVal = startVal;
        m_endVal   = endVal;
        m_actTime  = 0.0;
        m_dur      = dur;
        m_loop     = loop;
        m_updtCb   = std::move(updtCb);
        m_startTp  = std::chrono::system_clock::now();
    }

    void stop() override {
        m_run = false;
        if (m_endFunc) {
            m_endFunc();
        }
    }

    bool                            stopped() override { return !m_run; }
    AnimFunc                        getAnimFunc() override { return m_func; }
    const std::function<void()>&    getEndFunc() { return m_endFunc; }
    float                           getPercentage() { return m_perc; }

    void reset() {
        m_perc     = 0.0;
        m_calcPerc = 0.0;
    }

    T &getVal() {
        m_blendVal = (m_startVal * (1.f - m_calcPerc)) + m_endVal * m_calcPerc;
        return m_blendVal;
    }

    bool isWaiting() {
        return m_perc == 0.f;
    }

    void setVal(T val) {
        m_startVal = val;
        m_run      = false;
        m_calcPerc = 0.0;
    }

    void setAnimFunc(AnimFunc f) { m_func = f; }
    void setInitVal(T val) { m_startVal = val; }
    void setDelay(double val) { m_delay = val; }
    void setEndFunc(const std::function<void()>& endFunc) override { m_endFunc = endFunc; }
    void setUpdtFunc(const std::function<void(T&)>& updtCb) { m_updtCb = updtCb; }

private:
    T                                                  m_startVal;
    T                                                  m_endVal;
    T                                                  m_blendVal;
    AnimFunc                                           m_func     = AnimFunc::RAMP_LIN_UP;
    bool                                               m_run      = false;
    bool                                               m_loop     = false;
    double                                             m_dur      = 0.0;
    double                                             m_delay    = 0.0;
    float                                              m_perc     = 0.f;
    float                                              m_calcPerc = 0.f;
    double                                             m_actTime  = 0.0;
    std::function<void()>                              m_endFunc;
    std::function<void(T &)>                           m_updtCb;
    std::chrono::time_point<std::chrono::system_clock> m_startTp;
};

}  // namespace ara