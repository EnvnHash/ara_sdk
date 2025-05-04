#include "glsg_common/three_point_interp.h"

#include "GlbCommon/GlbCommon.h"

namespace ara {

// u is the tension parameter should be around 1/3
// ip-steps are the times the input array will be subdivided
unsigned int interp3Point(std::vector<glm::vec2> &inPoints, std::vector<glm::vec2> &outPoints, unsigned int ip_steps,
                          float u, unsigned short mode) {
    // we need at least 3 points
    if (inPoints.size() > 2) {
        // calculate the number of total number of points that will result
        // after all interpolation steps. Each step the new number of points is
        // 2(n-1) +n = 3n+2
        unsigned int totNrPoints = (unsigned int)inPoints.size();
        for (unsigned int i = 0; i < ip_steps; i++) totNrPoints = 3 * totNrPoints - 2;

        // printf("nrInPoints: %d ip_steps: %d, totNrPoints: %d \n",
        // inPoints.size(), ip_steps, totNrPoints);

        // resize the destination array to the necessary number of points
        outPoints.resize(totNrPoints);

        // create a vector with pointer to the actual points to process
        std::vector<glm::vec2 *> procPtr;

        // copy the existing points into the outPoints array
        unsigned int offset = totNrPoints / static_cast<unsigned int>(inPoints.size() - 1);
        for (unsigned int i = 0; i < inPoints.size(); i++) {
            outPoints[i * offset] = glm::vec2(inPoints[i]);
            procPtr.push_back(&outPoints[i * offset]);
        }

        // this interpolation algorithm by definition always takes three points
        // as input each consecutive three points (p1,p2,p3) of the input array
        // will be called a segment to calculate the first and last segment a
        // pseudo point has to be extrapolated here we will do it by mirroring
        // p2 by p1 for the first segment and p2 by p3 for the last segment
        unsigned int stepNr = 1;
        ip3_procSegments(procPtr, outPoints, &stepNr, u, ip_steps, mode);

        return totNrPoints;
    } else {
        LOGE << "interp3Point Error: Need at least 3 input points";
        return 0;
    }
}

void ip3_procSegments(std::vector<glm::vec2 *> &inPoints, std::vector<glm::vec2> &outPoints, unsigned int *stepNr,
                      float u, unsigned int nrSteps, unsigned short mode) {
    // we need to know the distance in indices between the newly generated
    // entries
    unsigned int indStep = (unsigned int)pow(3, nrSteps - *stepNr);

    // nr of segments in a point array to process is equal to the size of the
    // array
    for (size_t i = 0; i < inPoints.size(); i++) {
        // create an array with five positions. [0], [2] and [4] are the points
        // to interpolate [1] [2] will be the result

        unsigned int outPoint1Ind = (unsigned int)(indStep * (3 * i - 1));
        unsigned int outPoint2Ind = (unsigned int)(indStep * (3 * i + 1));

        if (i == 0) {
            // if we are dealing with the first segment a pseudo point has to be
            // extrapolated at the beginning, we just mirror p2 on p1 to get p0

            glm::vec2  pseudo_p_start = (*inPoints[0] - *inPoints[1]) + *inPoints[0];
            glm::vec2 *ptrAr[5]       = {&pseudo_p_start, nullptr, inPoints[0], &outPoints[outPoint2Ind], inPoints[1]};
            mode == 0 ? ip3_subdiv(ptrAr, u) : ip3_subdiv_mgm(ptrAr, u);
        } else if (i == (inPoints.size() - 1)) {
            // if we are dealing with the last segment a pseudo point has to be
            // extrapolated at the end, we just mirror p0 on p1 to get p2

            glm::vec2 pseudo_p_end =
                (*inPoints[inPoints.size() - 1] - *inPoints[inPoints.size() - 2]) + *inPoints[inPoints.size() - 1];
            glm::vec2 *ptrAr[5] = {inPoints[i - 1], &outPoints[outPoint1Ind], inPoints[i], nullptr, &pseudo_p_end};
            mode == 0 ? ip3_subdiv(ptrAr, u) : ip3_subdiv_mgm(ptrAr, u);
        } else {
            glm::vec2 *ptrAr[5] = {inPoints[i - 1], &outPoints[outPoint1Ind], inPoints[i], &outPoints[outPoint2Ind],
                                   inPoints[i + 1]};
            mode == 0 ? ip3_subdiv(ptrAr, u) : ip3_subdiv_mgm(ptrAr, u);
        }
    }

    // iterate
    (*stepNr)++;
    if (*stepNr <= nrSteps) {
        // prepare the inPoints, now we got 3n -2 points
        size_t newSize = 3 * inPoints.size() - 2;

        inPoints.resize(0);

        // build new input arrays
        for (size_t i = 0; i < newSize; i++) {
            inPoints.push_back(&outPoints[i * indStep]);
        }

        ip3_procSegments(inPoints, outPoints, stepNr, u, nrSteps, mode);
    }
}

// takes as input an array with pointers to exactly 5 points (glm::vec4)
// m_ptr[0] m_ptr[2] and m_ptr[4] should correspond to the source points to
// interpolate (these are not touched, the resulting two new points will be
// written to position [1] and [3]
//
// [0]		 [2]	   [4]
//  |         |         |
//  v		  v         v
// [0]  [1]  [2]  [3]  [4]
void ip3_subdiv(glm::vec2 **ptrAr, float u) {
    if (ptrAr[1]) {
        *ptrAr[1] = u * *ptrAr[0] + (FOUR_THIRDS - 2.f * u) * *ptrAr[2] + (u - ONE_THIRD) * *ptrAr[4];
    }

    if (ptrAr[3]) {
        *ptrAr[3] = (-ONE_THIRD + u) * *ptrAr[0] + (FOUR_THIRDS - 2.f * u) * *ptrAr[2] + u * *ptrAr[4];
    }
}

// calculate the modified geometric mean
float mgm(glm::vec2 *p) {
    float sum = p->x * p->y;
    if (sum > 0) {
        return (p->x >= 0.f ? 1.f : 0.f) * std::sqrt(p->x * p->y);
    } else {
        return 0.f;
    }
}

void ip3_subdiv_mgm(glm::vec2 **ptrAr, float u) {
    glm::vec2 dfp0       = *ptrAr[4] - *ptrAr[2];
    glm::vec2 dfpIminOne = *ptrAr[2] - *ptrAr[0];
    glm::vec2 mean       = (dfp0 - dfpIminOne) * 0.5f;

    if (ptrAr[1]) {
        *ptrAr[1] =
            (2.f * u - ONE_THIRD) * *ptrAr[0] + (FOUR_THIRDS - 2.f * u) * *ptrAr[2] + 2.f * (u - ONE_THIRD) * mean;
    }

    if (ptrAr[3]) {
        *ptrAr[3] =
            (FOUR_THIRDS - 2.f * u) * *ptrAr[2] + (2.f * u - ONE_THIRD) * *ptrAr[4] - 2.f * (u - ONE_THIRD) * mean;
    }
}

}  // namespace ara
