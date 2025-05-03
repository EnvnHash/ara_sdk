//
//  SkyBoxBlend.cpp
//  Tav_App
//
//  Created by Sven Hahne on 18/3/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "SkyBoxBlend.h"
#include "GeoPrimitives/Sphere.h"
#include "SceneNodes/SceneNode.h"
#include "Shaders/Shaders.h"
#include "CameraSets/CameraSet.h"
#include "Utils/TFO.h"
#include "Utils/Texture.h"


using namespace glm;
using namespace std;

namespace ara {

SkyBoxBlend::SkyBoxBlend(const std::string &textureFile, unsigned nrCams, sceneData* sd) {
    m_vShader = STRINGIFY(layout(location = 0) in vec4 position;
        out vec4 pos;
        void main() {
            pos = position;
        });
    m_vShader = ShaderCollector::getShaderHeader() + m_vShader;

    m_gShader = STRINGIFY(in vec4 pos[]; out vec3 tex_coord; out vec3 tex_coord_2; out vec3 tex_coord_3;
                        uniform mat4 tc_rot; uniform mat4 tc_rot2; uniform mat4 tc_rot3; void main() {
                            gl_ViewportIndex = gl_InvocationID;
                            for (int i = 0; i < gl_in.length(); i++) {
                                tex_coord   = normalize((tc_rot * pos[i]).xyz);
                                tex_coord_2 = normalize((tc_rot2 * pos[i]).xyz);
                                tex_coord_3 = normalize((tc_rot3 * pos[i]).xyz);
                                gl_Position = m_pvm[gl_InvocationID] * pos[i];
                                EmitVertex();
                            }
                            EndPrimitive();
                        });

    std::string gHeader = ShaderCollector::getShaderHeader();
    gHeader += "layout(triangles, invocations=" + std::to_string(nrCams) + ") in;\n";
    gHeader += "layout(triangle_strip, max_vertices=3) out;\n";
    gHeader += "uniform mat4 m_pvm[" + std::to_string(nrCams) + "];\n";
    m_gShader = gHeader + m_gShader;

    m_fShader = STRINGIFY(
        uniform vec2 noiseScale; uniform float width; uniform float height; uniform float time; uniform float time2;
        in vec3 tex_coord; in vec3 tex_coord_2; in vec3 tex_coord_3; layout(location = 0) out vec4 color;
        uniform samplerCube                                                                        tex;

        int i; int nrIts = 1; float pi = 3.1415926535897932384626433832795; float newNoise = 0.0;

        vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }

        vec4 mod289(vec4 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }

        vec4 permute(vec4 x) { return mod289(((x * 34.0) + 1.0) * x); }

        vec4 taylorInvSqrt(vec4 r) { return 1.79284291400159 - 0.85373472095314 * r; }

        vec3 fade(vec3 t) { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); }

