//
//  ShaderCollector.cpp
//
//  Created by Sven Hahne on 4/6/15.
//

#include "ShaderCollector.h"

using namespace std;

namespace ara {
ShaderCollector::ShaderCollector() {
    m_uiObjMapUniforms = "layout(location = 1) out vec4 objMap;\n uniform float objId;\n";

    m_uiObjMapMain =
        "objMap = vec4(mod(floor(objId * 1.52587890625e-5), 256.0) / 255.0, "
        "mod(floor(objId * 0.00390625), 256.0) / 255.0, mod(objId, 256.0) / "
        "255.0, 1.0);\n";
}

Shaders *ShaderCollector::add(const std::string &name, const std::string &vert, const std::string &frag) {
    if (!hasShader(name)) {
        shaderCollection[name] = make_unique<Shaders>(vert, frag, false);
        shaderCollection[name]->link();
    }

    return shaderCollection[name].get();
}

Shaders *ShaderCollector::add(const std::string &name, const std::string &vert, const std::string &geom,
                              const std::string &frag) {
    if (!hasShader(name)) {
        shaderCollection[name] = make_unique<Shaders>(vert, geom, frag, false);
        shaderCollection[name]->link();
    }

    return shaderCollection[name].get();
}

Shaders *ShaderCollector::add(const std::string &name, const std::string &comp) {
    if (!hasShader(name)) {
        shaderCollection[name] = make_unique<Shaders>(comp, false);
    }
    return shaderCollection[name].get();
}

Shaders *ShaderCollector::add(const std::string &name, const std::string &vert, const std::string &cont,
                              const std::string &eval, const std::string &geom, const std::string &frag) {
    if (!hasShader(name)) {
        shaderCollection[name] = make_unique<Shaders>(vert, cont, eval, geom, frag);
        shaderCollection[name]->link();
    }
    return shaderCollection[name].get();
}

Shaders *ShaderCollector::addNoLink(const std::string &name, const std::string &vert, const std::string &frag) {
    if (!hasShader(name)) {
        shaderCollection[name] = make_unique<Shaders>(vert, frag, false);
    }
    return shaderCollection[name].get();
}

Shaders *ShaderCollector::addNoLink(const std::string &name, const std::string &vert, const std::string &geom,
                                    const std::string &frag) {
    if (!hasShader(name)) {
        shaderCollection[name] = make_unique<Shaders>(vert, geom, frag, false);
    }
    return shaderCollection[name].get();
}

void ShaderCollector::deleteShader(const std::string &name) {
    if (hasShader(name)) {
        const auto it = shaderCollection.find(name);
        shaderCollection.erase(it);
    }
}

void ShaderCollector::deleteShader(const Shaders *ptr) {
    for (auto it = shaderCollection.begin(); it != shaderCollection.end(); ++it) {
        if (it->second.get() == ptr) {
            shaderCollection.erase(it);
            break;
        }
    }
}

Shaders *ShaderCollector::get(const std::string &name) {
    if (shaderCollection.contains(name)) {
        return shaderCollection[name].get();
    }
    return nullptr;
}

bool ShaderCollector::hasShader(const std::string &name) const {
    return (!shaderCollection.empty() && (shaderCollection.contains(name)));
}

Shaders *ShaderCollector::getStdClear(bool layered, int nrLayers) {
    if (hasShader("std_clear")) {
        return shaderCollection["std_clear"].get();
    }

    if (hasShader("std_clear_layer")) {
        return shaderCollection["std_clear_layer"].get();
    }

    std::string vert =
        STRINGIFY(layout(location = 0) in vec4 position; uniform vec4 clearCol; out vec4 i_col; void main() {
            i_col       = clearCol;
            gl_Position = position;
        });
    vert = shdr_Header + "// clear shader, vert\n" + vert;

    //---------------------------------------------------------

    std::string shdr_Header_g = getShaderHeader() + "layout(triangles, invocations=" + std::to_string(nrLayers) +
                                ") in;\nlayout(triangle_strip, max_vertices = 3) out;\nuniform vec4 "
                                "color[" +
                                std::to_string(nrLayers) + "];\n";

    std::string geom = STRINGIFY(uniform mat4 m_pvm; in vec4 i_col[];
        out vec4 o_col;
        void main() {\n
            for (int i = 0; i < gl_in.length(); i++) {\n
                gl_Layer = gl_InvocationID;
                o_col       = color[gl_InvocationID];
                gl_Position = m_pvm * gl_in[i].gl_Position;\n
                EmitVertex();\n
            }\n
            EndPrimitive();\n
    });

    //---------------------------------------------------------

    std::string frag;

    if (layered)
        frag = STRINGIFY(layout(location = 0) out vec4 color;\n in vec4 o_col;\n void main() { color = o_col; });
    else
        frag = STRINGIFY(layout(location = 0) out vec4 color;\n in vec4 i_col;\n void main() { color = i_col; });

    frag = shdr_Header + "// clear shader, frag\n" + frag;

    if (layered) {
        geom = shdr_Header_g + "// clear shader, geom\n" + geom;
        return add("std_clear_layer", vert, geom, frag);
    }
    return add("std_clear", vert, frag);
}

Shaders *ShaderCollector::getStdCol() {
    if (hasShader("std_color")) {
        return shaderCollection["std_color"].get();
    }

    std::string vert = STRINGIFY(
        layout(location = 0) in vec4 position;
        layout(location = 1) in vec4 normal;
        layout(location = 3) in vec4 color;
        uniform mat4 m_pvm;
        out vec4 col;
        void main() {
            col         = color;
            gl_Position = m_pvm * position;
        });

    vert = shdr_Header + "// basic color shader, vert\n" + vert;

    std::string frag = STRINGIFY(
        in vec4 col;
        layout(location = 0) out vec4 color;
        void main() {
            color = col;
        });

    frag = shdr_Header + "// basic color shader, frag\n" + frag;

    return add("std_color", vert, frag);
}

Shaders *ShaderCollector::getStdParCol() {
    if (hasShader("std_par_color")) return shaderCollection["std_par_color"].get();

#ifndef __EMSCRIPTEN__
    std::string vert = STRINGIFY(
        layout(location = 0) in vec4 position; \n
        uniform mat4 m_pvm;
        void main() {\n
            gl_Position = m_pvm * position;\n
    });
    vert = shdr_Header + "// parametric color shader, vert\n" + vert;

    std::string frag = STRINGIFY(
        layout(location = 0) out vec4 glFragColor;
        \n uniform vec4 color;
        \n void main() {\n
            glFragColor = color;\n
    });
    frag = shdr_Header + "// parametric color shader, frag\n" + frag;
#else
    std::string vert = STRINGIFY(precision highp float; \n attribute vec4 position; \n uniform mat4 m_pvm; void main() {
        \n gl_Position = m_pvm * position;
        \n
    });

    std::string frag = STRINGIFY(precision highp float; \n uniform vec4 color; \n void main() {
        \n glFragColor = color;
        \n
    });
#endif

    if (!hasShader("std_par_color")) {
        shaderCollection["std_par_color"] = make_unique<Shaders>(vert, frag, false);
#ifdef __EMSCRIPTEN__
        glBindAttribLocation(shaderCollection["std_par_color"]->getProgram(), 0, "position");
#endif
        shaderCollection["std_par_color"]->link();
    }

    return shaderCollection["std_par_color"].get();
}

Shaders *ShaderCollector::getUIParCol() {
    if (hasShader("ui_par_color")) return shaderCollection["ui_par_color"].get();

#ifndef __EMSCRIPTEN__
    std::string vert =
        STRINGIFY(layout(location = 0) in vec4 position; \n uniform mat4 m_pvm; uniform vec2 size; void main() {
            \n gl_Position = m_pvm * vec4(position.xy * size, position.z, 1.0);
            \n
        });
    vert = shdr_Header + "// ui parametric color shader, vert\n" + vert;

    std::string frag = STRINGIFY(layout(location = 0) out vec4 glFragColor; \n
        uniform vec4 color; \n
        void main() { \n
            glFragColor = color; \n
    });

    frag = shdr_Header + "// ui parametric color shader, frag\n" + frag;
#else
    std::string vert = STRINGIFY(precision highp float; \n attribute vec4 position; \n uniform mat4 m_pvm; void main() {
        \n gl_Position = m_pvm * position;
        \n
    });

    std::string frag = STRINGIFY(precision highp float; \n uniform vec4 color; \n void main() {
        \n glFragColor = color;
        \n
    });
#endif

    if (!hasShader("ui_par_color")) {
        shaderCollection["ui_par_color"] = make_unique<Shaders>(vert, frag, false);
#ifdef __EMSCRIPTEN__
        glBindAttribLocation(shaderCollection["std_par_color"]->getProgram(), 0, "position");
#endif
        shaderCollection["ui_par_color"]->link();
    }

    return shaderCollection["ui_par_color"].get();
}

Shaders *ShaderCollector::getStdColAlpha() {
    if (hasShader("std_color_alpha")) return shaderCollection["std_color_alpha"].get();

    std::string vert = STRINGIFY(layout(location = 0) in vec4 position; uniform mat4 m_pvm;
                                 void main() { gl_Position = m_pvm * position; });

    vert = shdr_Header + "// basic color alpha shader, vert\n" + vert;

    std::string frag = STRINGIFY(uniform vec4 col; layout(location = 0) out vec4 color; void main() { color = col; });

    frag = shdr_Header + "// basic color alpha  shader, frag\n" + frag;

    return add("std_color_alpha", vert, frag);
}

Shaders *ShaderCollector::getStdColBorder() {
    if (hasShader("std_color_border")) return shaderCollection["std_color_border"].get();

    std::string vert = STRINGIFY(
        layout(location = 0) in vec4 position;\n layout(location = 1) in vec4 normal; \n layout(location = 2) in vec2 texCoord; \n layout(location = 3) in vec4 color; \n uniform mat4 m_pvm; \n out vec2 tex_coord;\n void
            main() {
                \n tex_coord   = texCoord;
                \n gl_Position = m_pvm * position;
                \n
            });

    vert = shdr_Header + "// standard color border shader, vert\n" + vert;

    std::string frag = STRINGIFY(
        layout(location = 0) out vec4 fragColor;\n in vec2 tex_coord; \n uniform vec2 borderWidth; \n uniform vec4 borderColor; \n uniform vec4 color;\n void
            main() {
                \n vec2 border = vec2(tex_coord.x<borderWidth.x ? 1.0 : tex_coord.x>(1.0 - borderWidth.x) ? 1.0 : 0.0,
                                      tex_coord.y<borderWidth.y ? 1.0 : tex_coord.y>(1.0 - borderWidth.y) ? 1.0 : 0.0);
                \n fragColor = mix(color, borderColor, max(border.x, border.y));
                \n
            });

    frag = shdr_Header + "// standard color border shader, frag\n" + frag;

    return add("std_color_border", vert, frag);
}

Shaders *ShaderCollector::getUIObjMapOnly() {
    if (hasShader("ui_objmap_only")) return shaderCollection["ui_objmap_only"].get();

    std::string vert = STRINGIFY(
        layout(location = 0) in vec4 position;\n uniform nodeData {
            uniform mat4    pvm;
            \n uniform vec2 size;
            \n uniform uint objId;
            \n
        };
        const vec2[4] quadVertices = vec2[4](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 1.0));\n void
            main() {
                \n gl_Position = pvm * vec4(quadVertices[gl_VertexID] * size, position.z, 1.0);
                \n
            });

