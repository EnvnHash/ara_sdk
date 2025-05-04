#pragma once

#include "GlbCommon/GlbCommon.h"

namespace ara {

unsigned int interp3Point(std::vector<glm::vec2>& inPoints, std::vector<glm::vec2>& outPoints, unsigned int ip_steps,
                          float u, unsigned short mode);
void ip3_procSegments(std::vector<glm::vec2*>& inPoints, std::vector<glm::vec2>& outPoints, unsigned int* stepNr,
                      float u, unsigned int nrSteps, unsigned short mode);
void ip3_subdiv(glm::vec2** ptrAr, float u);
// glm::vec2 mgm(glm::vec2 *p1, glm::vec2 *p2);
float mgm(glm::vec2* p);
void  ip3_subdiv_mgm(glm::vec2** ptrAr, float u);

}  // namespace ara
