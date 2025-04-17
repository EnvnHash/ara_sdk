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

    for (int i = 0; i < (int)StereoEye::count; i++) {
        m_perspMat[i] = m_lensDist->getEyeProjectionMatrix(StereoEye::left, 0.1f, 100.f);

        m_transMat[i] = glm::translate(vec3{m_lensDist->getLensOffs((StereoEye)i), 0.f}) *
                        glm::scale(vec3{m_lensDist->getLensSize((StereoEye)i), 1.f});

        // if (m_vao[i].isInited()) m_vao[i].remove();
        // m_vao[i].init("position:2f,texCoord:2f");
        // m_vao[i].uploadMesh(&m_lensDist->getDistortionMesh((StereoEye)i));
    }
}

float *StereoRendererGeneral::getEyeLensTrans(StereoEye eye) { return &m_transMat[(int)eye][0][0]; }

}  // namespace ara