    vert = shdr_Header + "// ui objmap only shader, vert\n" + vert;

    std::string frag = STRINGIFY(
        layout(location = 0) out vec4 fragColor;\n layout(location = 1) out vec4 objMap;\n uniform nodeData {
            uniform mat4    pvm;
            \n uniform vec2 size;
            \n uniform uint objId;
            \n
        };
        void main() {
            fragColor    = vec4(0.0);
            float fObjId = float(objId);
            objMap       = vec4(mod(floor(fObjId * 1.52587890625e-5), 256.0) / 255.0,
                                mod(floor(fObjId * 0.00390625), 256.0) / 255.0, mod(fObjId, 256.0) / 255.0, 1.0);
            \n;
        });

    frag = shdr_Header + "// ui objmap only hader, frag\n" + frag;

    return add("ui_objmap_only", vert, frag);
}

Shaders *ShaderCollector::getUIColBorder() {
    if (hasShader("ui_color_border")) return shaderCollection["ui_color_border"].get();

    std::string vert = STRINGIFY(layout(location = 0) in vec4 position;\n
         layout(location = 2) in vec2 texCoord; \n
         uniform nodeData { \n
             mat4 m_pvm; \n
             vec2 size; \n // in pixels
             float zPos; \n
             uint objId;\n
             vec2 borderRadius; \n
             vec2 borderWidth; \n
             vec4 borderColor; \n
             vec2 alias; \n
             vec4 color;\n
             /*vec2 resolution; \n*/
         };\n
         out vec2 tex_coord;\n
         const vec2[4] quadVertices = vec2[4](vec2(0., 0.), vec2(1., 0.), vec2(0., 1.), vec2(1., 1.));\n
         void main() { \n
             tex_coord = quadVertices[gl_VertexID];\n
             // snap to full pixels
             //vec4 sp = m_pvm * vec4(quadVertices[gl_VertexID] * size, position.z, 1.0);
             //vec4 spNdc = sp / sp.w;
             //spNdc.xy = (floor((spNdc.xy * 0.5 + 0.5) * resolution) / resolution) * 2.0 - 1.0; // snap to pixels and normalize back
             //gl_Position = spNdc * sp.w;\n
             gl_Position = m_pvm * vec4(quadVertices[gl_VertexID] * size, position.z, 1.0);\n
    });

    vert = shdr_Header + "// ui color border shader, vert\n" + vert;

    std::string frag = STRINGIFY(layout(location = 0) out vec4 fragColor;\n
         layout(location = 1) out vec4 objMap;
         in vec2 tex_coord; \n
         uniform nodeData { \n
             mat4 m_pvm; \n
             vec2 size; \n // in pixels
             float zPos; \n
             uint objId;\n
             vec2 borderRadius; \n
             vec2 borderWidth; \n
             vec4 borderColor; \n
             vec2 alias; \n
             vec4 color;\n
             /*vec2 resolution; \n*/
         };\n
         const float pi_2 = 1.57079632679489661923;\n
         \n
         void main() { \n
             // origin to center, map all quadrants to the first quadrant
             vec2 tn = abs(tex_coord - 0.5);\n

             // check if inside border and or corner area
             bool isBorder = tn.x >= (0.5 - borderWidth.x) || tn.y >= (0.5 - borderWidth.y);\n
             bool isCorner = tn.x >= (0.5 - borderRadius.x) && tn.y >= (0.5 - borderRadius.y);\n
             bool validBr = borderRadius.x != 0.0 && borderRadius.y != 0.0;\n// check if the borderRadius is not 0
             bool validBw = borderWidth.x != 0.0 && borderWidth.y != 0.0; \n// check if the borderRadius is not 0

             // corner calculation, move left lower corner edge to the center
             // and scale by the size of the borderRadius
             // corner size = borderRadius
             vec2 cc = (tn - (0.5 - borderRadius)) / borderRadius;\n
             float r = length(cc);\n
             float angle = atan(cc.y, cc.x);\n

             // since borderradius pixels translate to different normalized values for x and y
             // we have to calculate a normalized radius and scale it by borderWidth
             vec2 rr = vec2(cos(angle), sin(angle));\n

             // borderWidth in corner area coordinates (ignored if borderRadius == 0)
             vec2 bw = borderWidth / (validBr ? borderRadius : vec2(1.0));\n

             // aliasWidth in corner area coordinates (ignored if borderRadius == 0)
             vec2 aw = alias / (validBr ? borderRadius : vec2(1.0));\n
             vec2 awH = aw * 0.5;\n

             // standard border (not corner)
             vec4 fc = isBorder ? borderColor : color;\n

             // inner edge
             //fc = isBorder && validBw ? mix(color, aux0, smoothstep(length(rr*(1.0-bw-aw)), length(rr*(1.0-bw)), r) ) : fc;
             fc = isCorner && validBw ? mix(color, borderColor, smoothstep(length(rr * (max(1.0-bw-aw, 0.0))), length(rr * max(1.0-bw, 0.0)), r)) : fc;\n

             // outer edge
             vec4 outColor = mix(fc, vec4(0.0), isCorner ? smoothstep(length(rr *(1.0-awH)), length(rr*(1.0+awH)), r) : 0.0);\n
             fragColor = outColor;

             float fObjId = float(objId);
             objMap = vec4(mod(floor(fObjId * 1.52587890625e-5), 256.0) / 255.0, mod(floor(fObjId * 0.00390625), 256.0) / 255.0, mod(fObjId, 256.0) / 255.0, 1.0);\n;

             gl_FragDepth = outColor.a > 0.001 ? zPos : gl_FragCoord.z;\n
             /*if (outColor.a < 0.001) discard;\n*/
    });

    frag = shdr_Header + "// ui color border shader, frag\n" + frag;
    return add("ui_color_border", vert, frag);
}