        // Classic Perlin noise
        float cnoise(vec3 P) {
            vec3 Pi0 = floor(P);         // Integer part for indexing
            vec3 Pi1 = Pi0 + vec3(1.0);  // Integer part + 1
            Pi0      = mod289(Pi0);
            Pi1      = mod289(Pi1);
            vec3 Pf0 = fract(P);         // Fractional part for interpolation
            vec3 Pf1 = Pf0 - vec3(1.0);  // Fractional part - 1.0
            vec4 ix  = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
            vec4 iy  = vec4(Pi0.yy, Pi1.yy);
            vec4 iz0 = Pi0.zzzz;
            vec4 iz1 = Pi1.zzzz;

            vec4 ixy  = permute(permute(ix) + iy);
            vec4 ixy0 = permute(ixy + iz0);
            vec4 ixy1 = permute(ixy + iz1);

            vec4 gx0 = ixy0 * (1.0 / 7.0);
            vec4 gy0 = fract(floor(gx0) * (1.0 / 7.0)) - 0.5;
            gx0      = fract(gx0);
            vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
            vec4 sz0 = step(gz0, vec4(0.0));
            gx0 -= sz0 * (step(0.0, gx0) - 0.5);
            gy0 -= sz0 * (step(0.0, gy0) - 0.5);

            vec4 gx1 = ixy1 * (1.0 / 7.0);
            vec4 gy1 = fract(floor(gx1) * (1.0 / 7.0)) - 0.5;
            gx1      = fract(gx1);
            vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
            vec4 sz1 = step(gz1, vec4(0.0));
            gx1 -= sz1 * (step(0.0, gx1) - 0.5);
            gy1 -= sz1 * (step(0.0, gy1) - 0.5);

            vec3 g000 = vec3(gx0.x, gy0.x, gz0.x);
            vec3 g100 = vec3(gx0.y, gy0.y, gz0.y);
            vec3 g010 = vec3(gx0.z, gy0.z, gz0.z);
            vec3 g110 = vec3(gx0.w, gy0.w, gz0.w);
            vec3 g001 = vec3(gx1.x, gy1.x, gz1.x);
            vec3 g101 = vec3(gx1.y, gy1.y, gz1.y);
            vec3 g011 = vec3(gx1.z, gy1.z, gz1.z);
            vec3 g111 = vec3(gx1.w, gy1.w, gz1.w);

            vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
            g000 *= norm0.x;
            g010 *= norm0.y;
            g100 *= norm0.z;
            g110 *= norm0.w;
            vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
            g001 *= norm1.x;
            g011 *= norm1.y;
            g101 *= norm1.z;
            g111 *= norm1.w;

            float n000 = dot(g000, Pf0);
            float n100 = dot(g100, vec3(Pf1.x, Pf0.yz));
            float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z));
            float n110 = dot(g110, vec3(Pf1.xy, Pf0.z));
            float n001 = dot(g001, vec3(Pf0.xy, Pf1.z));
            float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z));
            float n011 = dot(g011, vec3(Pf0.x, Pf1.yz));
            float n111 = dot(g111, Pf1);

            vec3  fade_xyz = fade(Pf0);
            vec4  n_z      = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z);
            vec2  n_yz     = mix(n_z.xy, n_z.zw, fade_xyz.y);
            float n_xyz    = mix(n_yz.x, n_yz.y, fade_xyz.x);
            return 2.2 * n_xyz;
        }

        float noiseC  = 0.0;
        float noiseC2 = 0.0; const vec3 LumCoeff = vec3(0.2125, 0.7154, 0.0721);
        const vec3                      AvgLumin = vec3(0.5, 0.5, 0.5);

        void main() {
            vec2 posCo = vec2(gl_FragCoord) / vec2(width, height) * noiseScale;

            noiseC = (cnoise(vec3(posCo * 32.0, time)) + 1.0) * 0.5;
            noiseC = min(max(pow(noiseC, 2.0) * 1.5, 0.0), 1.0);

            noiseC2 = (cnoise(vec3(posCo * 16.0, time2)) + 1.0) * 0.5;
            noiseC2 = min(max(pow(noiseC2, 2.0) * 1.5, 0.0), 1.0);
            // color = vec4(noiseC, noiseC, noiseC, 1.0);

            vec4 tex0 = texture(tex, tex_coord);
            vec4 tex2 = texture(tex, tex_coord_2);
            vec4 tex3 = texture(tex, tex_coord_3);

            vec4 upperLayer = tex0 * noiseC + tex2 * (1.f - noiseC);
            upperLayer      = upperLayer * noiseC2 + tex3 * (1.f - noiseC2);

            vec3 intensity = vec3(dot(upperLayer.rgb, LumCoeff));

            // could substitute a uniform for this 1. and have variable
            // saturation
            vec3 satColor = mix(intensity, upperLayer.rgb, 1.);
            vec3 conColor = mix(AvgLumin, satColor, 1.2);  // constrast

            color = vec4(conColor, 1);
        });
    m_fShader = ShaderCollector::getShaderHeader() + m_fShader;

    m_sbShader = sd->glbase->shaderCollector().add("SkyBoxBlend", m_vShader, m_gShader, m_fShader);
    m_sbShader->link();

    m_cubeTex = make_unique<Texture>(m_glbase);
    m_cubeTex->loadTextureCube(textureFile);

    m_sphere = make_unique<Sphere>(4.f, 32);
}

void SkyBoxBlend::draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo) const {
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glFrontFace(GL_CW);

    glm::mat4 rotMat  = glm::rotate(mat4(1.f), static_cast<float>(time) * 0.3f, vec3(0.f, 1.f, 0.f));
    glm::mat4 rotMat2 = glm::rotate(rotMat, static_cast<float>(M_PI), vec3(std::sin(time * 0.8f) * 0.4f, 0.8f, 0.2f));
    glm::mat4 rotMat3 = glm::rotate(rotMat, static_cast<float>(M_PI) * 0.3f, vec3(std::sin(time * 0.6f) * -0.4f, 0.8f, 0.f));

    m_sbShader->begin();
    m_sbShader->setUniformMatrix4fv("tc_rot", (GLfloat*)&rotMat[0][0]);
    m_sbShader->setUniformMatrix4fv("tc_rot2", (GLfloat*)&rotMat2[0][0]);
    m_sbShader->setUniformMatrix4fv("tc_rot3", (GLfloat*)&rotMat3[0][0]);
    m_sbShader->setUniformMatrix4fv("m_pvm", cs->getSetModelMatrPtr(), cs->getNrCameras());
    m_sbShader->setUniform1ui("samplerCube", 0);
    m_sbShader->setUniform2f("noiseScale", 0.04f, 0.04f);
    m_sbShader->setUniform1f("width", 1280.f);
    m_sbShader->setUniform1f("height", 720.f);
    m_sbShader->setUniform1f("time", static_cast<float>(time));
    m_sbShader->setUniform1f("time2", static_cast<float>(time) * 2.f);
    m_cubeTex->bind(0);

    m_sphere->draw();

    glFrontFace(GL_CCW);  // counter clockwise definition means front, as default
    glDepthMask(GL_TRUE);

    // reconnect shader from previous step
    shader->begin();
}

void SkyBoxBlend::remove() const {
    m_cubeTex->releaseTexture();
    Sphere::remove();
}

}  // namespace ara
