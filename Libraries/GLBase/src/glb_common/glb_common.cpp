#include <glb_common/glb_common.h>

using namespace std;
using namespace glm;

namespace ara {

glm::quat RotationBetweenVectors(glm::vec3 start, glm::vec3 dest) {
    start = glm::normalize(start);
    dest  = glm::normalize(dest);

    float     cosTheta = glm::dot(start, dest);
    glm::vec3 rotationAxis;

    if (cosTheta < -1 + 0.001f) {
        // special case when vectors in opposite directions:
        // there is no "ideal" rotation axis
        // So guess one; any will do as long as it's perpendicular to start
        rotationAxis = glm::cross(glm::vec3(0.0f, 0.0f, 1.0f), start);
        if (glm::length2(rotationAxis) < 0.01)  // bad luck, they were parallel, try again!
            rotationAxis = glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), start);

        rotationAxis = glm::normalize(rotationAxis);
        return glm::angleAxis(180.0f, rotationAxis);
    }

    rotationAxis = glm::cross(start, dest);

    float s    = glm::sqrt((1 + cosTheta) * 2);
    float invs = 1 / s;

    return {s * 0.5f, rotationAxis.x * invs, rotationAxis.y * invs, rotationAxis.z * invs};
}

void catmullRom(const std::vector<glm::vec2> &inPoints, std::vector<glm::vec2> &outPoints, unsigned int dstNrPoints) {
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
                glm::vec2 startP = (inPoints[0] - inPoints[1]) + inPoints[0];
                outPoints[offs] =
                    glm::catmullRom(startP, inPoints[segNr], inPoints[segNr + 1], inPoints[segNr + 2], fInd);
            } else if (segNr == inPoints.size() - 2) {
                // extrapolate point at the end by mirroring
                glm::vec2 endP =
                    (inPoints[inPoints.size() - 1] - inPoints[inPoints.size() - 2]) + inPoints[inPoints.size() - 1];
                outPoints[offs] =
                    glm::catmullRom(inPoints[segNr - 1], inPoints[segNr], inPoints[segNr + 1], endP, fInd);
            } else {
                outPoints[offs] = glm::catmullRom(inPoints[segNr - 1], inPoints[segNr], inPoints[segNr + 1],
                                                  inPoints[segNr + 2], fInd);
            }

            offs++;
        }
    }
}

float perlinOct1D(float x, int octaves, float persistence) {
    float r    = 0.f;
    float a    = 1.f;
    float freq = 1.f;

    for (int i = 0; i < octaves; i++) {
        float fx = x * freq;

        r += glm::perlin(glm::vec2(fx, 0.f)) * a;

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
        int   upperInd = static_cast<int>(glm::min(lowerInd + 1.0f, fArraySize - 1.0f));
        float weight   = fInd - lowerInd;

        if (weight == 0.0)
            outVal = array->at(lowerInd);
        else
            outVal = glm::mix(array->at(lowerInd), array->at(upperInd), weight);
    } else {
        LOGE << "linInterpolVec4() ERROR: array size = 0";
    }

    return outVal;
}