Shaders *ShaderCollector::getStdTexBorder() {
    if (hasShader("std_tex_border")) return shaderCollection["std_tex_border"].get();

#ifndef __EMSCRIPTEN__
    std::string vert = STRINGIFY(layout(location = 0) in vec4 position; layout(location = 2) in vec2 texCoord;
         out vec2 tex_coord; uniform mat4 m_pvm; void main() {
             tex_coord   = texCoord;
             gl_Position = m_pvm * position;
         });
    vert = shdr_Header + "// tex border shader, vert\n" + vert;

#else
    std::string vert = STRINGIFY(precision mediump float; attribute vec4 position; attribute vec2 texCoord;
                                 varying vec2 tex_coord; uniform mat4 m_pvm; void main() {
                                     tex_coord   = texCoord;
                                     gl_Position = m_pvm * position;
                                 });
#endif

#ifndef __EMSCRIPTEN__
    std::string frag =
        STRINGIFY(in vec2 tex_coord; layout(location = 0) out vec4 glFragColor; uniform sampler2D tex;
                  uniform vec2 borderWidth; uniform vec4 borderColor; uniform vec4 color; void main() {
                      vec2 border =
                          vec2(tex_coord.x<borderWidth.x ? 1.0 : tex_coord.x>(1.0 - borderWidth.x) ? 1.0 : 0.0,
                               tex_coord.y<borderWidth.y ? 1.0 : tex_coord.y>(1.0 - borderWidth.y) ? 1.0 : 0.0);
                      glFragColor = mix(texture(tex, tex_coord), borderColor, max(border.x, border.y));
                  });
    frag = shdr_Header + "// tex border shader, frag\n" + frag;

#else
    std::string frag =
        STRINGIFY(precision mediump float; varying vec2 tex_coord; uniform sampler2D tex; uniform vec2 m_borderWidth;
                  uniform vec4 aux0; uniform vec4 color; void main() {
                      // 0.9999 wegen rundungsfehlern...
                      vec2 border =
                          vec2(tex_coord.x<m_borderWidth.x ? 1.0 : tex_coord.x>(1.0 - m_borderWidth.x) ? 1.0 : 0.0,
                               tex_coord.y<m_borderWidth.y ? 1.0 : tex_coord.y>(1.0 - m_borderWidth.y) ? 1.0 : 0.0);
                      glFragColor = mix(texture(tex, tex_coord), aux0, max(border.x, border.y));
                  });
#endif

    if (!shaderCollection.contains("std_tex_border")) {
        shaderCollection["std_tex_border"] = make_unique<Shaders>(vert, frag, false);
#ifdef __EMSCRIPTEN__
        // for GLES and WEBGL bind the standard attribute locations
        glBindAttribLocation(shaderCollection["std_tex_border"]->getProgram(), 0, "position");
        glBindAttribLocation(shaderCollection["std_tex_border"]->getProgram(), 1, "normal");
        glBindAttribLocation(shaderCollection["std_tex_border"]->getProgram(), 2, "texCoord");
        glBindAttribLocation(shaderCollection["std_tex_border"]->getProgram(), 3, "color");
#endif
        shaderCollection["std_tex_border"]->link();
    }

    return shaderCollection["std_tex_border"].get();
}

Shaders *ShaderCollector::getStdDirLight() {
    if (hasShader("std_dir_light")) return shaderCollection["std_dir_light"].get();

    std::string vert = STRINGIFY(layout(location = 0) in vec4 position; layout(location = 1) in vec3 normal;
         layout(location = 2) in vec2 texCoord; layout(location = 3) in vec4 color;
         out vec4 Color; out vec3 Normal;  // interpolate the normalized surface normal
         out vec2 tex_coord; uniform mat4 m_pvm; uniform mat3 m_normal; void main() {
             Color = color;
             // transform the normal, without perspective, and
             // normalize it
             Normal      = normalize(m_normal * normal);
             tex_coord   = texCoord;
             gl_Position = m_pvm * position;
         });

    vert = shdr_Header + "// basic directional light shader, vert\n" + vert;

    std::string frag =
        STRINGIFY(uniform sampler2D tex_diffuse0; uniform vec4 ambient; uniform vec4 diffuse; uniform vec4 lightColor;
          uniform vec3  lightDirection;  // direction toward the light
          uniform vec3  halfVector;      // surface orientation for shiniest spots
          uniform float shininess;       // exponent for sharping highlights
          uniform float strength;        // extra factor to adjust shininess
          uniform float useTexture;      // extra factor to adjust shininess

          vec4 tex0;

          in vec4 Color; in vec3 Normal;  // surface normal, interpolated between vertices
          in vec2 tex_coord; out vec4 glFragColor;

          void main() {
              tex0 = texture(tex_diffuse0, tex_coord);

              // compute cosine of the directions, using dot products,
              // to see how much light would be reflected
              float diffuseAmt = max(0.0, dot(Normal, lightDirection));
              float specular   = max(0.0, dot(Normal, halfVector));

              // surfaces facing away from the light (negative dot products)
              // won’t be lit by the directional light
              if (diffuseAmt == 0.0)
                  specular = 0.0;
              else
                  specular = pow(specular, shininess);  // sharpen the highlight

              vec3 scatteredLight = vec3(ambient) + vec3(diffuse) * diffuseAmt;
              vec3 reflectedLight = vec3(lightColor) * specular * strength;

              // don’t modulate the underlying color with reflected light,
              // only with scattered light
              vec3 rgb = min((Color.rgb + tex0.rgb * useTexture) * scatteredLight + reflectedLight, vec3(1.0));
              glFragColor = vec4(rgb * 1.3, Color.a);
          });

    frag = shdr_Header + "// basic directional light shader, frag\n" + frag;

    return add("std_dir_light", vert, frag);
}

Shaders *ShaderCollector::getStdClassicOpenGlLight() {
    if (hasShader("std_classic_light")) return shaderCollection["std_classic_light"].get();

    std::string vert = STRINGIFY(
        layout(location = 0) in vec4 position; layout(location = 1) in vec3 normal;
        layout(location = 2) in vec2 texCoord; layout(location = 3) in vec4 color; out VS_FS {
            vec4 color;
            vec3 normal;  // interpolate the normalized surface normal
            vec2 tex_coord;
        } vertex_out;

        uniform mat4 m_pvm; uniform mat3 m_normal;

        void main() {
            vertex_out.color = color;
            // transform the normal, without perspective, and normalize it
            vertex_out.normal    = normalize(m_normal * normal);
            vertex_out.tex_coord = texCoord;
            gl_Position          = m_pvm * position;
        });
    vert = shdr_Header + "// standard classic opengl directional light shader, vert\n" + vert;

    std::string frag = STRINGIFY(
        uniform sampler2D tex_diffuse0; uniform vec4 lightColor; uniform vec4 ambient; uniform vec4 diffuse;
        uniform vec4 emissive; uniform vec3 lightDirection;  // direction toward the light
        uniform vec3                        halfVector;      // surface orientation for shiniest spots
        uniform float                       shininess;       // exponent for sharping highlights
        uniform float                       strength;        // extra factor to adjust shininess

        vec4 tex0;

        in VS_FS {
            vec4 color;
            vec3 normal;  // interpolate the normalized surface normal
            vec2 tex_coord;
        } vertex_in;

        out vec4 fragColor;

        void main() {
            tex0 = texture(tex_diffuse0, vertex_in.tex_coord);

            // compute cosine of the directions, using dot products,
            // to see how much light would be reflected
            float diffuseAmt = max(0.0, dot(vertex_in.normal, lightDirection));
            float specular   = max(0.0, dot(vertex_in.normal, halfVector));

            // surfaces facing away from the light (negative dot products)
            // won’t be lit by the directional light
            if (diffuseAmt == 0.0)
                specular = 0.0;
            else
                specular = pow(specular, shininess);  // sharpen the highlight

            vec3 scatteredLight = vec3(ambient) + vec3(diffuse) * diffuseAmt;
            vec3 reflectedLight = vec3(lightColor) * specular * strength;

            // don’t modulate the underlying color with reflected light,
            // only with scattered light
            vec3 rgb  = min((vertex_in.color.rgb + tex0.rgb) * scatteredLight + reflectedLight, vec3(1.0));
            fragColor = diffuse;
            // gl_FragColor = vec4(rgb * 1.3, Color.a);
        });

    frag = shdr_Header + "// standard classic opengl  light shader, frag\n" + frag;

    return add("std_classic_light", vert, frag);
}

Shaders *ShaderCollector::getStdTex() {
    if (hasShader("std_tex")) return shaderCollection["std_tex"].get();

    std::string vert = STRINGIFY(
        layout(location = 0) in vec4 position; \n layout(location = 1) in vec4 normal; \n layout(location = 2) in vec2 texCoord; \n layout(location = 3) in vec4 color; \n uniform mat4 m_pvm; \n out vec2 tex_coord; \n void
            main() {
                \n tex_coord   = texCoord;
                \n gl_Position = m_pvm * position;
                \n
            });

    vert = shdr_Header + "// basic texture shader, vert\n" + vert;

    std::string frag =
        STRINGIFY(uniform sampler2D tex; \n in vec2 tex_coord; \n layout(location = 0) out vec4 color; \n void main() {
            \n vec4 col = texture(tex, tex_coord);
            if (col.a < 0.00001) discard;
            color = col;
            \n
        });

    frag = shdr_Header + "// basic texture shader, frag\n" + frag;

    return add("std_tex", vert, frag);
}

Shaders *ShaderCollector::getStdTexNullVao() {
    if (hasShader("std_tex_nullvao")) return shaderCollection["std_tex_nullvao"].get();

    std::string vert =
        STRINGIFY(layout(location = 0) in vec4 position; \n uniform mat4 m_pvm; \n out vec2 tex_coord; \n void main() {
            \n const vec2[4] vr  = vec2[4](vec2(0., 0.), vec2(1., 0.), vec2(0., 1.), vec2(1., 1.));
            \n       tex_coord   = vr[gl_VertexID];
            \n       gl_Position = m_pvm * vec4(tex_coord * 2.0 - 1.0, 0.0, 1.0);
            \n
        });
    vert = shdr_Header + "// null m_vao texture shader, vert\n" + vert;

    std::string frag = STRINGIFY(
        uniform sampler2D tex; \n in vec2 tex_coord; \n layout(location = 0) out vec4 fragColor; \n void main() {
            \n vec4 col = texture(tex, tex_coord);
            if (col.a < 0.00001) discard;
            fragColor = col;
            \n
        });
    frag = shdr_Header + "// null m_vao shader, frag\n" + frag;

    return add("std_tex_nullvao", vert, frag);
}

