#include <GlbCommon/GlbCommon.h>

using namespace std;
using namespace glm;

namespace ara {

quat RotationBetweenVectors(vec3 start, vec3 dest) {
    start = normalize(start);
    dest  = normalize(dest);

    float     cosTheta = dot(start, dest);
    vec3 rotationAxis;

    if (cosTheta < -1 + 0.001f) {
        // special case when vectors in opposite directions:
        // there is no "ideal" rotation axis
        // So guess one; any will do as long as it's perpendicular to start
        rotationAxis = cross(vec3(0.0f, 0.0f, 1.0f), start);
        if (length2(rotationAxis) < 0.01) { // bad luck, they were parallel, try again!
            rotationAxis = cross(vec3(1.0f, 0.0f, 0.0f), start);
        }
        rotationAxis = normalize(rotationAxis);
        return angleAxis(180.0f, rotationAxis);
    }

    rotationAxis = cross(start, dest);

    float s    = sqrt((1 + cosTheta) * 2);
    float invs = 1 / s;

    return {s * 0.5f, rotationAxis.x * invs, rotationAxis.y * invs, rotationAxis.z * invs};
}

void catmullRom(const std::vector<vec2> &inPoints, std::vector<vec2> &outPoints, unsigned int dstNrPoints) {
    outPoints.resize(dstNrPoints);
    float nrIntPointPerSeg = static_cast<float>(dstNrPoints) / static_cast<float>(inPoints.size() - 1);

    unsigned int offs = 0;
    unsigned int sum  = 0;
    unsigned int diff = 0;

    // go through each pair of points and interpolate
    for (size_t segNr = 0; segNr < inPoints.size() - 1; segNr++) {
        // correct the rounding errors from float to integer, add additional
        // integration steps when we are behind the exact destination index.
        sum += static_cast<int>(nrIntPointPerSeg) + diff;
        diff = static_cast<int>(static_cast<float>(segNr + 1) * nrIntPointPerSeg) - sum;

        for (unsigned int i = 0; i < static_cast<int>(nrIntPointPerSeg) + diff; i++) {
            float fInd = static_cast<float>(i) / (nrIntPointPerSeg + static_cast<float>(diff) - 1.f);

            if (segNr == 0) {
                // extrapolate point at the beginning by mirroring
                vec2 startP = (inPoints[0] - inPoints[1]) + inPoints[0];
                outPoints[offs] = catmullRom(startP, inPoints[segNr], inPoints[segNr + 1], inPoints[segNr + 2], fInd);
            } else if (segNr == inPoints.size() - 2) {
                // extrapolate point at the end by mirroring
                vec2 endP = (inPoints[inPoints.size() - 1] - inPoints[inPoints.size() - 2]) + inPoints[inPoints.size() - 1];
                outPoints[offs] = catmullRom(inPoints[segNr - 1], inPoints[segNr], inPoints[segNr + 1],
                                                  endP, fInd);
            } else {
                outPoints[offs] = catmullRom(inPoints[segNr - 1], inPoints[segNr], inPoints[segNr + 1],
                                                  inPoints[segNr + 2], fInd);
            }

            ++offs;
        }
    }
}

float perlinOct1D(float x, int octaves, float persistence) {
    float r    = 0.f;
    float a    = 1.f;
    float freq = 1.f;

    for (int i = 0; i < octaves; i++) {
        float fx = x * freq;
        r += perlin(vec2(fx, 0.f)) * a;
        //! update amplitude
        a *= persistence;
        freq = static_cast<float>(2 << i);
    }
    return r;
}

// index ranges from 0-1
vec4 linInterpolVec4(float inInd, const std::vector<vec4> *array) {
    vec4 outVal = vec4(0.f);
    if (!array->empty()) {
        auto  fArraySize = static_cast<float>(array->size());
        float fInd       = fmod(fmin(fmax(inInd, 0.f), 1.f), 1.0f) * (fArraySize - 1.0f);

        int   lowerInd = static_cast<int>(floor(fInd));
        int   upperInd = static_cast<int>(glm::min(lowerInd + 1.f, fArraySize - 1.f));
        float weight   = fInd - lowerInd;

        outVal = weight == 0.0 ? array->at(lowerInd) : mix(array->at(lowerInd), array->at(upperInd), weight);
    } else {
        LOGE << "linInterpolVec4() ERROR: array size = 0";
    }

    return outVal;
}

pair<bool, vec2> projPointToLine(vec2 point, vec2 l1Start, vec2 l1End) {
    vec2 out;
    bool      pointInsideLine;

    float dx = l1End.x - l1Start.x;
    float dy = l1End.y - l1Start.y;
    if (dx == 0) {
        out             = { l1Start.x, point.y };
        pointInsideLine = l1Start.y <= l1End.y ? (l1Start.y <= point.y && point.y <= l1End.y)
                                               : (l1End.y <= point.y && point.y <= l1Start.y);
        return make_pair(pointInsideLine, out);
    };
    if (dy == 0) {
        out             = { point.x, l1Start.y };
        pointInsideLine = l1Start.x <= l1End.x ? (l1Start.x <= point.x && point.x <= l1End.x)
                                               : (l1End.x <= point.x && point.x <= l1Start.x);
        return make_pair(pointInsideLine, out);
    }

    float m1 = (dy / dx);
    float c1 = l1Start.y - m1 * l1Start.x;  // which is same as y2 - slope * x2

    // get the line orthogonal to this one through the point
    float m2 = -(1.f / m1);
    float c2 = point.y - m2 * point.x;
    out.x = (c2 - c1) / (m1 - m2);
    out.y = m1 * out.x + c1;

    pointInsideLine = ((l1Start.x <= l1End.x) ? ((l1Start.x <= out.x) && (out.x <= l1End.x))
                                              : ((l1End.x <= out.x) && (out.x <= l1Start.x))) &&
                      ((l1Start.y <= l1End.y) ? ((l1Start.y <= out.y) && (out.y <= l1End.y))
                                              : ((l1End.y <= out.y) && (out.y <= l1Start.y)));

    return make_pair(pointInsideLine, out);
}

pair<bool, vec2> lineIntersect(vec2 l1Start, vec2 l1End, vec2 l2Start, vec2 l2End) {
    double m1 = 0, c1 = 0, m2 = 0, c2 = 0;
    vec2   intersection     = vec2(0.f, 0.f);
    std::array<std::array<vec2, 2>, 2> linePoints{};
    linePoints[0] = {l1Start, l1End};
    linePoints[1] = {l2Start, l2End};
    bool   isOnLine         = false;

    double dx1 = static_cast<double>(l1End.x) - static_cast<double>(l1Start.x);
    double dy1 = static_cast<double>(l1End.y) - static_cast<double>(l1Start.y);
    if (dx1 != 0.0) {
        m1 = dy1 / dx1;
        c1 = static_cast<double>(l1Start.y) - (double)m1 * static_cast<double>(l1Start.x);  // which is same as y2 - slope * x2
    }

    double dx2 = static_cast<double>(l2End.x) - static_cast<double>(l2Start.x);
    double dy2 = static_cast<double>(l2End.y) - static_cast<double>(l2Start.y);
    if (dx2 != 0.0) {  // line2 is parallel to y-axis
        m2 = dy2 / dx2;
        c2 = static_cast<double>(l2End.y) - m2 * static_cast<double>(l2End.x);  // which is same as y2 - slope * x2
    }

    // lines are parallel and both parallel to y-axis
    if (dx1 == 0 && dx2 == 0) return make_pair(false, vec2(0.f, 0.f));

    if ((dx1 != 0) && (dx2 != 0) && (m1 - m2) == 0) {
        // lines are parallel
        isOnLine = false;
    } else {
        // line1 parallel to y-axis
        if (dx1 == 0.0 && dx2 != 0.0) {
            intersection.x = static_cast<float>(l1Start.x);
            intersection.y = static_cast<float>(m2 * l1Start.x + c2);
        }
        // line2 parallel to y-axis
        else if (dx2 == 0.0 && dx1 != 0.0) {
            intersection.x = l2Start.x;
            intersection.y = static_cast<float>(m1 * l2Start.x + c1);
        } else {
            intersection.x = static_cast<float>((c2 - c1) / (m1 - m2));
            intersection.y = static_cast<float>(m1 * intersection.x + c1);
        }

        isOnLine = lineIntersectCheckOutlier(linePoints, intersection);
    }
    return make_pair(isOnLine, intersection);
}

bool lineIntersectCheckOutlier(std::array<std::array<vec2, 2>, 2>& linePoints, vec2& intersection) {
    bool onLine[2] = {false, false};

    // check if the intersection point lies outside the line end points
    for (size_t i = 0; i < 2; i++) {
        onLine[i] = true;
        for (auto j = 0; j < 2; j++) {
            double roundingError    = 0.00002f;
            bool smallerThanRoundingError = (std::abs(linePoints[i][0][j] - intersection[j]) < roundingError) ||
                                            (std::abs(linePoints[i][1][j] - intersection[j]) < roundingError);

            if (linePoints[i][0][j] < linePoints[i][1][j]) {
                onLine[i] = onLine[i] &&
                            ((linePoints[i][0][j] <= intersection[j] && intersection[j] <= linePoints[i][1][j]) ||
                             smallerThanRoundingError);
            } else {
                onLine[i] = onLine[i] &&
                            ((linePoints[i][1][j] <= intersection[j] && intersection[j] <= linePoints[i][0][j]) ||
                             smallerThanRoundingError);
            }
        }
    }

    return onLine[0] && onLine[1];
}

inline double Det2D(const vec2 &p1, const vec2 &p2, const vec2 &p3) {
    return +p1.x * (p2.y - p3.y) + p2.x * (p3.y - p1.y) + p3.x * (p1.y - p2.y);
}

void CheckTriWinding(const vec2 &p1, vec2 &p2, vec2 &p3, bool allowReversed) {
    double detTri = Det2D(p1, p2, p3);
    if (detTri < 0.0) {
        if (allowReversed) {
            vec2 a = p3;
            p3          = p2;
            p2          = a;
        } else
            throw std::runtime_error("triangle has wrong winding direction");
    }
}

bool BoundaryCollideChk(const vec2 &p1, const vec2 &p2, const vec2 &p3, double eps) { return Det2D(p1, p2, p3) < eps; }

bool BoundaryDoesntCollideChk(const vec2 &p1, const vec2 &p2, const vec2 &p3, double eps) {
    return Det2D(p1, p2, p3) <= eps;
}

bool TriTri2D(vec2 *t1, vec2 *t2, double eps = 0.0, bool allowReversed = false, bool onBoundary = true) {
    // Trangles must be expressed anti-clockwise
    CheckTriWinding(t1[0], t1[1], t1[2], allowReversed);
    CheckTriWinding(t2[0], t2[1], t2[2], allowReversed);

    bool (*chkEdge)(const vec2 &, const vec2 &, const vec2 &, double) = nullptr;
    if (onBoundary) {
        // Points on the boundary are considered as colliding
        chkEdge = BoundaryCollideChk;
    } else {
        // Points on the boundary are not considered as colliding
        chkEdge = BoundaryDoesntCollideChk;
    }

    // For edge E of trangle 1,
    for (int i = 0; i < 3; i++) {
        int j = (i + 1) % 3;

        // Check all points of trangle 2 lay on the external side of the edge E.
        // If they do, the triangles do not collide.
        if (chkEdge(t1[i], t1[j], t2[0], eps) && chkEdge(t1[i], t1[j], t2[1], eps) && chkEdge(t1[i], t1[j], t2[2], eps))
            return false;
    }

    // For edge E of trangle 2,
    for (int i = 0; i < 3; i++) {
        int j = (i + 1) % 3;

        // Check all points of trangle 1 lay on the external side of the edge E.
        // If they do, the triangles do not collide.
        if (chkEdge(t2[i], t2[j], t1[0], eps) && chkEdge(t2[i], t2[j], t1[1], eps) && chkEdge(t2[i], t2[j], t1[2], eps))
            return false;
    }

    // The triangles collide
    return true;
}

float distPointLine(vec2 _point, vec2 _lineP1, vec2 _lineP2, bool *projIsOutside, float *projAngle) {
    // return minimum distance between line segment P1P2 and point
    float l2 = length(_lineP2 - _lineP1);
    l2 *= l2;

    if (l2 == 0.f) {
        if (projIsOutside) {
            *projIsOutside = false;
        }
        return distance(_point, _lineP1);
    }

    if (projIsOutside) {
        *projIsOutside = true;
    }

    // consider the line extending the segment, parameterized as v + t (P2 - P1).
    // we find projection of point p onto the line if falls where t = [(p - P1) . (P2 - P1)] / |P2 - P1|^2
    float t = dot(_point - _lineP1, _lineP2 - _lineP1) / l2;

    /*
    if (projAngle){
        vec2 diff = (proj - _point);
        *projAngle = orientedAngle(vec2(1.f, 0.f),
    normalize(diff));
    }
*/
    if (t < 0.f) {
        return distance(_point, _lineP1);  // beyond the P1 End of Segment
    } else if (t > 1.f) {
        return distance(_point, _lineP2);  // beyond the P2 End of Segment
    }

    auto proj = _lineP1 + t * (_lineP2 - _lineP1);

    return distance(_point, proj);
}

float sign(vec2 p1, vec2 p2, vec2 p3) {
    return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
}

bool pointInTriangle(vec2 pt, vec2 v1, vec2 v2, vec2 v3) {
    float d1 = sign(pt, v1, v2);
    float d2 = sign(pt, v2, v3);
    float d3 = sign(pt, v3, v1);

    bool has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    bool has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

    return !(has_neg && has_pos);
}

float map(float value, float inputMin, float inputMax, float outputMin, float outputMax, bool clamp) {
    if (std::fabs(inputMin - inputMax) < FLT_EPSILON) {
        return outputMin;
    } else {
        float outVal = ((value - inputMin) / (inputMax - inputMin) * (outputMax - outputMin) + outputMin);

        if (clamp) {
            if (outputMax < outputMin) {
                if (outVal < outputMax)
                    outVal = outputMax;
                else if (outVal > outputMin)
                    outVal = outputMin;
            } else {
                if (outVal > outputMax)
                    outVal = outputMax;
                else if (outVal < outputMin)
                    outVal = outputMin;
            }
        }
        return outVal;
    }
}

// Function to generate a random point within a plane defined by three points
vec3 getRandomPointOnPlane(const vec3& base, const vec3& v0, const vec3& v1) {
    return base + v0 * getRandF(0.05f, 0.95f) + v1 * getRandF(0.05f, 0.95f);
}

float angleBetweenVectors(const vec3& a, const vec3& b) {
    // Calculate dot product of a and b
    float dotProduct = dot(a, b);

    // Calculate magnitudes of a and b
    float magnitudeA = length(a);
    float magnitudeB = length(b);

    // Calculate cos(theta)
    float cosTheta = dotProduct / (magnitudeA * magnitudeB);

    // Clamp the value to [-1, 1] due to floating-point arithmetic errors
    cosTheta = glm::clamp(cosTheta, -1.f, 1.f);

    // Calculate theta in radians
    return std::acos(cosTheta);
}

void decomposeMtx(const mat4 &m, vec3 &pos, quat &rot, vec3 &scale) {
    pos = m[3];
    for (int i = 0; i < 3; i++) {
        scale[i] = length(vec3(m[i]));
    }
    const mat3 rotMtx(vec3(m[0]) / scale[0], vec3(m[1]) / scale[1], vec3(m[2]) / scale[2]);
    rot = quat_cast(rotMtx);
}

void decomposeRot(const mat4 &m, quat &rot) {
    vec3 scale;
    for (int i = 0; i < 3; i++) {
        scale[i] = length(vec3(m[i]));
    }

    rot = quat_cast(mat3(vec3(m[0]) / scale[0], vec3(m[1]) / scale[1], vec3(m[2]) / scale[2]));
}

/** there is no glGetTexImage in GLES, so bind the texture to a FBO and use
 * glReadPixels */
void glesGetTexImage(GLuint textureObj, GLenum target, GLenum format, GLenum pixelType, int width, int height,
                     GLubyte *pixels) {
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, textureObj, 0);

    glReadPixels(0, 0, width, height, format, pixelType, pixels);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);
}

vector<vec2> get2DRing(int nrPoints) {
    vector<vec2> ringPos(nrPoints);

    for (int i = 0; i < nrPoints; i++) {
        // define a circle with n points
        float fInd = static_cast<float>(i) / static_cast<float>(nrPoints);

        // tip and cap
        ringPos[i].x = std::cos(fInd * static_cast<float>(M_PI) * 2.f);
        ringPos[i].y = std::sin(fInd * static_cast<float>(M_PI) * 2.f);
    }

    return ringPos;
}

bool initGLEW() {
#ifndef ARA_USE_GLES31
    glewExperimental = GL_TRUE;
    GLuint err       = glewInit();
    if (GLEW_OK != err) {
        LOGE << "Error couldn't init GLEW : " << glewGetErrorString(err);
        return false;
    }
    glGetError();  // delete glew standard error (bug in glew)
    return true;
#endif
    return true;
}

}  // namespace ara