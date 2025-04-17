//
//  AssimpAnimation.cpp
//

#ifdef ARA_USE_ASSIMP

#include "AssimpAnimation.h"

namespace ara {
AssimpAnimation::AssimpAnimation(const aiScene* _scene, aiAnimation* _animation)
    : scene(_scene), animation(_animation) {
    animationCurrTime      = 0;
    animationPrevTime      = 0;
    bPlay                  = false;
    bPause                 = false;
    loopType               = LOOP_NONE;
    progress               = 0;
    progressInSeconds      = 0;
    progressInMilliSeconds = 0;
    durationInSeconds      = 0;
    durationInMilliSeconds = 0;
    speed                  = 1;

    if (animation != nullptr) {
        durationInSeconds      = (float)animation->mDuration;
        durationInMilliSeconds = (int)durationInSeconds * 1000;
    }
}

aiAnimation* AssimpAnimation::getAnimation() { return animation; }

void AssimpAnimation::update(double time) {
    animationPrevTime = animationCurrTime;
    animationCurrTime = (float)time;
    double tps        = animation->mTicksPerSecond ? animation->mTicksPerSecond : 25.f;
    animationCurrTime *= (float)tps;

    if (!bPlay || bPause) {
        return;
    }

    float duration     = getDurationInSeconds();
    float timeStep     = animationCurrTime - animationPrevTime;
    float positionStep = timeStep / (float)duration;
    float position     = getPosition() + positionStep;

    if (position > 1.0 && loopType == LOOP_NONE) {
        position = 1.0;
        stop();
    } else if (position > 1.0 && loopType == LOOP_NORMAL) {
        position = fmod(position, 1.0f);
    }

    setPosition(position);
}

void AssimpAnimation::updateAnimationNodes() {
    for (uint i = 0; i < animation->mNumChannels; i++) {
        const aiNodeAnim* channel    = animation->mChannels[i];
        aiNode*           targetNode = scene->mRootNode->FindNode(channel->mNodeName);

        aiVector3D presentPosition(0, 0, 0);
        if (channel->mNumPositionKeys > 0) {
            uint frame = 0;
            while (frame < channel->mNumPositionKeys - 1) {
                if (progressInSeconds < channel->mPositionKeys[frame + 1].mTime) {
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
                float factor    = float((progressInSeconds - key.mTime) / diffTime);
                presentPosition = key.mValue + (nextKey.mValue - key.mValue) * factor;
            } else {
                presentPosition = key.mValue;
            }
        }

        aiQuaternion presentRotation(1, 0, 0, 0);
        if (channel->mNumRotationKeys > 0) {
            uint frame = 0;
            while (frame < channel->mNumRotationKeys - 1) {
                if (progressInSeconds < channel->mRotationKeys[frame + 1].mTime) {
                    break;
                }
                frame++;
            }

            uint             nextFrame = (frame + 1) % channel->mNumRotationKeys;
            const aiQuatKey& key       = channel->mRotationKeys[frame];
            const aiQuatKey& nextKey   = channel->mRotationKeys[nextFrame];
            double           diffTime  = nextKey.mTime - key.mTime;
            if (diffTime < 0.0) {
                diffTime += getDurationInSeconds();
            }
            if (diffTime > 0) {
                float factor = float((progressInSeconds - key.mTime) / diffTime);
                aiQuaternion::Interpolate(presentRotation, key.mValue, nextKey.mValue, factor);
            } else {
                presentRotation = key.mValue;
            }
        }

        aiVector3D presentScaling(1, 1, 1);
        if (channel->mNumScalingKeys > 0) {
            uint frame = 0;
            while (frame < channel->mNumScalingKeys - 1) {
                if (progressInSeconds < channel->mScalingKeys[frame + 1].mTime) {
                    break;
                }
                frame++;
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
    if (animation == nullptr) {
        return;
    }
    if (bPlay) {         // if already playing, ignore.
        bPause = false;  // if paused, then unpause.
        return;
    }
    bPlay  = true;
    bPause = false;

    setPosition(0);
}

void AssimpAnimation::stop() {
    if (!bPlay) {
        return;
    }
    bPlay  = false;
    bPause = false;
}

void AssimpAnimation::reset() { setPosition(0); }

bool AssimpAnimation::isFrameNew() {
    return (bPlay && !bPause);  // assume its always a new frame when playing and not paused.
}

bool AssimpAnimation::isPaused() { return bPause; }

bool AssimpAnimation::isPlaying() { return bPlay; }

bool AssimpAnimation::isFinished() { return !bPlay && (getPosition() == 1.0); }

float AssimpAnimation::getPosition() { return progress; }

float AssimpAnimation::getPositionInSeconds() { return progressInSeconds; }

int AssimpAnimation::getPositionInMilliSeconds() { return progressInMilliSeconds; }

float AssimpAnimation::getSpeed() { return speed; }

float AssimpAnimation::getDurationInSeconds() { return durationInSeconds; }

int AssimpAnimation::getDurationInMilliSeconds() { return durationInMilliSeconds; }

void AssimpAnimation::setPaused(bool paused) { bPause = paused; }

void AssimpAnimation::setPosition(float position) {
    position = glm::clamp(position, 0.0f, 1.0f);
    if (progress == position) {
        return;
    }
    progress               = position;
    progressInSeconds      = progress * getDurationInSeconds();
    progressInMilliSeconds = static_cast<int>(progress * getDurationInMilliSeconds());

    updateAnimationNodes();
}

void AssimpAnimation::setLoopState(enum loopType state) { loopType = state; }

void AssimpAnimation::setSpeed(float speed) { speed = 1; }

AssimpAnimation::~AssimpAnimation() {}

}  // namespace ara
#endif