Shaders *ShaderCollector::getStdParColNullVao() {
    if (hasShader("std_parcol_nullvao")) return shaderCollection["std_parcol_nullvao"].get();

    std::string vert = STRINGIFY(
        layout(location = 0) in vec4 position; \n uniform mat4 m_pvm; \n const vec2[4] vr = vec2[4](vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(-1.0, 1.0), vec2(1.0, 1.0));\n void
            main() {
                \n gl_Position = m_pvm * vec4(vr[gl_VertexID], 0.0, 1.0);
                \n
            });
    vert = shdr_Header + "// null m_vao texture shader, vert\n" + vert;

    std::string frag = STRINGIFY(uniform vec4 color; \n layout(location = 0) out vec4 fragColor; \n void main() {
        \n fragColor = color;
        \n
    });
    frag             = shdr_Header + "// null m_vao shader, frag\n" + frag;

    return add("std_parcol_nullvao", vert, frag);
}

Shaders *ShaderCollector::getStdTexColConv() {
    if (hasShader("std_tex_conv")) return shaderCollection["std_tex_conv"].get();

    std::string vert = STRINGIFY(out vec2 tex_coord; \n void main() {
        \n const vec2[4] vr  = vec2[4](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 1.0));
        \n       tex_coord   = vr[gl_VertexID];
        \n       gl_Position = vec4(vr[gl_VertexID] * 2.0 - 1.0, 0.0, 1.0);
        \n
    });
    vert             = "// texture color converter shader, vert\n" + shdr_Header + vert;

    std::string frag = STRINGIFY(
        uniform sampler2D tex; \n uniform int inverted; \n uniform int hFlip; \n in vec2 tex_coord; \n layout(location = 0) out vec4 color; \n void
            main() {
                \n vec4 col = texture(tex, bool(hFlip) ? vec2(tex_coord.x, 1.0 - tex_coord.y) : tex_coord);
                color       = vec4(0.0, 0.0, 0.0, bool(inverted) ? col.r : 1.0 - col.r);
                \n
            });

    frag = "// texture color converter shader, frag\n" + shdr_Header + frag;

    return add("std_tex_conv", vert, frag);
}

Shaders *ShaderCollector::getGridShdrNullVao() {
    if (hasShader("grid_shader_nullvao")) return shaderCollection["grid_shader_nullvao"].get();

    std::string vert =
        STRINGIFY(layout(location = 0) in vec4 position; \n uniform mat4 m_pvm; \n out vec2 tex_coord; \n void main() {
            \n const vec2[4] vr  = vec2[4](vec2(0., 0.), vec2(1., 0.), vec2(0., 1.), vec2(1., 1.));
            \n       tex_coord   = vr[gl_VertexID];
            \n       gl_Position = m_pvm * vec4(tex_coord * 2.0 - 1.0, 0.0, 1.0);
            \n
        });
    vert = shdr_Header + "// null vao grid shader, vert\n" + vert;

    std::string frag = STRINGIFY(
        uniform vec4 color; \n layout(location = 0) out vec4 fragColor; \n in vec2 tex_coord; \n uniform vec4 bColor;
        uniform vec4 wColor; uniform vec4 borderColor; uniform vec2 size; uniform vec2 gridRes;
        uniform int enableBorder; uniform int invert; uniform ivec2 borderWidth; void main() {
            \n bool brd = bool(enableBorder);
            vec2    contSize =
                brd ? vec2(1.0 - (borderWidth.x / size.x * 2.0), 1.0 - (borderWidth.y / size.y * 2.0)) : vec2(1.0);
            vec2  contOffs    = brd ? borderWidth / size : vec2(0.0);
            vec2  cellSize    = contSize / gridRes;
            ivec2 cellPos     = ivec2((tex_coord - contOffs) / cellSize);
            bool  cpYEven     = bool(cellPos.y % 2);
            bool  isBlackCell = cpYEven ? bool(cellPos.x % 2) : bool((cellPos.x + 1) % 2);
            isBlackCell       = bool(invert) ? !isBlackCell : isBlackCell;
            vec2 ct           = abs(tex_coord - 0.5);
            bool isBorder     = brd && ct.x > (0.5 - contOffs.x) || ct.y > (0.5 - contOffs.y);
            fragColor         = isBorder ? borderColor : (isBlackCell ? bColor : wColor);
        });
    frag = shdr_Header + "// null grid shader, frag\n" + frag;

    return add("grid_shader_nullvao", vert, frag);
}

Shaders *ShaderCollector::getUITex() {
    if (hasShader("ui_tex")) return shaderCollection["ui_tex"].get();

    std::string vert = STRINGIFY(
        layout(location = 0) in vec4 position; \n layout(location = 1) in vec4 normal; \n layout(location = 2) in vec2 texCoord; \n layout(location = 3) in vec4 color; \n uniform mat4 m_pvm; \n uniform vec2 size; \n uniform int hFlip; \n out vec2 tex_coord; \n void
            main() {
                \n tex_coord   = texCoord;
                \n tex_coord.y = (hFlip == 1) ? 1.0 - texCoord.y : texCoord.y;
                \n gl_Position = m_pvm * vec4(position.xy * size, position.z, 1.0);
                \n
            });

    vert = shdr_Header + "// ui texture shader, vert\n" + vert;

    std::string frag = STRINGIFY(
        uniform sampler2D tex; \n uniform float bright; \n uniform vec4 color = vec4(1.0); \n in vec2 tex_coord; \n layout(location = 0) out vec4 fragColor; \n void
            main() {
                \n vec4 col = texture(tex, tex_coord);
                if (col.a < 0.00001) discard;
                fragColor = col * color * bright;
                \n
            });

    frag = shdr_Header + "// ui texture shader, frag\n" + frag;

    frag = "// ui texture shader, frag\n" + shdr_Header + m_uiObjMapUniforms + frag;

    return add("ui_tex", vert, frag);
}

