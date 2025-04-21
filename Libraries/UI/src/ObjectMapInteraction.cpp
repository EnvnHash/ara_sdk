#include "ObjectMapInteraction.h"

using namespace glm;
using namespace std;

namespace ara {

ObjectMapInteraction::ObjectMapInteraction(ShaderCollector* _shCol, uint32_t width, uint32_t height, FBO* extFbo,
                                           bool isMultisample)
    : m_guiFboSize(ivec2(width, height)), m_idCtr(0), m_nrPboBufs(3), m_pboIndex(0), m_shCol(_shCol),
      m_multiSamp(isMultisample) {}

void ObjectMapInteraction::initObjIdShdr() {
    string vert = std::string("#version 130\n") // marco.g : need 1.3 for flat in/out
           + STRINGIFY(
       precision highp float;\n
		attribute vec4 position;\n                                                                                  
		attribute vec2 texCoord;\n
		uniform vec2 size;\n
		uniform mat4 m_pvm;\n
		uniform mat4 perspMat;\n
        uniform int usePersp;\n
        uniform float objId;\n // 0-1 float
		varying vec2 tex_coord;\n
        flat out float passId;\n                            // marco.g : Passing the ObjID as a flat
        \n
		vec2 undist(vec2 inPoint, mat4 distMat) { \n
			float x = inPoint.x; \n
			float y = inPoint.y; \n
			float w = 1.0/(x * distMat[0][2] + y * distMat[1][2] + distMat[2][2]); \n
			inPoint.x = (x * distMat[0][0] + y * distMat[1][0] + distMat[2][0]) *w; \n
			inPoint.y = (x * distMat[0][1] + y * distMat[1][1] + distMat[2][1]) *w; \n
			return inPoint; \n
		} \n
		\n	
        void main() {
		    tex_coord = texCoord;
			vec4 p = (usePersp == 1 ? vec4(undist(position.xy, perspMat), 0.0, 1.0) : position); \n
            gl_Position = m_pvm * vec4(p.xy * size, p.z, 1.0);
            passId=objId;
		});

    string frag = STRINGIFY(#version 130\n precision highp float;\n uniform int useTex;\n uniform sampler2D tex;\n varying vec2 tex_coord;\n flat in float passId;\n  // marco.g : Receiving the ObjID as a flat
        void main() {
            if (useTex == 1) {  // texture comes in as uint32 to 4 bytes
                vec4 texCol = texture2D(tex, tex_coord);
                if (texCol.a < 0.5) discard;
            }
            float z      = (passId) * 16777216.0;  // depth values 0 - 1
            gl_FragColor = vec4(mod(floor(z * 1.52587890625e-5), 256.0) / 255.0,
                                mod(floor(z * 0.00390625), 256.0) / 255.0, mod(z, 256.0) / 255.0, 1.0);
        });

    // m_objIdShdr = m_shCol->add("Warping_objId", vert, frag);
    m_objIdShdr = m_shCol->addNoLink("Warping_objId", vert, frag);

    // VAO binds texCoord to Attribute Location 2, since we are using WebGL
    // compatible drawing, we have to do explicit attrib binding
    glBindAttribLocation(m_objIdShdr->getProgram(), 0, "position");
    glBindAttribLocation(m_objIdShdr->getProgram(), 2, "texCoord");

    m_objIdShdr->link();
}

void ObjectMapInteraction::initExtFboObjIdShdr() {
    string vert = STRINGIFY(
        layout(location = 0) in vec4 position; \n layout(location = 2) in vec2 texCoord; \n uniform vec2 size;\n uniform mat4 m_pvm;\n uniform mat4 perspMat;\n uniform int usePersp;\n uniform int flipVTexCoord;\n out vec2 tex_coord;\n
        \n vec2 undist(vec2 inPoint, mat4 distMat) {
            \n float x         = inPoint.x;
            \n float y         = inPoint.y;
            \n float w         = 1.0 / (x * distMat[0][2] + y * distMat[1][2] + distMat[2][2]);
            \n       inPoint.x = (x * distMat[0][0] + y * distMat[1][0] + distMat[2][0]) * w;
            \n       inPoint.y = (x * distMat[0][1] + y * distMat[1][1] + distMat[2][1]) * w;
            \n return inPoint;
            \n
        } \n
        \n void main() {
            tex_coord        = vec2(texCoord.x, bool(flipVTexCoord) ? 1.0 - texCoord.y : texCoord.y);
            vec4 p           = (usePersp == 1 ? vec4(undist(position.xy, perspMat), 0.0, 1.0) : position);
            \n   gl_Position = m_pvm * vec4(p.xy * size, p.z, 1.0);
        });
    vert = "// objid shader (extfbo), vert\n" + m_shCol->getShaderHeader() + vert;

    string frag = STRINGIFY(uniform int useTex;\n
        uniform sampler2D tex;\n
        uniform sampler2DMS texMs;\n
        uniform float objId;\n // 0-1 float
        uniform int isMultiSampleTex;\n
        in vec2 tex_coord;\n
        uniform vec2 tex_size;\n
        layout(location = 1) out vec4 frag; \n
        void main() { \n
            if (bool(useTex)){  \n// texture comes in as uint32 to 4 bytes
                vec4 texCol;
                if (bool(isMultiSampleTex))
                    texCol = texelFetch(texMs, ivec2(tex_size * tex_coord), 0);
                //else
                  //  texCol = texture2D(tex, tex_coord); \n

                if (texCol.a < 0.5) discard; \n
            } \n
            float z=objId * 16777216.0; \n
            frag = vec4(mod(floor(z * 1.52587890625e-5), 256.0) / 255.0, mod(floor(z * 0.00390625), 256.0) / 255.0, mod(z, 256.0) / 255.0, 1.0);  \n
    });
    frag = "// objid shader (extfbo), frag\n" + m_shCol->getShaderHeader() + frag;

    m_objIdShdr = m_shCol->add("ObjectMapInteraction_extFbo", vert, frag);
}

void ObjectMapInteraction::initNormExtFboObjIdShdr() {
    string vert = STRINGIFY(
        layout(location = 0) in vec4 position; \n layout(location = 2) in vec2 texCoord; \n uniform vec2 size;\n uniform mat4 m_pvm;\n uniform mat4 perspMat;\n uniform int usePersp;\n out vec2 tex_coord;\n
        \n vec2 undist(vec2 inPoint, mat4 distMat) {
            \n float x         = inPoint.x;
            \n float y         = inPoint.y;
            \n float w         = 1.0 / (x * distMat[0][2] + y * distMat[1][2] + distMat[2][2]);
            \n       inPoint.x = (x * distMat[0][0] + y * distMat[1][0] + distMat[2][0]) * w;
            \n       inPoint.y = (x * distMat[0][1] + y * distMat[1][1] + distMat[2][1]) * w;
            \n return inPoint;
            \n
        } \n
        \n void main() {
            tex_coord        = texCoord;
            vec4 p           = (usePersp == 1 ? vec4(undist(position.xy, perspMat), 0.0, 1.0) : position);
            \n   gl_Position = m_pvm * p;
        });
    vert = "// objid norm shader (extfbo), vert\n" + m_shCol->getShaderHeader() + vert;

    string frag = STRINGIFY(uniform int useTex;\n
        uniform sampler2D tex;\n
        uniform float objId;\n // 0-1 float
        in vec2 tex_coord;\n
        layout(location = 1) out vec4 frag; \n
        void main() {
            if (useTex == 1){ // texture comes in as uint32 to 4 bytes
                vec4 texCol = texture2D(tex, tex_coord);
                if (texCol.a < 0.5) discard;
            }
            float z=objId * 16777216.0;
            frag = vec4(mod(floor(z * 1.52587890625e-5), 256.0) / 255.0, mod(floor(z * 0.00390625), 256.0) / 255.0, mod(z, 256.0) / 255.0, 1.0);
        });
    frag = "// objid norm shader (extfbo), frag\n" + m_shCol->getShaderHeader() + frag;

    m_normObjIdShdr = m_shCol->add("ObjectMapInteraction_norm_extFbo", vert, frag);
}

void ObjectMapInteraction::initExtFboObjIdTexAlphaShdr() {
    std::string vert = STRINGIFY(
        out vec2 uv;\n
        out vec2 tuv;\n
        flat out int out_flags;\n
        uniform mat4 mvp;\n
        uniform vec2 pos;\n
        uniform vec2 size;\n
        uniform sampler2D stex;\n
        uniform ivec2 align;\n		// 0:left,1:center,2:right\n
        uniform int flags;\n					// bit 0: cover all surface, bit 1: scale image using 'scale' uniform (this overrides bit0)\n
        // bit 2: flip H, bit 3: flip V, bit 4: use nearest interpolation, bit 5: no aspect ratio
        uniform ivec2 section_pos;	\n
        uniform ivec2 section_size;\n		// if section_size!=0 will use sections\n
        uniform float scale;\n
        void main(void){    \n
            const vec2[4] vr = vec2[4](vec2(0.,0.),vec2(1.,0.),vec2(0.,1.),vec2(1.,1.));\n
            vec2 v=vr[gl_VertexID]*size;\n
            vec2 tso=section_size.x<=0 ? textureSize(stex,0) : section_size;\n
            vec2 ts=tso*((flags & 1)!=0 ? max(size.x/tso.x,size.y/tso.y) : min(size.x/tso.x,size.y/tso.y));\n
            if ((flags & 2)!=0) ts=tso*scale;\n
            uv=vec2(v.x-(align.x==1 ? (size.x/2.0-(ts.x/2)) : align.x==2 ? size.x-(ts.x) : 0),\n
                    v.y-(align.y==1 ? (size.y/2.0-(ts.y/2)) : align.y==2 ? size.y-(ts.y) : 0))/ts;\n
            uv.x = ((flags & 4)!=0) ? 1-uv.x : uv.x;\n
            uv.y = ((flags & 8)!=0) ? 1-uv.y : uv.y;\n
            tuv=section_size.x<=0 ? uv : (section_pos+section_size*uv)/textureSize(stex,0);\n
            if ((flags & 32)!=0){
                uv=tuv=vr[gl_VertexID];
                tuv.x = ((flags & 4)!=0) ? 1.0-tuv.x : tuv.x;\n
                tuv.y = ((flags & 8)!=0) ? 1.0-tuv.y : tuv.y;\n
            }\n						// no-aspect ratio
            out_flags=flags;\n
            gl_Position = mvp * vec4(pos+vr[gl_VertexID]*size,0,1);\n
        });
    vert = "// objid shader (extfbo, texAlpha), vert\n" + m_shCol->getShaderHeader() + vert;

    string frag = STRINGIFY(
        in vec2 uv; \n
        in vec2 tuv; \n
        flat in int out_flags; \n
        uniform sampler2D stex; \n
        uniform float objId;\n // 0-1 float
        uniform int bitCount; \n

        layout(location = 1)  out vec4 objmap; \n

        void main(void) {\n
            objmap=vec4(0.0); // explicitly write nothing to objectmap
            if (uv.x>=0.0 && uv.x<1.0 && uv.y>=0.0 && uv.y<1.0)\n
            { \n
                vec4 out_color = (((out_flags&16)!=0 ? texelFetch(stex, ivec2(tuv * vec2(textureSize(stex, 0))), 0) : texture(stex, vec2(tuv.x,1.0-tuv.y)))) ; \n
                out_color = (bitCount==8) ? vec4(out_color.r, out_color.r, out_color.r, out_color.a) : out_color;
                if (out_color.a < 0.001) discard;

                float z=objId * 16777216.0; \n
                objmap = vec4(mod(floor(z * 1.52587890625e-5), 256.0) / 255.0, mod(floor(z * 0.00390625), 256.0) / 255.0, mod(z, 256.0) / 255.0, 1.0);  \n
            }
            else {\n
                discard; //out_color = bg_color;		// we should discard here or use the _background_ color\n
            }
        });

    frag = "// objid shader (extfbo, texAlpha), frag\n" + m_shCol->getShaderHeader() + frag;

    m_shCol->add("ObjectMapInteraction_texAlpha_extFbo", vert, frag);
}

void ObjectMapInteraction::initMSTextureReadShdr() {
    string vert = STRINGIFY(void main() { gl_Position = vec4(0.0, 0.0, 0.0, 1.0); });
    vert        = "// objid read ms FBO (extfbo), vert\n" + m_shCol->getShaderHeader() + vert;

    string frag;

    if (m_useSSBO) frag += "layout( std140, binding=0 ) buffer sb { uint objId[]; };\n";

    frag += STRINGIFY(uniform sampler2DMS tex;\n
        uniform ivec2 pos; \n
        layout(location = 0) out vec4 frag; \n
        void main() {
        \n vec4 outCol = texelFetch(tex, pos, 0);
        \n      frag   = outCol; \n
    );

        if (m_useSSBO)
            frag += "objId[0] = (uint(outCol.r * 255.0) << 16) + (uint(outCol.g * "
                                         "255.0) << 8) + uint(outCol.b * 255.0);\n";

        frag += "}";
        frag = "// objid read ms FBO (extfbo), frag\n" + m_shCol->getShaderHeader() + frag;

        m_msFboReadShdr = m_shCol->add("ObjectMapInteraction_readFboShdr", vert, frag);
}

}  // namespace ara
