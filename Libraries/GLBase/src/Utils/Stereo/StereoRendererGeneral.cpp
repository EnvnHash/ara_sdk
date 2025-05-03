//
// Created by sven on 18-10-22.
//

#include "StereoRendererGeneral.h"

#include "Utils/Stereo/DistortionMesh.h"
#include "Utils/Stereo/LensDistortion.h"

using namespace glm;
using namespace std;

namespace ara {

void StereoRendererGeneral::init(void *extData) {
    if (m_lensDist) m_lensDist.reset();

    m_lensDist = make_unique<LensDistortion>(m_devPar, m_scrPar);

    for (int i = 0; i < static_cast<int>(StereoEye::count); i++) {
        m_perspMat[i] = m_lensDist->getEyeProjectionMatrix(StereoEye::left, 0.1f, 100.f);

        m_transMat[i] = translate(vec3{m_lensDist->getLensOffs(static_cast<StereoEye>(i)), 0.f}) *
                        scale(vec3{m_lensDist->getLensSize(static_cast<StereoEye>(i)), 1.f});
    }
}

float *StereoRendererGeneral::getEyeLensTrans(StereoEye eye) {
    return &m_transMat[static_cast<int>(eye)][0][0];
}

}  // namespace ara