Shaders *ShaderCollector::getUITexInv() {
    if (hasShader("ui_tex_inv")) return shaderCollection["ui_tex_inv"].get();

    std::string vert = STRINGIFY(
        layout(location = 0) in vec4 position; \n layout(location = 1) in vec4 normal; \n layout(location = 2) in vec2 texCoord; \n layout(location = 3) in vec4 color; \n uniform mat4 m_pvm; \n uniform vec2 size; \n uniform int hFlip; \n out vec2 tex_coord; \n void
            main() {
                \n tex_coord   = texCoord;
                \n tex_coord.y = (hFlip == 1) ? 1.0 - texCoord.y : texCoord.y;
                \n gl_Position = m_pvm * vec4(position.xy * size, position.z, 1.0);
                \n
            });

    vert = "// ui texture invert shader, vert\n" + shdr_Header + vert;

        std::string frag = STRINGIFY(
            uniform sampler2D tex; \n
            uniform int invert; \n
            uniform vec4 invClearColor; \n
            uniform vec4 alphaToColor; \n
            uniform float alpha; \n
            in vec2 tex_coord; \n
            layout(location = 0) out vec4 fragColor; \n
            void main() {
                \n vec4 col = texture(tex, tex_coord);
                fragColor   = bool(invert) ? mix(alphaToColor, invClearColor, col.a) : col;
                \n fragColor.a *= alpha;
        );

        frag += m_uiObjMapMain + "}";
        frag = shdr_Header + "// ui texture invert shader, frag\n" + m_uiObjMapUniforms + frag;
        return add("ui_tex_inv", vert, frag);
    }

/** only used for direct drawing */
Shaders *ShaderCollector::getUIGridTexSimple() {
    if (hasShader("ui_simple_grid_tex")) return shaderCollection["ui_simple_grid_tex"].get();

    std::string vert = STRINGIFY(out vec2 uv;\n
         out vec2 tuv;\n
         flat out int out_flags;\n
         uniform sampler2D stex;\n  // OpenGL requires sampler variables to be explicitly declared as uniform
         uniform sampler2D depth;\n
         uniform sampler2D objIdTex;\n
         uniform nodeData {
             int useDepthTex; \n
             int useTexObjId; \n
             mat4 mvp;\n
             vec2 pos;\n
             float zPos;\n
             vec2 size;\n
             float scale;\n
             ivec2 align;\n         // 0:left,1:center,2:right\n
             vec4 color; \n
             int flags;\n           // bit 0: cover all surface, bit 1: scale image using 'scale' uniform (this overrides bit0)\n
                                    // bit 2: flip H, bit 3: flip V, bit 4: use nearest interpolation, bit 5: no aspect ratio
             ivec2 section_pos;\n
             ivec2 section_size;\n  // if section_size!=0 will use sections\n
             ivec2 section_sep;\n
             int bit_count; \n
             uint objId; \n
             float alpha; \n
             float borderRadius;
         };\n
         void main(){ \n
             const vec2[4] vr = vec2[4](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 1.0));\n
             vec2 v = vr[gl_VertexID] * size;\n

             vec2 tso = vec2(section_size.x <= 0 ? textureSize(stex, 0) : section_size);\n
             vec2 ts = tso * ((flags & 1) != 0 ? max(size.x / tso.x, size.y / tso.y) : min(size.x/tso.x, size.y/tso.y));\n

             if ((flags & 2)!=0) ts = tso * scale;\n

             uv = vec2(v.x - (align.x == 1 ? (size.x * 0.5 - ts.x * 0.5) : align.x == 2 ? size.x - ts.x : 0.0), \n
                       v.y - (align.y == 1 ? (size.y * 0.5 - ts.y * 0.5) : align.y == 2 ? size.y - ts.y : 0.0)) / ts;\n

             uv.x = ((flags & 4)!=0) ? 1.0 - uv.x : uv.x; \n
             uv.y = ((flags & 8)!=0) ? 1.0 - uv.y : uv.y; \n

            tuv = section_size.x <= 0 ? uv : (vec2(section_pos) + vec2(section_size) * uv) / vec2(textureSize(stex, 0));\n

             if ((flags & 32)!=0) {
                 uv = tuv = vr[gl_VertexID];
                 tuv.x = ((flags & 4)!=0) ? 1.0 - tuv.x : tuv.x;\n
                 tuv.y = ((flags & 8)!=0) ? 1.0 - tuv.y : tuv.y;\n
             }\n                        // no-aspect ratio

             out_flags = flags;\n
             gl_Position = mvp * vec4(pos+vr[gl_VertexID]*size, zPos, 1);\n
     });

    vert = shdr_Header + "// ui simple grid texture shader, vert\n" + vert;

    std::string frag = STRINGIFY(in vec2 uv; \n
         in vec2 tuv;               \n
         flat in int out_flags;     \n
         uniform sampler2D stex;    \n   // OpenGL requires sampler variables to be explicitly declared as uniform
         uniform sampler2D depth;   \n
         uniform sampler2D objIdTex;\n
         uniform nodeData {         \n
             int useDepthTex;       \n
             int useTexObjId;       \n
             mat4 mvp;              \n
             vec2 pos;              \n
             float zPos;            \n
             vec2 size;             \n
             float scale;           \n
             ivec2 align;           \n // 0:left,1:center,2:right\n
             vec4 color;            \n
             int flags;             \n // bit 0: cover all surface, bit 1: scale image using 'scale' uniform (this overrides bit0)\n
             // bit 2: flip H, bit 3: flip V, bit 4: use nearest interpolation, bit 5: no aspect ratio
             ivec2 section_pos;     \n
             ivec2 section_size;    \n // if section_size!=0 will use sections\n
             ivec2 section_sep;     \n
             int bit_count;         \n
             uint objId;            \n
             float alpha;           \n
             float borderRadius;    \n
         };                         \n

         layout(location = 0)  out vec4 out_color; \n
         layout(location = 1)  out vec4 out_objId; \n

         void main(void) { \n
             if (uv.x>=0.0 && uv.x<1.0 && uv.y>=0.0 && uv.y<1.0) { \n
                out_color = (((out_flags&16)!=0 ? texelFetch(stex, ivec2(tuv * vec2(textureSize(stex, 0))), 0) : texture(stex, vec2(tuv.x, 1.0-tuv.y)))) * color; \n
                out_color = (bit_count==8) ? vec4(out_color.r, out_color.r, out_color.r, out_color.a) : out_color;
                out_color.a *= alpha;
                if (out_color.a < 0.001) discard;
                gl_FragDepth = bool(useDepthTex) ? texture(depth, vec2(tuv.x, 1.0 - tuv.y)).r : gl_FragCoord.z;\n
                out_objId = bool(useTexObjId) ? texture(objIdTex, vec2(tuv.x, 1.0 - tuv.y)) : vec4(0.0);\n
            } else { \n
                discard;
            }\n
     });

    frag = shdr_Header + "// ui simple grid texture shader, frag\n" + frag;

    return add("ui_simple_grid_tex", vert, frag);
}

Shaders *ShaderCollector::getUIGridTexYuv() {
    if (hasShader("ui_yuv_grid_tex")) return shaderCollection["ui_yuv_grid_tex"].get();

    std::string vert = STRINGIFY(out vec2 uv;\n
         out vec2 tuv;\n
         flat out int out_flags;\n
         uniform sampler2D tex_unit;\n   // OpenGL requires sampler variables to be explicitly declared as uniform
         uniform sampler2D u_tex_unit;\n   // OpenGL requires sampler variables to be explicitly declared as uniform
         uniform sampler2D v_tex_unit;\n   // OpenGL requires sampler variables to be explicitly declared as uniform
         uniform sampler2D depth;\n
         uniform sampler2D objIdTex;\n
         uniform nodeData {
             int useDepthTex; \n
             int useTexObjId; \n
             mat4 mvp;\n
             vec2 pos;\n
             float zPos;\n
             vec2 size;\n
             float scale;\n
             ivec2 align;\n            // 0:left,1:center,2:right\n
             vec4 color; \n
             int flags;\n            // bit 0: cover all surface, bit 1: scale image using 'scale' uniform (this overrides bit0)\n
             // bit 2: flip H, bit 3: flip V, bit 4: use nearest interpolation, bit 5: no aspect ratio
             ivec2 section_pos;    \n
             ivec2 section_size;\n    // if section_size!=0 will use sections\n
             ivec2 section_sep;\n
             int bit_count; \n
             uint objId; \n
             float alpha; \n
         };\n
         void main(void){ \n
             const vec2[4] vr = vec2[4](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 1.0));\n
             vec2 v = vr[gl_VertexID] * size;\n
             vec2 tso = vec2(textureSize(tex_unit, 0));\n
             vec2 ts = tso * ((flags & 1)!=0 ? max(size.x/tso.x, size.y/tso.y) : min(size.x/tso.x, size.y/tso.y));\n

             if ((flags & 2)!=0) ts = tso * scale; \n

             uv = vec2(v.x - (align.x==1 ? (size.x*0.5 - (ts.x*0.5)) : align.x==2 ? size.x-(ts.x) : 0.0), \n
             v.y - (align.y==1 ? (size.y*0.5 - (ts.y*0.5)) : align.y==2 ? size.y-(ts.y) : 0.0)) / ts;\n

             uv.x = ((flags & 4)!=0) ? 1.0 - uv.x : uv.x;\n
             uv.y = ((flags & 8)!=0) ? 1.0 - uv.y : uv.y;\n

             tuv = section_size.x <= 0 ? uv : (vec2(section_pos+section_size) * uv) / vec2(textureSize(tex_unit, 0));\n

             if ((flags & 32)!=0){
             uv = tuv = vr[gl_VertexID];
             tuv.x = ((flags & 4)!=0) ? 1.0 - tuv.x : tuv.x;\n
             tuv.y = ((flags & 8)!=0) ? 1.0 - tuv.y : tuv.y;\n
         }\n                        // no-aspect ratio

         out_flags = flags;\n
         gl_Position = mvp * vec4(pos+vr[gl_VertexID]*size, 0.0, 1.0);\n
    });

    vert = shdr_Header + "// ui yuv grid texture shader, vert\n" + vert;

    std::string frag = STRINGIFY(in vec2 uv; \n
         in vec2 tuv; \n
         flat in int out_flags; \n
         uniform sampler2D tex_unit;\n   // OpenGL requires sampler variables to be explicitly declared as uniform
         uniform sampler2D u_tex_unit;\n   // OpenGL requires sampler variables to be explicitly declared as uniform
         uniform sampler2D v_tex_unit;\n   // OpenGL requires sampler variables to be explicitly declared as uniform
         uniform sampler2D depth;\n
         uniform sampler2D objIdTex;\n
         uniform int isYuv;\n
         uniform int isNv12;\n
         uniform int isNv21;\n
         uniform int isYCbCr;\n
         uniform int isCrCbY;\n
         uniform int rot90;\n
         uniform float alpha;\n
         uniform nodeData {
             int useDepthTex; \n
             int useTexObjId; \n
             mat4 mvp;\n
             vec2 pos;\n
             float zPos;\n
             vec2 size;\n
             float scale;\n
             ivec2 align;\n            // 0:left,1:center,2:right\n
             vec4 color; \n
             int flags;\n            // bit 0: cover all surface, bit 1: scale image using 'scale' uniform (this overrides bit0)\n
             // bit 2: flip H, bit 3: flip V, bit 4: use nearest interpolation, bit 5: no aspect ratio
             ivec2 section_pos;    \n
             ivec2 section_size;\n    // if section_size!=0 will use sections\n
             ivec2 section_sep;\n
             int bit_count; \n
             uint objId; \n
             float alpha; \n
         };\n

         layout(location = 0)  out vec4 out_color; \n
         layout(location = 1)  out vec4 out_objId; \n

         vec4 getYuvNv12(vec2 tex_coord) { \n
             float y = texture(tex_unit, tex_coord).r; \n
             float u = texture(u_tex_unit, tex_coord).r - 0.5; \n
             float v = texture(u_tex_unit, tex_coord).g - 0.5; \n

             return vec4((vec3(y + 1.4021 * v, \n
             y - 0.34482 * u - 0.71405 * v, \n
             y + 1.7713 * u) - 0.05) * 1.07, \n
             alpha); \n
         }\n
         \n
         vec4 getYuvNv21(vec2 tex_coord) { \n
             float y = texture(tex_unit, tex_coord).r; \n
             float u = texture(u_tex_unit, tex_coord).g - 0.5; \n
             float v = texture(u_tex_unit, tex_coord).r - 0.5; \n

             return vec4((vec3(y + 1.4021 * v, \n
             y - 0.34482 * u - 0.71405 * v, \n
             y + 1.7713 * u) - 0.05) * 1.07, \n
             alpha); \n
         }\n

         vec4 getYuv420p(vec2 tex_coord) { \n
             float y = texture(tex_unit, tex_coord).r; \n
             float u = texture(u_tex_unit, tex_coord).r - 0.5; \n
             float v = texture(v_tex_unit, tex_coord).r - 0.5; \n
             \n
             float r = y + 1.402 * v; \n
             float g = y - 0.344 * u - 0.714 * v; \n
             float b = y + 1.772 * u; \n
             \n
             return vec4(vec3(r, g, b), alpha);\n
        }

         vec4 getYCbCr(vec2 tex_coord) { \n
             vec3 ycbcr;
             ivec2 tsize = textureSize(tex_unit, 0);
             ivec2 a = ivec2(tex_coord * vec2(tsize));
             vec4 texCol = texelFetch(tex_unit, a, 0);

             ycbcr.x = texCol.g;
             ycbcr.y = ((a.x & 1) == 0 ? texCol.r : texelFetch(tex_unit, ivec2(a.x - 1, a.y), 0).r) - 0.5;
             ycbcr.z = ((a.x & 1) == 0 ? texelFetch(tex_unit, ivec2(a.x + 1, a.y), 0).r : texCol.r) - 0.5;

             return vec4(ycbcr.x + (1.4065 * (ycbcr.z)),
             ycbcr.x - (0.3455 * (ycbcr.y)) - (0.7169 * (ycbcr.z)),
             ycbcr.x + (1.7790 * (ycbcr.y)), texCol.a);
        }

         vec4 getCrCbY(vec2 tex_coord) { \n
             vec3 ycbcr;
             ivec2 tsize = textureSize(tex_unit, 0);
             ivec2 a = ivec2(tex_coord * vec2(tsize));
             vec4 texCol = texelFetch(tex_unit, a, 0);

             ycbcr.x = texCol.r;
             ycbcr.y = ((a.x & 1) == 0 ? texCol.g : texelFetch(tex_unit, ivec2(a.x - 1, a.y), 0).g) - 0.5;
             ycbcr.z = ((a.x & 1) == 0 ? texelFetch(tex_unit, ivec2(a.x + 1, a.y), 0).g : texCol.g) - 0.5;

             return vec4(ycbcr.x + (1.4065 * (ycbcr.z)),
             ycbcr.x - (0.3455 * (ycbcr.y)) - (0.7169 * (ycbcr.z)),
             ycbcr.x + (1.7790 * (ycbcr.y)), texCol.a);;
        }

         vec4 getPix(vec2 tex_coord){
             vec2 tc = bool(rot90) ? vec2(tex_coord.y, tex_coord.x) : tex_coord;
             return bool(isYuv) ? bool(isNv21) ? getYuvNv21(tc) :
                 bool(isNv12) ? getYuvNv12(tc) :
                 bool(isYCbCr) ? getYCbCr(tc) :
                 bool(isCrCbY) ? getCrCbY(tc) : getYuv420p(tc) : texture(tex_unit, tc);
         }

         void main(void) { \n
             if (uv.x>=0.0 && uv.x<1.0 && uv.y>=0.0 && uv.y<1.0) { \n
                 out_color = ((out_flags&16) !=0 ? getPix(tuv) : getPix(vec2(tuv.x, tuv.y))) * color; \n
                 out_color = (bit_count==8) ? vec4(out_color.r, out_color.r, out_color.r, out_color.a) : out_color;
                 out_color.a *= alpha;
                 if (out_color.a < 0.001) discard;

                 gl_FragDepth = bool(useDepthTex) ? texture(depth, vec2(tuv.x, tuv.y)).r : zPos;\n
                 out_objId = bool(useTexObjId) ? texture(objIdTex, vec2(tuv.x, tuv.y)) : vec4(0.0);\n
             } else { \n
                discard; //out_color = bg_color;		// we should discard here or use the _background_ color\n
             }
     });

    frag = shdr_Header + "// ui yuv grid texture shader, frag\n" + frag;
    return add("ui_yuv_grid_tex", vert, frag);
}

