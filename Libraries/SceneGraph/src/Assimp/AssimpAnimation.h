//
//  AssimpAnimation.h
//  adapted from of original created by Lukasz Karluk on 4/12/12.
//

#pragma once

#ifdef ARA_USE_ASSIMP

#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glsg_common/glsg_common.h>

namespace ara {

class AssimpAnimation {
public:
    enum loopType { LOOP_NONE, LOOP_NORMAL };

    AssimpAnimation(const aiScene* _scene, aiAnimation* _animation);
    ~AssimpAnimation() = default;

    aiAnimation* getAnimation();

    void  update(double time);
    void  play();
    void  stop();
    void  reset();

    [[nodiscard]] bool  isFrameNew() const;
    [[nodiscard]] bool  isPaused() const;
    [[nodiscard]] bool  isPlaying() const;
    [[nodiscard]] bool  isFinished() const;
    [[nodiscard]] float getPosition() const;
    [[nodiscard]] float getPositionInSeconds() const;
    [[nodiscard]] int   getPositionInMilliSeconds() const;
    [[nodiscard]] float getSpeed() const;
    [[nodiscard]] float getDurationInSeconds() const;
    [[nodiscard]] int   getDurationInMilliSeconds() const;

    void  setPaused(bool paused);
    void  setPosition(float position);
    void  setLoopState(loopType state);
    static void  setSpeed(float speed);

protected:
    void updateAnimationNodes();

    std::shared_ptr<const aiScene> m_scene;
    aiAnimation*                   m_animation;
    float                          m_animationCurrTime{};
    float                          m_animationPrevTime{};
    bool                           m_play{};
    bool                           m_pause{};
    loopType                       m_loopType{};
    float                          m_progress{};
    float                          m_progressInSeconds{};
    int                            m_progressInMilliSeconds{};
    float                          m_durationInSeconds{};
    int                            m_durationInMilliSeconds{};
    float                          m_speed = 1.f;
};
}  // namespace ara
#endif