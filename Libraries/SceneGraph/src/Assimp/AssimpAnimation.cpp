//
//  AssimpAnimation.cpp
//

#ifdef ARA_USE_ASSIMP

#include "AssimpAnimation.h"

namespace ara {
AssimpAnimation::AssimpAnimation(const aiScene* _scene, aiAnimation* _animation)
    : m_scene(_scene), m_animation(_animation) {

    if (m_animation != nullptr) {
        m_durationInSeconds      = static_cast<float>(m_animation->mDuration);
        m_durationInMilliSeconds = static_cast<int>(m_durationInSeconds) * 1000;
    }
}

aiAnimation* AssimpAnimation::getAnimation() { return m_animation; }

void AssimpAnimation::update(double time) {
    m_animationPrevTime = m_animationCurrTime;
    m_animationCurrTime = static_cast<float>(time);
    double tps        = m_animation->mTicksPerSecond ? m_animation->mTicksPerSecond : 25.f;
    m_animationCurrTime *= static_cast<float>(tps);

    if (!m_play || m_pause) {
        return;
    }

    float duration     = getDurationInSeconds();
    float timeStep     = m_animationCurrTime - m_animationPrevTime;
    float positionStep = timeStep / (float)duration;
    float position     = getPosition() + positionStep;

    if (position > 1.0 && m_loopType == LOOP_NONE) {
        position = 1.0;
        stop();
    } else if (position > 1.0 && m_loopType == LOOP_NORMAL) {
        position = fmod(position, 1.0f);
    }

    setPosition(position);
}

void AssimpAnimation::updateAnimationNodes() {
    for (uint i = 0; i < m_animation->mNumChannels; i++) {
        const aiNodeAnim* channel    = m_animation->mChannels[i];
        aiNode*           targetNode = m_scene->mRootNode->FindNode(channel->mNodeName);

        aiVector3D presentPosition(0, 0, 0);
        if (channel->mNumPositionKeys > 0) {
            uint frame = 0;
            while (frame < channel->mNumPositionKeys - 1) {
                if (m_progressInSeconds < channel->mPositionKeys[frame + 1].mTime) {
                    break;
                }
                frame++;
            }

            uint               nextFrame = (frame + 1) % channel->mNumPositionKeys;
            const aiVectorKey& key       = channel->mPositionKeys[frame];
            const aiVectorKey& nextKey   = channel->mPositionKeys[nextFrame];
            double             diffTime  = nextKey.mTime - key.mTime;
            if (diffTime < 0.0) {
                diffTime += getDurationInSeconds();
            }
            if (diffTime > 0) {
                auto factor    = float((m_progressInSeconds - key.mTime) / diffTime);
                presentPosition = key.mValue + (nextKey.mValue - key.mValue) * factor;
            } else {
                presentPosition = key.mValue;
            }
        }

        aiQuaternion presentRotation(1, 0, 0, 0);
        if (channel->mNumRotationKeys > 0) {
            uint frame = 0;
            while (frame < channel->mNumRotationKeys - 1) {
                if (m_progressInSeconds < channel->mRotationKeys[frame + 1].mTime) {
                    break;
                }
                ++frame;
            }

            uint             nextFrame = (frame + 1) % channel->mNumRotationKeys;
            const aiQuatKey& key       = channel->mRotationKeys[frame];
            const aiQuatKey& nextKey   = channel->mRotationKeys[nextFrame];
            double           diffTime  = nextKey.mTime - key.mTime;

            if (diffTime < 0.0) {
                diffTime += getDurationInSeconds();
            }

            if (diffTime > 0) {
                auto factor = float((m_progressInSeconds - key.mTime) / diffTime);
                aiQuaternion::Interpolate(presentRotation, key.mValue, nextKey.mValue, factor);
            } else {
                presentRotation = key.mValue;
            }
        }

        aiVector3D presentScaling(1, 1, 1);
        if (channel->mNumScalingKeys > 0) {
            uint frame = 0;
            while (frame < channel->mNumScalingKeys - 1) {
                if (m_progressInSeconds < channel->mScalingKeys[frame + 1].mTime) {
                    break;
                }
                ++frame;
            }

            presentScaling = channel->mScalingKeys[frame].mValue;
        }

        aiMatrix4x4 mat = aiMatrix4x4(presentRotation.GetMatrix());
        mat.a1 *= presentScaling.x;
        mat.b1 *= presentScaling.x;
        mat.c1 *= presentScaling.x;
        mat.a2 *= presentScaling.y;
        mat.b2 *= presentScaling.y;
        mat.c2 *= presentScaling.y;
        mat.a3 *= presentScaling.z;
        mat.b3 *= presentScaling.z;
        mat.c3 *= presentScaling.z;
        mat.a4 = presentPosition.x;
        mat.b4 = presentPosition.y;
        mat.c4 = presentPosition.z;

        targetNode->mTransformation = mat;
    }
}

void AssimpAnimation::play() {
    if (m_animation == nullptr) {
        return;
    }
    if (m_play) {         // if already playing, ignore.
        m_pause = false;  // if paused, then unpause.
        return;
    }
    m_play  = true;
    m_pause = false;

    setPosition(0);
}

void AssimpAnimation::stop() {
    if (!m_play) {
        return;
    }
    m_play  = false;
    m_pause = false;
}

void AssimpAnimation::reset() { setPosition(0); }

bool AssimpAnimation::isFrameNew() const {
    return (m_play && !m_pause);  // assume its always a new frame when playing and not paused.
}

bool AssimpAnimation::isPaused() const {
    return m_pause;
}

bool AssimpAnimation::isPlaying() const {
    return m_play;
}

bool AssimpAnimation::isFinished() const {
    return !m_play && (getPosition() == 1.0);
}

float AssimpAnimation::getPosition() const {
    return m_progress;
}

float AssimpAnimation::getPositionInSeconds() const {
    return m_progressInSeconds;
}

int AssimpAnimation::getPositionInMilliSeconds() const {
    return m_progressInMilliSeconds;
}

float AssimpAnimation::getSpeed() const {
    return m_speed;
}

float AssimpAnimation::getDurationInSeconds() const {
    return m_durationInSeconds;
}

int AssimpAnimation::getDurationInMilliSeconds() const {
    return m_durationInMilliSeconds;
}

void AssimpAnimation::setPaused(bool paused) {
    m_pause = paused;
}

void AssimpAnimation::setPosition(float position) {
    position = glm::clamp(position, 0.0f, 1.0f);
    if (m_progress == position) {
        return;
    }
    m_progress               = position;
    m_progressInSeconds      = m_progress * getDurationInSeconds();
    m_progressInMilliSeconds = static_cast<int>(m_progress * getDurationInMilliSeconds());

    updateAnimationNodes();
}

void AssimpAnimation::setLoopState(enum loopType state) {
    m_loopType = state;
}

void AssimpAnimation::setSpeed(float speed) {
    speed = 1;
}

}  // namespace ara
#endif