/** only used for direct drawing */
Shaders *ShaderCollector::getUIGridTexFrame() {
    if (hasShader("ui_frame_grid_tex")) return shaderCollection["ui_frame_grid_tex"].get();

    std::string vert = STRINGIFY(
        out vec2 tuv;\n
        flat out ivec2 texsize;\n
        flat out ivec2 sp;\n
        flat out ivec2 ss;\n
        flat out ivec2 sd;\n
        flat out ivec2 ts;\n
        uniform sampler2D stex;\n  // OpenGL requires sampler variables to be explicitly declared as uniform
        uniform nodeData {
            int useDepthTex; \n
            int useTexObjId; \n
            mat4 mvp;\n
            vec2 pos;\n
            float zPos;\n
            vec2 size;\n
            float scale;\n
            ivec2 align;\n            // 0:left,1:center,2:right\n
            vec4 color; \n
            int flags;\n            // bit 0: cover all surface, bit 1: scale image using 'scale' uniform (this overrides bit0)\n
            // bit 2: flip H, bit 3: flip V, bit 4: use nearest interpolation, bit 5: no aspect ratio
            ivec2 section_pos;    \n
            ivec2 section_size;\n    // if section_size!=0 will use sections\n
            ivec2 section_sep;\n
            int bit_count; \n
            uint objId; \n
            float alpha; \n
        };\n
        void main(void){ \n
            const vec2[4] vr = vec2[4](vec2(0., 0.), vec2(1., 0.), vec2(0., 1.), vec2(1., 1.));\n
            sp = section_pos;\n
            ss = section_size;\n
            sd = section_sep;\n
            ts = ivec2(size);\n
            texsize = textureSize(stex, 0);
            tuv = vr[gl_VertexID] * size; \n
            gl_Position = mvp * vec4(pos + tuv, 0, 1);\n
    });

    vert = shdr_Header + "// ui frame grid texture shader, vert\n" + vert;

    std::string frag = STRINGIFY(
        in vec2 tuv;\n
        flat in ivec2 texsize;\n
        flat in ivec2 sp;\n
        flat in ivec2 ss;\n
        flat in ivec2 sd;\n
        flat in ivec2 ts;\n
        uniform sampler2D stex;\n  // OpenGL requires sampler variables to be explicitly declared as uniform
        uniform nodeData {
            int useDepthTex; \n
            int useTexObjId; \n
            mat4 mvp;\n
            vec2 pos;\n
            float zPos;\n
            vec2 size;\n
            float scale;\n
            ivec2 align;\n            // 0:left,1:center,2:right\n
            vec4 color; \n
            int flags;\n            // bit 0: cover all surface, bit 1: scale image using 'scale' uniform (this overrides bit0)\n
            // bit 2: flip H, bit 3: flip V, bit 4: use nearest interpolation, bit 5: no aspect ratio
            ivec2 section_pos;    \n
            ivec2 section_size;\n    // if section_size!=0 will use sections\n
            ivec2 section_sep;\n
            int bit_count; \n
            uint objId; \n
            float alpha; \n
        };\n
        \n
        layout(location = 0)  out vec4 out_color; \n
        layout(location = 1)  out vec4 objMap; \n
        \n
        void main(void){ \n
            ivec2 v = ivec2(tuv);\n

            ivec2 pv = ivec2( v.x<ss.x ? v.x : v.x>=ts.x-ss.x ? sd.x*2+v.x-ts.x+ss.x : sd.x+(v.x%ss.x),
            v.y<ss.y ? v.y : v.y>=ts.y-ss.y ? sd.y*2+v.y-ts.y+ss.y : sd.y+(v.y%ss.y));\n

            ivec2 pp = ivec2(vec2(pv+sp) * scale);

            out_color = texelFetch(stex, ivec2(pp.x, texsize.y-pp.y), 0) * color;\n
            out_color.a *= alpha;

            if (out_color.a < 0.001) discard;

            float fObjId = float(objId);
            objMap = vec4(mod(floor(fObjId * 1.52587890625e-5), 256.0) / 255.0, mod(floor(fObjId * 0.00390625), 256.0) / 255.0, mod(fObjId, 256.0) / 255.0, 1.0);\n;

            gl_FragDepth = zPos;\n
    });

    frag = shdr_Header + "// ui frame grid texture shader, frag\n" + frag;

    return add("ui_frame_grid_tex", vert, frag);
}