pair<bool, vec2> projPointToLine(glm::vec2 point, glm::vec2 l1Start, glm::vec2 l1End) {
    glm::vec2 out;
    bool      pointInsideLine;

    float dx = l1End.x - l1Start.x;
    float dy = l1End.y - l1Start.y;
    if (dx == 0) {
        out             = glm::vec2(l1Start.x, point.y);
        pointInsideLine = l1Start.y <= l1End.y ? (l1Start.y <= point.y && point.y <= l1End.y)
                                               : (l1End.y <= point.y && point.y <= l1Start.y);
        return make_pair(pointInsideLine, out);
    };
    if (dy == 0) {
        out             = glm::vec2(point.x, l1Start.y);
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

pair<bool, vec2> lineIntersect(glm::vec2 l1Start, glm::vec2 l1End, glm::vec2 l2Start, glm::vec2 l2End) {
    double m1 = 0, c1 = 0, m2 = 0, c2 = 0;
    double dx1 = 0.0, dx2 = 0.0;
    vec2   intersection     = vec2(0.f, 0.f);
    vec2   linePoints[2][2] = {{l1Start, l1End}, {l2Start, l2End}};
    bool   isOnLine         = false;

    dx1 = static_cast<double>(l1End.x) - static_cast<double>(l1Start.x);
    double dy1 = static_cast<double>(l1End.y) - static_cast<double>(l1Start.y);
    if (dx1 != 0.0) {
        m1 = dy1 / dx1;
        c1 = static_cast<double>(l1Start.y) - (double)m1 * static_cast<double>(l1Start.x);  // which is same as y2 - slope * x2
    }

    dx2 = static_cast<double>(l2End.x) - static_cast<double>(l2Start.x);
    double dy2 = static_cast<double>(l2End.y) - static_cast<double>(l2Start.y);
    if (dx2 != 0.0) {  // line2 is parallel to y-axis
        m2 = dy2 / dx2;
        c2 = static_cast<double>(l2End.y) - m2 * static_cast<double>(l2End.x);  // which is same as y2 - slope * x2
    }

    // lines are parallel and both parallel to y-axis
    if (dx1 == 0 && dx2 == 0) return make_pair(false, glm::vec2(0.f, 0.f));

    if ((dx1 != 0) && (dx2 != 0) && (m1 - m2) == 0) {
        // lines are parallel
        isOnLine = false;
    } else {
        // line1 parallel to y-axis
        if (dx1 == 0.0 && dx2 != 0.0) {
            intersection.x = (float)l1Start.x;
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

        bool onLine[2] = {false, false};
        // check if the intersection point lies outside the line end points
        for (size_t i = 0; i < 2; i++) {
            onLine[i] = true;
            for (auto j = 0; j < 2; j++) {
                double roundingError    = 0.00002f;
                bool smallerThanRoundingError = (std::abs(linePoints[i][0][j] - intersection[j]) < roundingError) ||
                                                (std::abs(linePoints[i][1][j] - intersection[j]) < roundingError);

                if (linePoints[i][0][j] < linePoints[i][1][j])
                    onLine[i] = onLine[i] &&
                                ((linePoints[i][0][j] <= intersection[j] && intersection[j] <= linePoints[i][1][j]) ||
                                 smallerThanRoundingError);
                else
                    onLine[i] = onLine[i] &&
                                ((linePoints[i][1][j] <= intersection[j] && intersection[j] <= linePoints[i][0][j]) ||
                                 smallerThanRoundingError);
            }
        }

        isOnLine = onLine[0] && onLine[1];
    }
    return make_pair(isOnLine, intersection);
}

inline double Det2D(const glm::vec2 &p1, const glm::vec2 &p2, const glm::vec2 &p3) {
    return +p1.x * (p2.y - p3.y) + p2.x * (p3.y - p1.y) + p3.x * (p1.y - p2.y);
}

void CheckTriWinding(const glm::vec2 &p1, glm::vec2 &p2, glm::vec2 &p3, bool allowReversed) {
    double detTri = Det2D(p1, p2, p3);
    if (detTri < 0.0) {
        if (allowReversed) {
            glm::vec2 a = p3;
            p3          = p2;
            p2          = a;
        } else
            throw std::runtime_error("triangle has wrong winding direction");
    }
}

bool BoundaryCollideChk(const glm::vec2 &p1, const glm::vec2 &p2, const glm::vec2 &p3, double eps) { return Det2D(p1, p2, p3) < eps; }

bool BoundaryDoesntCollideChk(const glm::vec2 &p1, const glm::vec2 &p2, const glm::vec2 &p3, double eps) {
    return Det2D(p1, p2, p3) <= eps;
}

bool TriTri2D(glm::vec2 *t1, glm::vec2 *t2, double eps = 0.0, bool allowReversed = false, bool onBoundary = true) {
    // Trangles must be expressed anti-clockwise
    CheckTriWinding(t1[0], t1[1], t1[2], allowReversed);
    CheckTriWinding(t2[0], t2[1], t2[2], allowReversed);

    bool (*chkEdge)(const glm::vec2 &, const glm::vec2 &, const glm::vec2 &, double) = nullptr;
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

float distPointLine(glm::vec2 _point, glm::vec2 _lineP1, glm::vec2 _lineP2, bool *projIsOutside, float *projAngle) {
    // return minimum distance between line segment P1P2 and point
    float l2 = glm::length(_lineP2 - _lineP1);
    l2 *= l2;

    if (l2 == 0.f) {
        if (projIsOutside) {
            *projIsOutside = false;
        }
        return glm::distance(_point, _lineP1);
    }

    if (projIsOutside) {
        *projIsOutside = true;
    }

    // consider the line extending the segment, parameterized as v + t (P2 - P1).
    // we find projection of point p onto the line if falls where t = [(p - P1) . (P2 - P1)] / |P2 - P1|^2
    float t = glm::dot(_point - _lineP1, _lineP2 - _lineP1) / l2;

    /*
    if (projAngle){
        glm::vec2 diff = (proj - _point);
        *projAngle = glm::orientedAngle(glm::vec2(1.f, 0.f),
    glm::normalize(diff));
    }
*/
    if (t < 0.f) {
        return glm::distance(_point, _lineP1);  // beyond the P1 End of Segment
    } else if (t > 1.f) {
        return glm::distance(_point, _lineP2);  // beyond the P2 End of Segment
    }

    auto proj = _lineP1 + t * (_lineP2 - _lineP1);

    return glm::distance(_point, proj);
}

float sign(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3) {
    return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
}

bool pointInTriangle(glm::vec2 pt, glm::vec2 v1, glm::vec2 v2, glm::vec2 v3) {
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

void makeMatr(glm::mat4 *_matr, bool *_inited, float xOffs, float yOffs, float zOffs, float rotX, float rotY,
              float rotZ, float scaleX, float scaleY, float scaleZ) {
    /*
     glm::mat4 rotM, transScaleM, matrix;

     rotM = rotM.createRotationAroundAxis(
     rotX / 360.0f * TWO_PI,
     rotY / 360.0f * TWO_PI,
     rotZ / 360.0f * TWO_PI
     );
     transScaleM.setTranslation(xOffs, yOffs, zOffs);
     transScaleM.setScaling(scaleX, scaleY, scaleZ);
     rotM = rotM * transScaleM;

     *_matr = vmath::Matrix4f(rotM);

     if ( xOffs != 0.0f || yOffs != 0.0f || zOffs != 0.0f
     || rotX != 0.0f || rotY != 0.0f || rotZ != 0.0f
     || scaleX != 1.0f || scaleY != 1.0f || scaleZ != 1.0f
     )
     *_inited = true;
     */
}

// Function to generate a random point within a plane defined by three points
glm::vec3 getRandomPointOnPlane(const glm::vec3& base, const glm::vec3& v0, const glm::vec3& v1) {
    return base + v0 * getRandF(0.05f, 0.95f) + v1 * getRandF(0.05f, 0.95f);
}

float angleBetweenVectors(const glm::vec3& a, const glm::vec3& b) {
    // Calculate dot product of a and b
    float dotProduct = glm::dot(a, b);

    // Calculate magnitudes of a and b
    float magnitudeA = glm::length(a);
    float magnitudeB = glm::length(b);

    // Calculate cos(theta)
    float cosTheta = dotProduct / (magnitudeA * magnitudeB);

    // Clamp the value to [-1, 1] due to floating-point arithmetic errors
    cosTheta = glm::clamp(cosTheta, -1.0f, 1.0f);

    // Calculate theta in radians
    return std::acos(cosTheta);
}

double matrix_get_var(matrix* m, int row, int col) {
    if (row >= 0 && row < m->row && col >= 0 && col < m->col) {
        return m->var[row][col];
    } else {
        fprintf(stderr, "Matrix get_var: Index out of bounds\n");
        exit(EXIT_FAILURE);
    }
}

void matrix_set_var(matrix* m, int row, int col, double value) {
    if (row >= 0 && row < m->row && col >= 0 && col < m->col) {
        m->var[row][col] = value;
    } else {
        fprintf(stderr, "Matrix set_var: Index out of bounds\n");
        exit(EXIT_FAILURE);
    }
}

void fill_matrix_a(matrix* a, const double *x, const double *y, const double *_x, const double *_y) {
    for (int i = 0; i < 4; ++i) {
        matrix_set_var(a, i, 0, x[i]);
        matrix_set_var(a, i, 1, y[i]);
        matrix_set_var(a, i, 2, 1.0);
        matrix_set_var(a, i, 6, -_x[i] * x[i]);
        matrix_set_var(a, i, 7, -_x[i] * y[i]);
    }

    for (int i = 0; i < 4; ++i) {
        matrix_set_var(a, i + 4, 3, x[i]);
        matrix_set_var(a, i + 4, 4, y[i]);
        matrix_set_var(a, i + 4, 5, 1.0);
        matrix_set_var(a, i + 4, 6, -x[i] * _y[i]);
        matrix_set_var(a, i + 4, 7, -y[i] * _y[i]);
    }
}

void fill_matrix_b(matrix* b, const double *_x, const double *_y) {
    for (int i = 0; i < 4; ++i) {
        matrix_set_var(b, i, 0, _x[i]);
    }

    for (int i = 0; i < 4; ++i) {
        matrix_set_var(b, i + 4, 0, _y[i]);
    }
}

matrix* projection_matrix(const double *x, const double *y, const double *_x, const double *_y) {
    matrix *a = matrix_new(8, 8);
    matrix *b = matrix_new(8, 1);

    fill_matrix_a(a, x, y, _x, _y);
    fill_matrix_b(b, _x, _y);

    matrix *a_inv = matrix_inv(a);
    matrix *c = matrix_multiple(a_inv, b);

    matrix *projection = matrix_new(3, 3);
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            matrix_set_var(projection, i, j, matrix_get_var(c, i + j * 4, 0));
        }
    }
    matrix_set_var(projection, 2, 2, 1.0);

    matrix_free(a);
    matrix_free(b);
    matrix_free(c);
    matrix_free(a_inv);

    return projection;
}

glm::mat4 matrixToGlm(const matrix *_mat) {
    auto out = glm::mat4(1.f);
    for (short j = 0; j < 3; j++) {
        for (short i = 0; i < 3; i++) {
            out[i][j] = static_cast<float>(_mat->var[j][i]);
        }
    }

    return out;
}

GLenum postGLError(bool silence) {
    GLenum glErr = glGetError();

    if (glErr != GL_NO_ERROR && !silence) {
        switch (glErr) {
            case GL_INVALID_ENUM:
                LOGE << "\n GL ERROR: GL_INVALID_ENUM \n \n"; break;
            case GL_INVALID_VALUE:
                LOGE << "\n GL ERROR: GL_INVALID_VALUE, A numeric argument is "
                        "out of range  \n \n";
                break;
            case GL_INVALID_OPERATION:
                LOGE << "\n GL ERROR: GL_INVALID_OPERATION, The specified "
                        "operation is not allowed in the current state.  \n \n";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                LOGE << "\n GL ERROR: GL_INVALID_FRAMEBUFFER_OPERATION, The "
                        "framebuffer object is not complete. \n  \n";
                break;
#ifndef ARA_USE_GLES31
            case GL_STACK_OVERFLOW:
                LOGE << "\n GL ERROR: GL_STACK_OVERFLOW, Given when a stack "
                        "pushing operation cannot be done because it would "
                        "overflow the limit of that stack's size.  \n \n";
                break;
            case GL_STACK_UNDERFLOW:
                LOGE << "\n GL ERROR: GL_STACK_UNDERFLOW, Given when a stack "
                        "popping operation cannot be done because the stack is "
                        "already at its lowest point.  \n \n";
                break;
#endif
            case GL_OUT_OF_MEMORY: LOGE << "\n GL ERROR: GL_OUT_OF_MEMORY, GL_OUT_OF_MEMORY  \n \n"; break;
            default: LOGE << "\n GL ERROR \n \n"; break;
        }
        return glErr;
    }
    return GL_NO_ERROR;
}

#ifdef ARA_USE_EGL
std::string eglErrorString(EGLint err) {
    switch (err) {
        case EGL_SUCCESS: return "no error";
        case EGL_NOT_INITIALIZED: return "EGL not, or could not be, initialized";
        case EGL_BAD_ACCESS: return "access violation";
        case EGL_BAD_ALLOC: return "could not allocate resources";
        case EGL_BAD_ATTRIBUTE: return "invalid attribute";
        case EGL_BAD_CONTEXT: return "invalid context specified";
        case EGL_BAD_CONFIG: return "invald m_frame buffer configuration specified";
        case EGL_BAD_CURRENT_SURFACE:
            return "current window : return pbuffer or pixmap surface is no "
                   "longer valid";
        case EGL_BAD_DISPLAY: return "invalid display specified";
        case EGL_BAD_SURFACE: return "invalid surface specified";
        case EGL_BAD_MATCH: return "bad argument match";
        case EGL_BAD_PARAMETER: return "invalid paramater";
        case EGL_BAD_NATIVE_PIXMAP: return "invalid NativePixmap";
        case EGL_BAD_NATIVE_WINDOW: return "invalid NativeWindow";
        case EGL_CONTEXT_LOST: return "APM event caused context loss";
        default: return "unknown error " + std::to_string(err);
    }
    return "";
}
#endif

GLenum getExtType(GLenum inType) {
    GLenum extType = 0;

    switch (inType) {
        case GL_R8: extType = GL_RED; break;
        case GL_R8_SNORM: extType = GL_RED; break;
#ifndef ARA_USE_GLES31
        case GL_R16: extType = GL_RED; break;
        case GL_R16_SNORM: extType = GL_RED; break;
#endif
        case GL_RG8: extType = GL_RG; break;
        case GL_RG8_SNORM: extType = GL_RG; break;
#ifndef ARA_USE_GLES31
        case GL_RG16: extType = GL_RG; break;
        case GL_RG16_SNORM: extType = GL_RG; break;
        case GL_R3_G3_B2: extType = GL_RGB; break;
        case GL_RGB4: extType = GL_RGB; break;
        case GL_RGB5: extType = GL_RGB; break;
#endif
        case GL_RGB565: extType = GL_RGB; break;
        case GL_RGB8: extType = GL_RGB; break;
        case GL_RGB8_SNORM: extType = GL_RGB; break;
#ifndef ARA_USE_GLES31
        case GL_RGB10: extType = GL_RGB; break;
        case GL_RGB12: extType = GL_RGB; break;
        case GL_RGB16: extType = GL_RGB; break;
        case GL_RGB16_SNORM: extType = GL_RGB; break;
        case GL_RGBA2: extType = GL_RGBA; break;
#endif
        case GL_RGBA4: extType = GL_RGBA; break;
        case GL_RGB5_A1: extType = GL_RGBA; break;
        case GL_RGBA8:
            extType = GL_BGRA;  //??
            break;
        case GL_RGBA8_SNORM: extType = GL_RGBA; break;
        case GL_RGB10_A2: extType = GL_RGBA; break;
        case GL_RGB10_A2UI: extType = GL_RGBA; break;
#ifndef ARA_USE_GLES31
        case GL_RGBA12: extType = GL_RGBA; break;
        case GL_RGBA16: extType = GL_RGBA; break;
        case GL_RGBA16_SNORM: extType = GL_RGBA; break;
#endif
        case GL_SRGB8: extType = GL_RGB; break;
        case GL_SRGB8_ALPHA8: extType = GL_RGBA; break;
        case GL_R16F: extType = GL_RED; break;
        case GL_RG16F: extType = GL_RG; break;
        case GL_RGB16F: extType = GL_RGB; break;
        case GL_RGBA16F: extType = GL_RGBA; break;
        case GL_R32F: extType = GL_RED; break;
        case GL_RG32F: extType = GL_RG; break;
        case GL_RGB32F: extType = GL_RGB; break;
        case GL_RGBA32F: extType = GL_RGBA; break;
        case GL_R11F_G11F_B10F: extType = GL_RGB; break;
        case GL_RGB9_E5: extType = GL_RGB; break;
        case GL_R8I: extType = GL_RED_INTEGER; break;
        case GL_R8UI: extType = GL_RED_INTEGER; break;
        case GL_R16I: extType = GL_RED_INTEGER; break;
        case GL_R16UI: extType = GL_RED_INTEGER; break;
        case GL_R32I: extType = GL_RED_INTEGER; break;
        case GL_R32UI: extType = GL_RED_INTEGER; break;
        case GL_RG8I: extType = GL_RG_INTEGER; break;
        case GL_RG8UI: extType = GL_RG_INTEGER; break;
        case GL_RG16I: extType = GL_RG_INTEGER; break;
        case GL_RG16UI: extType = GL_RG_INTEGER; break;
        case GL_RG32I: extType = GL_RG_INTEGER; break;
        case GL_RG32UI: extType = GL_RG_INTEGER; break;
        case GL_RGB8I: extType = GL_RGB_INTEGER; break;
        case GL_RGB8UI: extType = GL_RGB_INTEGER; break;
        case GL_RGB16I: extType = GL_RGB_INTEGER; break;
        case GL_RGB16UI: extType = GL_RGB_INTEGER; break;
        case GL_RGB32I: extType = GL_RGB_INTEGER; break;
        case GL_RGB32UI: extType = GL_RGB_INTEGER; break;
        case GL_RGBA8I: extType = GL_RGBA_INTEGER; break;
        case GL_RGBA8UI: extType = GL_RGBA_INTEGER; break;
        case GL_RGBA16I: extType = GL_RGBA_INTEGER; break;
        case GL_RGBA16UI: extType = GL_RGBA_INTEGER; break;
        case GL_RGBA32I: extType = GL_RGBA_INTEGER; break;
        case GL_RGBA32UI: extType = GL_RGBA_INTEGER; break;
        case GL_DEPTH_COMPONENT24: extType = GL_DEPTH_COMPONENT; break;
        case GL_DEPTH24_STENCIL8: extType = GL_DEPTH_STENCIL; break;
        case GL_DEPTH32F_STENCIL8: extType = GL_DEPTH_STENCIL; break;
        case GL_DEPTH_COMPONENT32F: extType = GL_DEPTH_COMPONENT; break;
    }

    return extType;
}

GLenum getPixelType(GLenum inType) {
    GLenum format = 0;

    switch (inType) {
        case GL_R8: format = GL_UNSIGNED_BYTE; break;
        case GL_R8_SNORM: format = GL_UNSIGNED_BYTE; break;
#ifndef ARA_USE_GLES31
        case GL_R16: format = GL_UNSIGNED_SHORT; break;
        case GL_R16_SNORM: format = GL_UNSIGNED_SHORT; break;
#endif
        case GL_RG8: format = GL_UNSIGNED_BYTE; break;
        case GL_RG8_SNORM: format = GL_UNSIGNED_BYTE; break;
#ifndef ARA_USE_GLES31
        case GL_RG16: format = GL_UNSIGNED_SHORT; break;
        case GL_RG16_SNORM: format = GL_UNSIGNED_SHORT; break;
        case GL_R3_G3_B2: format = GL_UNSIGNED_BYTE; break;
        case GL_RGB4: format = GL_UNSIGNED_BYTE; break;
        case GL_RGB5: format = GL_UNSIGNED_BYTE; break;
        case GL_RGB565: format = GL_UNSIGNED_BYTE; break;
#endif
        case GL_RGB8: format = GL_UNSIGNED_BYTE; break;
        case GL_RGB8_SNORM: format = GL_UNSIGNED_BYTE; break;
#ifndef ARA_USE_GLES31
        case GL_RGB10: format = GL_UNSIGNED_BYTE; break;
        case GL_RGB12: format = GL_UNSIGNED_BYTE; break;
        case GL_RGB16: format = GL_UNSIGNED_SHORT; break;
        case GL_RGB16_SNORM: format = GL_UNSIGNED_SHORT; break;
        case GL_RGBA2: format = GL_UNSIGNED_BYTE; break;
#endif
        case GL_RGBA4: format = GL_UNSIGNED_BYTE; break;
        case GL_RGB5_A1: format = GL_UNSIGNED_BYTE; break;
        case GL_RGBA8: format = GL_UNSIGNED_BYTE; break;
        case GL_RGBA8_SNORM: format = GL_UNSIGNED_BYTE; break;
        case GL_RGB10_A2: format = GL_UNSIGNED_BYTE; break;
        case GL_RGB10_A2UI: format = GL_UNSIGNED_BYTE; break;
#ifndef ARA_USE_GLES31
        case GL_RGBA12: format = GL_UNSIGNED_BYTE; break;
        case GL_RGBA16: format = GL_UNSIGNED_SHORT; break;
        case GL_RGBA16_SNORM: format = GL_UNSIGNED_SHORT; break;
#endif
        case GL_SRGB8: format = GL_UNSIGNED_BYTE; break;
        case GL_SRGB8_ALPHA8: format = GL_UNSIGNED_BYTE; break;
        case GL_R16F: format = GL_HALF_FLOAT; break;
        case GL_RG16F: format = GL_HALF_FLOAT; break;
        case GL_RGB16F: format = GL_HALF_FLOAT; break;
        case GL_RGBA16F: format = GL_HALF_FLOAT; break;
        case GL_R32F: format = GL_FLOAT; break;
        case GL_RG32F: format = GL_FLOAT; break;
        case GL_RGB32F: format = GL_FLOAT; break;
        case GL_RGBA32F: format = GL_FLOAT; break;
        case GL_R11F_G11F_B10F: format = GL_FLOAT; break;
        case GL_RGB9_E5: format = GL_UNSIGNED_BYTE; break;
        case GL_R8I: format = GL_BYTE; break;
        case GL_R8UI: format = GL_UNSIGNED_BYTE; break;
        case GL_R16I: format = GL_SHORT; break;
        case GL_R16UI: format = GL_UNSIGNED_SHORT; break;
        case GL_R32I: format = GL_INT; break;
        case GL_R32UI: format = GL_UNSIGNED_INT; break;
        case GL_RG8I: format = GL_BYTE; break;
        case GL_RG8UI: format = GL_UNSIGNED_BYTE; break;
        case GL_RG16I: format = GL_SHORT; break;
        case GL_RG16UI: format = GL_UNSIGNED_SHORT; break;
        case GL_RG32I: format = GL_INT; break;
        case GL_RG32UI: format = GL_UNSIGNED_INT; break;
        case GL_RGB8I: format = GL_BYTE; break;
        case GL_RGB8UI: format = GL_UNSIGNED_BYTE; break;
        case GL_RGB16I: format = GL_SHORT; break;
        case GL_RGB16UI: format = GL_UNSIGNED_SHORT; break;
        case GL_RGB32I: format = GL_INT; break;
        case GL_RGB32UI: format = GL_UNSIGNED_INT; break;
        case GL_RGBA8I: format = GL_BYTE; break;
        case GL_RGBA8UI: format = GL_UNSIGNED_BYTE; break;
        case GL_RGBA16I: format = GL_SHORT; break;
        case GL_RGBA16UI: format = GL_UNSIGNED_SHORT; break;
        case GL_RGBA32I: format = GL_INT; break;
        case GL_RGBA32UI: format = GL_UNSIGNED_INT; break;
        case GL_DEPTH_COMPONENT24: format = GL_UNSIGNED_INT; break;
        case GL_DEPTH24_STENCIL8: format = GL_UNSIGNED_INT_24_8; break;
        case GL_DEPTH32F_STENCIL8: format = GL_FLOAT_32_UNSIGNED_INT_24_8_REV; break;
        case GL_DEPTH_COMPONENT32F: format = GL_FLOAT; break;
    }

    return format;
}

short getNrColChans(GLenum internalType) {
    switch (internalType) {
        case GL_R8: return 1;
        case GL_R8_SNORM: return 1;
#ifndef ARA_USE_GLES31
        case GL_R16: return 1;
        case GL_R16_SNORM: return 1;
#endif
        case GL_RG8: return 2;
        case GL_RG8_SNORM: return 2;
#ifndef ARA_USE_GLES31
        case GL_RG16: return 2;
        case GL_RG16_SNORM: return 2;
        case GL_R3_G3_B2: return 3;
        case GL_RGB4: return 3;
        case GL_RGB5: return 3;
#endif
        case GL_RGB565: return 3;
        case GL_RGB8: return 3;
        case GL_RGB8_SNORM: return 3;
#ifndef ARA_USE_GLES31
        case GL_RGB10: return 3;
        case GL_RGB12: return 3;
        case GL_RGB16: return 3;
        case GL_RGB16_SNORM: return 3;
        case GL_RGBA2: return 4;
#endif
        case GL_RGBA4: return 4;
        case GL_RGB5_A1: return 4;
        case GL_RGBA8: return 4;
        case GL_RGBA8_SNORM: return 4;
        case GL_RGB10_A2: return 4;
        case GL_RGB10_A2UI: return 4;
#ifndef ARA_USE_GLES31
        case GL_RGBA12: return 4;
        case GL_RGBA16: return 4;
        case GL_RGBA16_SNORM: return 4;
#endif
        case GL_SRGB8: return 3;
        case GL_SRGB8_ALPHA8: return 4;
        case GL_R16F: return 1;
        case GL_RG16F: return 2;
        case GL_RGB16F: return 3;
        case GL_RGBA16F: return 4;
        case GL_R32F: return 1;
        case GL_RG32F: return 2;
        case GL_RGB32F: return 3;
        case GL_RGBA32F: return 4;
        case GL_R11F_G11F_B10F: return 3;
        case GL_RGB9_E5: return 4;
        case GL_R8I: return 1;
        case GL_R8UI: return 1;
        case GL_R16I: return 1;
        case GL_R16UI: return 1;
        case GL_R32I: return 1;
        case GL_R32UI: return 1;
        case GL_RG8I: return 2;
        case GL_RG8UI: return 2;
        case GL_RG16I: return 2;
        case GL_RG16UI: return 2;
        case GL_RG32I: return 2;
        case GL_RG32UI: return 2;
        case GL_RGB8I: return 3;
        case GL_RGB8UI: return 3;
        case GL_RGB16I: return 3;
        case GL_RGB16UI: return 3;
        case GL_RGB32I: return 3;
        case GL_RGB32UI: return 3;
        case GL_RGBA8I: return 4;
        case GL_RGBA8UI: return 4;
        case GL_RGBA16I: return 4;
        case GL_RGBA16UI: return 4;
        case GL_RGBA32I: return 4;
        case GL_RGBA32UI: return 4;
        default: break;
    }
    return 0;
}

uint getBitCount(GLenum inType) {
    switch (inType) {
        case GL_R8: return 8;
        case GL_R8_SNORM: return 8;
#ifndef ARA_USE_GLES31
        case GL_R16: return 16;
        case GL_R16_SNORM: return 16;
#endif
        case GL_RG8: return 16;
        case GL_RG8_SNORM: return 16;
#ifndef ARA_USE_GLES31
        case GL_RG16: return 32;
        case GL_RG16_SNORM: return 32;
        case GL_R3_G3_B2: return 8;
        case GL_RGB4: return 12;
        case GL_RGB5: return 15;
#endif
        case GL_RGB565: return 16;
        case GL_RGB8: return 24;
        case GL_RGB8_SNORM: return 24;
#ifndef ARA_USE_GLES31
        case GL_RGB10: return 30;
        case GL_RGB12: return 36;
        case GL_RGB16: return 48;
        case GL_RGB16_SNORM: return 48;
        case GL_RGBA2: return 26;
#endif
        case GL_RGBA4: return 28;
        case GL_RGB5_A1: return 16;
        case GL_RGBA8: return 32;
        case GL_RGBA8_SNORM: return 32;
        case GL_RGB10_A2: return 32;
        case GL_RGB10_A2UI: return 32;
#ifndef ARA_USE_GLES31
        case GL_RGBA12: return 48;
        case GL_RGBA16: return 64;
        case GL_RGBA16_SNORM: return 64;
#endif
        case GL_SRGB8: return 24;
        case GL_SRGB8_ALPHA8: return 32;
        case GL_R16F: return 16;
        case GL_RG16F: return 32;
        case GL_RGB16F: return 48;
        case GL_RGBA16F: return 64;
        case GL_R32F: return 32;
        case GL_RG32F: return 64;
        case GL_RGB32F: return 96;
        case GL_RGBA32F: return 128;
        case GL_R11F_G11F_B10F: return 32;
        case GL_RGB9_E5: return 32;
        case GL_R8I: return 8;
        case GL_R8UI: return 8;
        case GL_R16I: return 16;
        case GL_R16UI: return 16;
        case GL_R32I: return 32;
        case GL_R32UI: return 32;
        case GL_RG8I: return 16;
        case GL_RG8UI: return 16;
        case GL_RG16I: return 32;
        case GL_RG16UI: return 32;
        case GL_RG32I: return 64;
        case GL_RG32UI: return 64;
        case GL_RGB8I: return 24;
        case GL_RGB8UI: return 24;
        case GL_RGB16I: return 48;
        case GL_RGB16UI: return 48;
        case GL_RGB32I: return 96;
        case GL_RGB32UI: return 96;
        case GL_RGBA8I: return 32;
        case GL_RGBA8UI: return 32;
        case GL_RGBA16I: return 64;
        case GL_RGBA16UI: return 64;
        case GL_RGBA32I: return 128;
        case GL_RGBA32UI: return 128;
        default: break;
    }
    return 0;
}

void decomposeMtx(const glm::mat4 &m, glm::vec3 &pos, glm::quat &rot, glm::vec3 &scale) {
    pos = m[3];
    for (int i = 0; i < 3; i++) scale[i] = glm::length(vec3(m[i]));
    const glm::mat3 rotMtx(glm::vec3(m[0]) / scale[0], glm::vec3(m[1]) / scale[1], glm::vec3(m[2]) / scale[2]);
    rot = glm::quat_cast(rotMtx);
}

void decomposeRot(const glm::mat4 &m, glm::quat &rot) {
    vec3 scale;
    for (int i = 0; i < 3; i++) scale[i] = glm::length(vec3(m[i]));

    rot = glm::quat_cast(glm::mat3(glm::vec3(m[0]) / scale[0], glm::vec3(m[1]) / scale[1], glm::vec3(m[2]) / scale[2]));
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

vector<GLfloat> get2DRing(int nrPoints) {
    vector<GLfloat> ringPos(nrPoints * 2);

    for (int i = 0; i < nrPoints; i++) {
        // define a circle with n points
        float fInd = static_cast<float>(i) / static_cast<float>(nrPoints);
        float x = std::cos(fInd * static_cast<float>(M_PI) * 2.f);
        float z = std::sin(fInd * static_cast<float>(M_PI) * 2.f);

        // tip and cap
        ringPos[i * 2]     = x;
        ringPos[i * 2 + 1] = z;
    }

    return ringPos;
}

bool initGLEW() {
#ifndef ARA_USE_GLES31
    glewExperimental = GL_TRUE;
    GLuint err       = glewInit();
    if (GLEW_OK != err) {
        fprintf(stderr, "Error couldn't init GLEW : %s\n", glewGetErrorString(err));
        return false;
    }
    glGetError();  // delete glew standard error (bug in glew)
    return true;
#endif
    return true;
}

}  // namespace ara