Shaders *ShaderCollector::getStdGreyTex() {
    if (hasShader("std_tex_grey")) return shaderCollection["std_tex_grey"].get();

    std::string vert = STRINGIFY(
        layout(location = 0) in vec4 position; \n layout(location = 1) in vec4 normal; \n layout(location = 2) in vec2 texCoord; \n layout(location = 3) in vec4 color; \n uniform mat4 m_pvm; \n out vec2 tex_coord; \n void
            main() {
                \n tex_coord   = texCoord;
                \n gl_Position = m_pvm * position;
                \n
            });

    vert = shdr_Header + "// grey texture shader, vert\n" + vert;

    std::string frag = STRINGIFY(
        uniform sampler2D tex; \n in vec2 tex_coord; \n layout(location = 0) out vec4 color; \n void main() {
            \n float red   = texture(tex, tex_coord).r;
            \n       color = vec4(red, red, red, 1.0);
            \n
        });

    frag = shdr_Header + "// grey texture shader, frag\n" + frag;

    return add("std_tex_grey", vert, frag);
}

Shaders *ShaderCollector::getUIGreyTex() {
    if (hasShader("ui_tex_grey")) return shaderCollection["ui_tex_grey"].get();

    std::string vert = STRINGIFY(
        layout(location = 0) in vec4 position; \n layout(location = 1) in vec4 normal; \n layout(location = 2) in vec2 texCoord; \n layout(location = 3) in vec4 color; \n uniform mat4 m_pvm; \n uniform vec2 size; \n out vec2 tex_coord; \n void
            main() {
                \n tex_coord   = texCoord;
                \n gl_Position = m_pvm * vec4(position.xy * size, position.z, 1.0);
                \n
            });

    vert = shdr_Header + "// grey texture shader, vert\n" + vert;

    std::string frag = STRINGIFY(
        uniform sampler2D tex; \n uniform float bright; \n in vec2 tex_coord; \n layout(location = 0) out vec4 color; \n void
            main() {
                \n float red   = texture(tex, tex_coord).r;
                \n       color = bright * vec4(red, red, red, 1.0);
                \n
            });

    frag = shdr_Header + "// grey texture shader, frag\n" + frag;

    return add("ui_tex_grey", vert, frag);
}

Shaders *ShaderCollector::getStdDepthDebug() {
    if (hasShader("std_depth_debug")) return shaderCollection["std_depth_debug"].get();

    std::string vert = STRINGIFY(
        layout(location = 0) in vec4 position; \n layout(location = 1) in vec4 normal; \n layout(location = 2) in vec2 texCoord; \n layout(location = 3) in vec4 color; \n uniform mat4 m_pvm; \n out vec2 tex_coord; \n void
            main() {
                \n tex_coord   = texCoord;
                \n gl_Position = m_pvm * position;
                \n
            });

    vert = shdr_Header + "// basic depth debug shader, vert\n" + vert;

    std::string frag = STRINGIFY(
        uniform sampler2D tex; \n in vec2 tex_coord; \n uniform vec2 screenSize; \n layout(location = 0) out vec4 color; \n void
            main() {
                \n vec4 z = texelFetch(tex, ivec2(tex_coord * screenSize), 0);
                float   c = (z.r + z.g + z.b) * 0.3333;
                float   n = 1.0;    // the near plane
                float   f = 100.0;  // the far plane
                c         = (2.0 * n) / (f + n - c * (f - n));
                color     = vec4(c, c, c, 1.0);
                \n
            });

    frag = shdr_Header + "// basic depth debug shader, frag\n" + frag;

    return add("std_depth_debug", vert, frag);
}

Shaders *ShaderCollector::getStdRec() {
    if (hasShader("std_rec")) return shaderCollection["std_rec"].get();

    std::string vert =
        STRINGIFY(layout(location = 0) in vec4 position; layout(location = 1) in vec3 normal;
                  layout(location = 2) in vec2 texCoord; layout(location = 3) in vec4 color;
                  layout(location = 4) in vec4 texCorMod; layout(location = 10) in mat4 modMatr;

                  uniform int useInstancing; uniform int texNr;

                  uniform mat4 modelMatrix; uniform mat4 projectionMatrix;

                  out vec4 rec_position; out vec3 rec_normal; out vec4 rec_texCoord; out vec4 rec_color;

                  mat4 MVM;

                  void main(void) {
                      rec_color    = color;
                      vec2 tc      = useInstancing == 0
                                         ? texCoord
                                         : texCoord * vec2(texCorMod.b, texCorMod.a) + vec2(texCorMod.r, texCorMod.g);
                      rec_texCoord = vec4(tc.x, tc.y, float(texNr), 0.0);

                      MVM          = (useInstancing == 0 ? modelMatrix : modMatr);
                      rec_position = MVM * position;
                      rec_normal   = normalize((MVM * vec4(normal, 0.0)).xyz);

                      gl_Position = rec_position;
                  });

    vert = shdr_Header + "// standard record fragment shader, vert\n" + vert;

    std::string frag = STRINGIFY(layout(location = 0) out vec4 color; void main() { color = vec4(1.0); });

    frag = shdr_Header + "// standard record fragment shader, frag\n" + frag;

    return add("std_rec", vert, frag);
}

Shaders *ShaderCollector::getStdTexMulti() {
    if (hasShader("std_tex_multi")) return shaderCollection["std_tex_multi"].get();

    std::string vert = STRINGIFY(
        layout(location = 0) in vec4 position; \n layout(location = 1) in vec4 normal;\n layout(location = 2) in vec2 texCoord; \n layout(location = 3) in vec4 color; \n uniform mat4 m_pvm; \n out vec2 tex_coord; \n void
            main() {
                \n tex_coord   = texCoord;
                \n gl_Position = m_pvm * position;
                \n
            });

    vert = shdr_Header + "// basic multisample texture shader, vert\n" + vert;

    std::string frag = STRINGIFY(
        uniform sampler2DMS tex; \n in vec2 tex_coord; \n uniform vec2 fboSize; \n uniform int nrSamples; \n layout(location = 0) out vec4 color; \n
            vec4 sampCol;
        void     main() {
            \n sampCol = vec4(0);
            for (int i = 0; i < 64; i++) {
                if (i >= nrSamples) break;
                sampCol += texelFetch(tex, ivec2(fboSize * tex_coord), i);
            }
            color = sampCol / float(nrSamples);
            \n
        });

    frag = shdr_Header + "// basic multisample texture shader, frag\n" + frag;

    return add("std_tex_multi", vert, frag);
}

Shaders *ShaderCollector::getStdTexAlpha(bool multiSampTex) {
    if (hasShader("std_tex_alpha"))
        return shaderCollection["std_tex_alpha"].get();
    else if (hasShader("std_tex_alpha_ms"))
        return shaderCollection["std_tex_alpha_ms"].get();

    std::string vert = STRINGIFY(
        layout(location = 0) in vec4 position; \n layout(location = 1) in vec4 normal; \n layout(location = 2) in vec2 texCoord; \n layout(location = 3) in vec4 color; \n uniform mat4 m_pvm; \n out vec2 tex_coord; \n void
            main() {
                \n tex_coord   = texCoord;
                \n gl_Position = m_pvm * position;
                \n
            });

    vert = shdr_Header + "// basic alpha texture shader, vert\n" + vert;

    std::string frag;

    if (multiSampTex) {
        frag += STRINGIFY(
            uniform float alpha;\n uniform sampler2DMS tex;\n uniform int nrSamples; \n uniform vec2 scr_size; \n in vec2 tex_coord; \n layout(location = 0) out vec4 color; \n
                vec4 outColMS;

            void main() {
                \n outColMS = vec4(0.0);
                for (int i = 0; i < 64; i++) {
                    if (i > nrSamples) break;
                    outColMS += texelFetch(tex, ivec2(scr_size * tex_coord), i);
                    \n
                }
                color = outColMS / float(nrSamples);
                color.a *= alpha;
                \n
            });
    } else {
        frag += STRINGIFY(
            uniform float alpha;\n uniform sampler2D tex;\n in vec2 tex_coord;\n layout(location = 0) out vec4 color; \n void
                main() {
                    \n color = texture(tex, tex_coord);
                    \n color.a *= alpha;
                    \n
                });
    }

    frag = shdr_Header + "// basic alpha texture shader, frag\n" + frag;

    std::string name;

    if (multiSampTex)
        name = "std_tex_alpha_ms";
    else
        name = "std_tex_alpha";

    return add(name, vert, frag);
}

Shaders *ShaderCollector::getStdGlyphShdr() {
    if (hasShader("std_glyph_shader")) return shaderCollection["std_glyph_shader"].get();

    std::string vert = STRINGIFY(layout(location = 0) in vec4 position; \n
                                         layout(location = 2) in vec2 texCoord; \n
                                         layout(location = 3) in vec4 color; \n
                                         out VS_FS{ \n
                                         vec2 uv;                    \n
                                         vec3 tuv;                   \n
                                 } vout;                         \n
                                         uniform sampler2D stex;         \n
                                         uniform nodeData { \n
                                         mat4 mvp;                   \n
                                         vec4 tcolor;                \n
                                         vec4 mask;                  \n
                                 };
                                         void main(void){ \n
                                         vout.tuv = position.xyz;    \n
                                         vout.uv = texCoord;         \n
                                         gl_Position = mvp * position; \n
                                 });
    vert = "// standard glyph shader, vert\n" + shdr_Header + vert;

    std::string frag = STRINGIFY(
            in VS_FS{\n
            vec2 uv;                    \n
            vec3 tuv;                   \n
    } vin;                          \n
            uniform nodeData { \n
            mat4 mvp;                   \n
            vec4 tcolor;                \n
            vec4 mask;                  \n
    };                              \n
            uniform sampler2D stex;                 \n
            layout(location = 0) out vec4 fcolor;   \n
            layout(location = 1) out vec4 objmap;   \n
            void main(void){ \n
            if ((vin.tuv.x < mask.x || vin.tuv.x > mask.x + mask.z || vin.tuv.y < mask.y || vin.tuv.y > mask.y + mask.w)) discard; \n
            vec4 outCol = vec4(tcolor.rgb, tcolor.a * texture(stex, vin.uv).r); \n
            fcolor = outCol;
            objmap = vec4(0.0);
    });
    frag = shdr_Header + "// standard glyph shader, frag\n" + frag;

    return add("std_glyph_shader", vert, frag);
}

Shaders *ShaderCollector::getEdgeDetect() {
    if (hasShader("std_edge_detect")) return shaderCollection["std_edge_detect"].get();

    std::string shdr_Header = "#version 410 core\n";
    std::string vert        = STRINGIFY(layout(location = 0) in vec4 position; layout(location = 1) in vec4 normal;
                                        layout(location = 2) in vec2 texCoord; layout(location = 3) in vec4 color;
                                        uniform float stepX; uniform float stepY; uniform mat4 m_pvm; out vec2 le;
                                        out vec2 ri; out vec2 tex_coord; void main() {
                                     le          = texCoord + vec2(-stepX, 0.0);
                                     ri          = texCoord + vec2(stepX, 0.0);
                                     tex_coord   = texCoord;
                                     gl_Position = m_pvm * position;
                                        });

    vert = shdr_Header + "// Standard edge detect vertex shader\n" + vert;

    std::string frag = STRINGIFY(
        uniform sampler2D tex; in vec2 le; in vec2 ri; in vec2 tex_coord; layout(location = 0) out vec4 color;
        layout(location = 1) out vec4 shape; bool leftDet; bool rightDet; float outVal; void main() {
            vec4 center = texture(tex, tex_coord);
            vec4 left   = texture(tex, le);
            vec4 right  = texture(tex, ri);

            leftDet  = (!bool(left.r) && center.r > 0.0) || (left.r > 0.0 && !bool(center.r));
            rightDet = (!bool(right.r) && center.r > 0.0) || (right.r > 0.0 && !bool(center.r));

            outVal = leftDet || rightDet ? 1.0 : 0.0;
            outVal = tex_coord.x > 0.015 ? outVal : 0;
            outVal = tex_coord.x < 0.995 ? outVal : 0;

            color = vec4(outVal);
        });

    frag = shdr_Header + "// Standard Edge detect fragment shader\n" + frag;

    return add("std_edge_detect", vert, frag);
}

// takes color rgb Values as input (specialized version)
Shaders *ShaderCollector::getStdHeightMapSobel() {
    if (hasShader("std_hm_sobel")) return shaderCollection["std_hm_sobel"].get();

    std::string shdr_Header = "#version 410 core\n";

    std::string vert = STRINGIFY(layout(location = 0) in vec4 position; layout(location = 1) in vec3 normal;
                                 layout(location = 2) in vec2 texCoord; layout(location = 3) in vec4 color;
                                 out vec2 tex_coord; void main(void) {
                                     tex_coord   = texCoord;
                                     gl_Position = position;
                                 });
    vert             = shdr_Header +
           "//Standard sobel filter: generate normals from height map. "
           "Vert\n" +
           vert;

    std::string frag = STRINGIFY(
        layout(location = 0) out vec4 norm_tex; in vec2 tex_coord; uniform sampler2D heightMap;
        uniform float heightFact; uniform vec2 texGridStep;

        vec3 posTop; vec3 posBottom; vec3 posCenter; vec3 posLeft; vec3 posRight; vec3 norms[2];

        vec3 pixToHeight(vec3 inPix, vec2 tPos, float heightFact) {
            vec3 pos = vec3(tPos * 2.0 - vec2(1.0), (inPix.r + inPix.g + inPix.b) * heightFact);
            return pos;
        }

        void main() {
            // read neighbour positions left, right, top, bottom
            posTop    = pixToHeight(texture(heightMap, vec2(tex_coord.x, tex_coord.y + texGridStep.y)).xyz,
                                    vec2(tex_coord.x, tex_coord.y + texGridStep.y), heightFact);
            posBottom = pixToHeight(texture(heightMap, vec2(tex_coord.x, tex_coord.y - texGridStep.y)).xyz,
                                    vec2(tex_coord.x, tex_coord.y - texGridStep.y), heightFact);
            posCenter = pixToHeight(texture(heightMap, tex_coord).xyz, tex_coord, heightFact);

            posLeft  = pixToHeight(texture(heightMap, vec2(tex_coord.x - texGridStep.x, tex_coord.y)).xyz,
                                   vec2(tex_coord.x - texGridStep.x, tex_coord.y), heightFact);
            posRight = pixToHeight(texture(heightMap, vec2(tex_coord.x + texGridStep.x, tex_coord.y)).xyz,
                                   vec2(tex_coord.x + texGridStep.x, tex_coord.y), heightFact);

            norms[0] = normalize(cross((posTop - posCenter), (posLeft - posCenter)));
            norms[1] = normalize(cross((posBottom - posCenter), (posRight - posCenter)));

            for (int i = 0; i < 2; i++) norms[i] = norms[i].z > 0.0 ? norms[i] : norms[i] * -1.0;

            norms[0] = normalize((norms[0] + norms[1]) * 0.5);
            norm_tex = vec4(norms[0], 1.0);
        });

    frag = shdr_Header +
           "// Standard sobel filter: generate normals from height map "
           "frag\n" +
           frag;

    return add("std_hm_sobel", vert, frag);
}

Shaders *ShaderCollector::getPerlin() {
    if (hasShader("perlin")) return shaderCollection["perlin"].get();

    std::string vert = STRINGIFY(layout(location = 0) in vec4 position; layout(location = 3) in vec4 color;
                                 uniform mat4 m_pvm; out vec4 col;

                                 void main() {
                                     col         = color;
                                     gl_Position = m_pvm * position;
                                 });

    vert = shdr_Header + "// perlin noise, vert\n" + vert;

    std::string frag = STRINGIFY(
        uniform vec2 noiseScale; uniform vec2 noiseScale2; uniform float width; uniform float height; in vec4 col;
        layout(location = 0) out vec4                                                                         color;

        int i; int nrIts = 3; float pi = 3.1415926535897932384626433832795; float newNoise = 0.0;

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

        // Classic Perlin noise, periodic variant
        float pnoise(vec3 P, vec3 rep) {
            vec3 Pi0 = mod(floor(P), rep);         // Integer part, modulo period
            vec3 Pi1 = mod(Pi0 + vec3(1.0), rep);  // Integer part + 1, mod period
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

        void main() {
            vec2  posCo  = vec2(gl_FragCoord) / vec2(width, height) * noiseScale;
            float noiseC = 0.0;

            for (i = 0; i < nrIts; i++) {
                newNoise = (cnoise(vec3(posCo * pow(2.0, float(i + 2)) * 32.0, 0.0)) + 1.0) * 0.25;
                noiseC += newNoise / pow(2.0, float(i));
            }

            noiseC = pow(noiseC, 4.0) * 4.0;
            color  = vec4(noiseC, noiseC, noiseC, 1.0);
        });

    frag = shdr_Header + "// perlin noise shader, frag\n" + frag;

    return add("perlin", vert, frag);
}

std::string ShaderCollector::getFisheyeVertSnippet(size_t nrCameras) {
    return "float "
           "c_PI=3."
           "141592653589793238462643383279502884197169399375105820974944592"
           "308;\n"
           "vec4 fishEyePos(vec4 p){ \n"
           "\t vec4 ndc = p / p.w; \n"
           "\t float l0 = length(ndc.xy); \n"
           "\t float l1 = length(ndc.xyz); \n"
           "\t float a = (ndc.z>0.0) ? c_PI - asin(l0/l1) : asin(l0/l1); \n"
           "\t a /= fishEyeAdjust[gl_InvocationID].z; \n"
           "\t vec4 outV = vec4(a/l0*ndc.xy, a*2.0 -1.0, 1.0); \n"  // shader
                                                                    // ported
                                                                    // from
                                                                    // directx
                                                                    // (NDC
                                                                    // z
                                                                    // [0:1]),
                                                                    // for
                                                                    // this
                                                                    // reason
                                                                    // normalized
                                                                    // to
                                                                    // NDC z
                                                                    // [-1:1]
           "\t outV.xy *= fishEyeAdjust[gl_InvocationID].xy; \n"
           "\t return outV; \n"
           "}\n";
}

void ShaderCollector::clear() {
    for (auto &val: shaderCollection | views::values) {
        val.reset();
    }

    shaderCollection.clear();
}
}
