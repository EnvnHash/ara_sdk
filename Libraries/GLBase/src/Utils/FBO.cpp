#include "Utils/FBO.h"

#include "GLBase.h"
#include "GeoPrimitives/Quad.h"
#include "Shaders/ShaderCollector.h"
#include "Utils/Texture.h"

using namespace std;

namespace ara {

FBO::FBO(const FboInitParams& ip)
    : m_hasDepthBuf(ip.depthBuf),
    m_layered(ip.layered),
    m_tex_width(ip.width),
    m_tex_height(ip.height),
    m_tex_depth(ip.depth),
    m_type(ip.type),
    m_extType(ara::getExtType(ip.type)),
    m_pixType(ara::getPixelType(ip.type)),
    m_target(ip.target),
    m_wrapMode(ip.wrapMode),
    m_glbase(ip.glbase),
    m_mipMapLevels(ip.mipMapLevels),
    m_nrAttachments(ip.nrAttachments),
    m_nrSamples(ip.nrSamples)  {
    init();
}

FBO::~FBO() { remove(); }

void FBO::remove() {
    if (m_inited) {
        if (!m_isShared || m_hasTexViews) {
            if (m_hasDepthBuf && !m_isShared) {
                glDeleteTextures(1, &m_depthBuf);
            }
            if (!m_textures.empty()) {
                glDeleteTextures(m_nrAttachments, &m_textures[0]);
            }
        }

        if (m_fbo) {
            glDeleteFramebuffers(1, &m_fbo);
        }
        m_textures.clear();
        m_bufModes.clear();
        m_inited      = false;
        m_hasTexViews = false;
    }
}

void FBO::fromShared(FBO *sharedFbo) {
    if (sharedFbo) {
        getActStates();
        initFromShared(sharedFbo);

        m_textures.resize(sharedFbo->getTextures()->size());
        ranges::copy(*sharedFbo->getTextures(), m_textures.begin());
        m_bufModes.resize(sharedFbo->getBufModes()->size());
        ranges::copy(*sharedFbo->getBufModes(), m_bufModes.begin());

        glGenFramebuffers(1, &m_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

        attachTextures(true);
        m_inited = true;
        restoreStates();
    }
}

void FBO::fromTexMan(const Texture *texMan) {
    if (texMan) {
        getActStates();

        if (m_inited) {
            remove();
        }

        m_textures.resize(1);
        m_bufModes.resize(1);

        m_target        = GL_TEXTURE_2D;
        m_textures[0]   = texMan->getId();
        m_bufModes[0]   = GL_COLOR_ATTACHMENT0;
        m_tex_width     = texMan->getWidth();
        m_tex_height    = texMan->getHeight();
        m_tex_depth     = 1;
        m_f_tex_depth   = 1;
        m_f_tex_width   = static_cast<float>(m_tex_width);
        m_f_tex_height  = static_cast<float>(m_tex_height);
        m_isShared      = false;
        m_type          = texMan->getInternalFormat();
        m_extType       = ara::getExtType(m_type);
        m_pixType       = ara::getPixelType(m_type);
        m_hasDepthBuf   = false;
        m_layered       = false;
        m_nrAttachments = 1;
        m_mipMapLevels  = static_cast<int>(texMan->getMipMapLevels());
        m_nrSamples     = 1;
        m_wrapMode      = ara::Texture::getWrapMode();
        m_magFilterType = texMan->getMagnificationFilter();
        m_minFilterType = texMan->getMinificationFilter();

        m_quad = make_unique<Quad>(QuadInitParams{});

        // get standard shaders for clearing the FBO
        m_colShader     = m_shCol->getStdCol();
        m_clearShader   = m_shCol->getStdClear(m_layered, static_cast<int>(m_tex_depth));
        m_toAlphaShader = m_shCol->getStdTex();

        glGenFramebuffers(1, &m_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

        // Attach the texture to the s_fbo,  iterate through the different types
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_target, m_textures[0], 0);

        checkFbo();
        m_inited = true;
        restoreStates();
    }
}

void FBO::fromSharedSelectAttachment(FBO *sharedFbo, int attNr) {
    if (sharedFbo) {
        getActStates();
        initFromShared(sharedFbo);

        m_textures.resize(1);
        m_textures[0] = sharedFbo->getTextures()->at(attNr);

        m_bufModes.resize(1);
        m_bufModes[0] = sharedFbo->getBufModes()->at(attNr);

        m_nrAttachments = 1;

        glGenFramebuffers(1, &m_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

        // Attach the texture to the s_fbo,  iterate through the different types
        switch (m_target) {
#ifndef ARA_USE_GLES31
            case GL_TEXTURE_1D:
                glFramebufferTexture1D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_target, m_textures[0], 0);
                break;
#endif
            case GL_TEXTURE_2D:
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_target, m_textures[0], 0);
                break;
            case GL_TEXTURE_2D_MULTISAMPLE:
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_target, m_textures[0], 0);
                break;
#ifndef ARA_USE_GLES31
            case GL_TEXTURE_3D: glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_textures[0], 0); break;
            case GL_TEXTURE_RECTANGLE:
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_target, m_textures[0], 0);
                break;
#endif
        }

        if (m_hasDepthBuf)
#ifndef ARA_USE_GLES31
            glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, m_depthBuf, 0);
#else
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_depthBuf, 0);
#endif

        checkFbo();

        m_inited = true;
        // m_hasBeenInited = true;

        restoreStates();
    }
}

void FBO::fromSharedExtractLayer(FBO *sharedFbo, int layerNr) {
#ifndef ARA_USE_GLES31
    // fromShared(sharedFbo);
    m_sharedLayer = layerNr;

    // glTextureView is OpenGL 4.3 functionality
    if (sharedFbo) {
        getActStates();

        initFromShared(sharedFbo);

        switch (m_target) {
            case GL_TEXTURE_1D_ARRAY: m_target = GL_TEXTURE_1D; break;
            case GL_TEXTURE_2D_ARRAY: m_target = GL_TEXTURE_2D; break;
            case GL_TEXTURE_2D_MULTISAMPLE_ARRAY: m_target = GL_TEXTURE_2D_MULTISAMPLE; break;
            default: m_target = GL_TEXTURE_2D; break;
        }

        m_textures.resize(1);
        m_bufModes.resize(sharedFbo->getBufModes()->size());
        ranges::copy(*sharedFbo->getBufModes(), m_bufModes.begin());

        // generate a textureview onto the s_fbo's layered color texture
        glGenTextures(1, &m_textures[0]);

        // Now, create a view of the depth textures
        glTextureView(m_textures[0],             // New texture view
                      m_target,                  // Target for the new view
                      sharedFbo->getColorImg(),  // Original texture
                      getType(),                 // two component 32F texture with moments
                      0, m_mipMapLevels,         // All mipmaps
                      layerNr, 1);               // the specific layer, only one layer

        glGenFramebuffers(1, &m_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

        // Attach the texture to the s_fbo,  iterate through the different types
        switch (m_target) {
            case GL_TEXTURE_1D:
                glFramebufferTexture1D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_target, m_textures[0], 0);
                break;
            case GL_TEXTURE_2D:
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_target, m_textures[0], 0);
                break;
            case GL_TEXTURE_2D_MULTISAMPLE:
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_target, m_textures[0], 0);
                break;
            case GL_TEXTURE_3D: glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_textures[0], 0); break;
            case GL_TEXTURE_RECTANGLE:
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_target, m_textures[0], 0);
                break;
        }

        if (m_hasDepthBuf && m_depthBuf)
#ifndef ARA_USE_GLES31
            glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, m_depthBuf, 0);
#else
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_depthBuf, 0);
#endif

        checkFbo();

        // if the FBO we are sharing is removed and m_inited again, we'd like to
        // be informed since we also need to update the local parameters of this
        // instance
        // sharedFbo->addReinitCb((void*)sharedFbo, [this, sharedFbo,
        // layerNr](){
        //   if (this->isInited()){
        //        this->remove();
        //       this->fromSharedExtractLayer(sharedFbo, layerNr);
        //     }
        // });

        m_inited = true;
        // m_hasBeenInited = true;
        m_hasTexViews = true;

        restoreStates();
    }
#endif
}

void FBO::initFromShared(FBO *sharedFbo) {
    m_isShared    = true;
    m_sharedFbo   = sharedFbo;
    m_tex_width   = sharedFbo->getWidth();
    m_tex_height  = sharedFbo->getHeight();
    m_tex_depth   = sharedFbo->getDepth();
    m_type        = sharedFbo->getType();
    m_extType     = ara::getExtType(m_type);
    m_pixType     = ara::getPixelType(m_type);
    m_target      = sharedFbo->getTarget();
    m_hasDepthBuf = sharedFbo->getHasDepthBuffer();
    if (m_hasDepthBuf) {
        m_depthBuf  = sharedFbo->getDepthImg();
        m_depthType = sharedFbo->getDepthType();
    }
    m_layered       = sharedFbo->getIsLayered();
    m_nrAttachments = sharedFbo->getNrAttachments();
    m_mipMapLevels  = sharedFbo->getMipMapLevels();
    m_nrSamples     = sharedFbo->getNrSamples();
    m_wrapMode      = sharedFbo->getWrapMode();
    m_magFilterType = sharedFbo->getMagFilterType();
    m_minFilterType = sharedFbo->getMinFilterType();

    m_shCol = &m_glbase->shaderCollector();
    m_quad  = make_unique<Quad>(QuadInitParams{});

    // get standard shaders for clearing the FBO
    m_colShader     = m_shCol->getStdCol();
    m_clearShader   = m_shCol->getStdClear(m_layered, static_cast<int>(m_tex_depth));
    m_toAlphaShader = m_shCol->getStdTex();

    // this FBO may be shared, in this case to FBO holding a reference can be
    // notified, that their reference has gone
    // if (m_hasBeenInited)
    //  for (auto &it : m_reinitCb)
    //    it.second();
}

void FBO::init() {
    getActStates();

    m_shCol         = &m_glbase->shaderCollector();
    m_quad          = make_unique<Quad>(QuadInitParams{});
    m_mipMapLevels  = std::min(std::max<int>(1, m_mipMapLevels), m_glbase->maxTexMipMapLevels());
    m_isMultiSample = (m_target == GL_TEXTURE_2D_MULTISAMPLE) || (m_target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY);

#ifndef ARA_USE_GLES31
    if (m_target == GL_TEXTURE_RECTANGLE) {
        m_mipMapLevels = 1;
    }
#endif

    if (m_nrAttachments > m_glbase->maxNrAttachments()) {
        LOGE << "FBO Error: trying to attach more textures to FBO than possible by hardware. Max: "
             << m_glbase->maxNrAttachments();
    }

    m_nrAttachments = std::min<int>(m_nrAttachments, m_glbase->maxNrAttachments());

    m_textures.resize(m_nrAttachments);
    m_bufModes.resize(m_nrAttachments);

    m_f_tex_width  = static_cast<float>(m_tex_width);
    m_f_tex_height = static_cast<float>(m_tex_height);
    m_f_tex_depth  = static_cast<float>(m_tex_depth);

    allocColorTexture();
    if (m_hasDepthBuf) {
        allocDepthTexture();
    }

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    attachTextures(true);

    // get standard shaders for clearing the FBO
    m_colShader     = m_shCol->getStdCol();
    m_clearShader   = m_shCol->getStdClear(m_layered, m_tex_depth);
    m_toAlphaShader = m_shCol->getStdTex();

    m_inited = true;
    restoreStates();
}

/**
 * allocate textures for the FBOs color attachment
 */
void FBO::allocColorTexture() {
    // generate s_fbo textures
    glGenTextures(m_nrAttachments, &m_textures[0]);

    for (auto i = 0; i < m_nrAttachments; i++) {
        if (m_type != GL_DEPTH24_STENCIL8) {
            m_bufModes[i] = GL_COLOR_ATTACHMENT0 + i;
        }

        glBindTexture(m_target, m_textures[i]);
#ifndef __APPLE__
        // NOTE: Using glTexStorage* is generally considered best practice in OpenGL as once defined, the OpenGL
        // implementation can make assumptions that the dimensions and m_format of the texture object
        // will not change over its lifetime and thus can stop tracking certain aspects of the texture object.
        switch (m_target) {
#ifndef ARA_USE_GLES31
            case GL_TEXTURE_1D:
                glTexStorage1D(m_target, m_mipMapLevels, m_type, m_tex_width);
                break;
#endif
            case GL_TEXTURE_2D:
                glTexStorage2D(m_target, m_mipMapLevels, m_type, m_tex_width, m_tex_height);
                break;
#ifndef ARA_USE_GLES31
            case GL_TEXTURE_2D_MULTISAMPLE:
                glTexImage2DMultisample(m_target, m_nrSamples, m_type, m_tex_width, m_tex_height, GL_FALSE);
                break;
            case GL_TEXTURE_1D_ARRAY:
                glTexStorage2D(m_target, m_mipMapLevels, m_type, m_tex_width, m_tex_height);
                break;
            case GL_TEXTURE_RECTANGLE:
                glTexStorage2D(m_target, m_mipMapLevels, m_type, m_tex_width, m_tex_height);
                break;
#endif
            case GL_TEXTURE_3D:
                glTexStorage3D(m_target, m_mipMapLevels, m_type, m_tex_width, m_tex_height, m_tex_depth);
                break;
            case GL_TEXTURE_2D_ARRAY:
                glTexStorage3D(m_target, m_mipMapLevels, m_type, m_tex_width, m_tex_height, m_tex_depth);
                break;
#ifndef ARA_USE_GLES31
            case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
                glTexStorage3DMultisample(m_target, m_mipMapLevels, m_type, m_tex_width, m_tex_height, m_tex_depth,
                                          GL_FALSE);
                break;
#endif
            default: break;
        }
#else
        switch (m_target) {
            case GL_TEXTURE_1D:
                glTexImage1D(m_target, 0, m_type, m_tex_width, 0, ara::getExtType(m_type), ara::getPixelType(m_type), nullptr);
                break;

            case GL_TEXTURE_2D:
                glTexImage2D(m_target, 0, m_type, m_tex_width, m_tex_height, 0, ara::getExtType(m_type),
                             ara::getPixelType(m_type), nullptr);
                break;

            case GL_TEXTURE_2D_MULTISAMPLE:
                glTexImage2DMultisample(m_target, m_nrSamples, m_type, m_tex_width, m_tex_height, GL_FALSE);
                break;

            case GL_TEXTURE_1D_ARRAY:
                glTexImage2D(m_target, 0, m_type, m_tex_width, m_tex_height, 0, ara::getExtType(m_type),
                             ara::getPixelType(m_type), nullptr);
                break;

            case GL_TEXTURE_RECTANGLE:
                glTexImage2D(m_target, 0, m_type, m_tex_width, m_tex_height, 0, ara::getExtType(m_type),
                             ara::getPixelType(m_type), nullptr);
                break;

            case GL_TEXTURE_3D:
                glTexImage3D(m_target, 0, m_type, m_tex_width, m_tex_height, m_tex_depth, 0, ara::getExtType(m_type),
                             ara::getPixelType(m_type), nullptr);
                break;

            case GL_TEXTURE_2D_ARRAY:
                glTexImage3D(m_target, 0, m_type, m_tex_width, m_tex_height, m_tex_depth, 0, ara::getExtType(m_type),
                             ara::getPixelType(m_type), nullptr);
                break;

            case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
                glTexImage3DMultisample(m_target, m_nrSamples, m_type, m_tex_width, m_tex_height, m_tex_depth,
                                        GL_FALSE);
                break;

            default: break;
        }
#endif

        if (!m_isMultiSample) {
            if (m_mipMapLevels == 1) {
                glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, m_minFilterType);
                glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, m_magFilterType);
            } else {
                glGenerateMipmap(m_target);
                glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, m_magFilterType);
            }

#ifndef ARA_USE_GLES31
            if (m_target != GL_TEXTURE_RECTANGLE) {
                glTexParameteri(m_target, GL_TEXTURE_WRAP_S, m_wrapMode);
                glTexParameteri(m_target, GL_TEXTURE_WRAP_T, m_wrapMode);
            }
#endif
        }

        if (m_target == GL_TEXTURE_3D) {
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, m_wrapMode);
        }

        glBindTexture(m_target, 0);
    }
}

/**
 * allocate textures for the FBOs depth attachment
 */
void FBO::allocDepthTexture() {
    glGenTextures(1, &m_depthBuf);
    glBindTexture(m_target, m_depthBuf);

    m_depthType = GL_DEPTH24_STENCIL8;

#ifndef __APPLE__

    switch (m_target) {
#ifndef ARA_USE_GLES31
        case GL_TEXTURE_1D: glTexStorage1D(m_target, m_mipMapLevels, m_depthType, m_tex_width); break;
#endif
        case GL_TEXTURE_2D: glTexStorage2D(m_target, m_mipMapLevels, m_depthType, m_tex_width, m_tex_height); break;

        case GL_TEXTURE_2D_MULTISAMPLE:
            glTexStorage2DMultisample(m_target, m_nrSamples, m_depthType, m_tex_width, m_tex_height, GL_FALSE);
            break;

#ifndef ARA_USE_GLES31
        case GL_TEXTURE_1D_ARRAY:
            glTexStorage2D(m_target, m_mipMapLevels, m_depthType, m_tex_width, m_tex_height);
            break;

        case GL_TEXTURE_RECTANGLE:
            glTexStorage2D(m_target, m_mipMapLevels, m_depthType, m_tex_width, m_tex_height);
            break;
#endif

        case GL_TEXTURE_3D:
            glTexStorage3D(m_target, m_mipMapLevels, m_depthType, m_tex_width, m_tex_height, m_tex_depth);
            break;

        case GL_TEXTURE_2D_ARRAY:
            glTexStorage3D(m_target, m_mipMapLevels, m_depthType, m_tex_width, m_tex_height, m_tex_depth);
            break;

#ifndef ARA_USE_GLES31
        case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
            glTexStorage3D(m_target, m_mipMapLevels, m_depthType, m_tex_width, m_tex_height, m_tex_depth);
            break;
#endif
    }
#else

    switch (m_target) {
        case GL_TEXTURE_1D:
            glTexImage1D(m_target, 0, m_depthType, m_tex_width, 0, ara::getExtType(m_depthType), ara::getPixelType(m_depthType),
                         nullptr);
            break;

        case GL_TEXTURE_2D: {
            glTexImage2D(m_target, 0, m_depthType, m_tex_width, m_tex_height, 0, ara::getExtType(m_depthType),
                         ara::getPixelType(m_depthType), nullptr);
        } break;

        case GL_TEXTURE_2D_MULTISAMPLE:
            glTexImage2DMultisample(m_target, m_nrSamples, m_depthType, m_tex_width, m_tex_height, GL_FALSE);
            break;

        case GL_TEXTURE_1D_ARRAY:
            glTexImage2D(m_target, 0, m_depthType, m_tex_width, m_tex_height, 0, ara::getExtType(m_depthType),
                         ara::getPixelType(m_depthType), nullptr);
            break;

        case GL_TEXTURE_RECTANGLE:
            glTexImage2D(m_target, 0, m_depthType, m_tex_width, m_tex_height, 0, ara::getExtType(m_depthType),
                         ara::getPixelType(m_depthType), nullptr);
            break;

        case GL_TEXTURE_3D:
            glTexImage3D(m_target, 0, m_depthType, m_tex_width, m_tex_height, m_tex_depth, 0, ara::getExtType(m_depthType),
                         ara::getPixelType(m_depthType), nullptr);
            break;

        case GL_TEXTURE_2D_ARRAY:
            glTexImage3D(m_target, 0, m_depthType, m_tex_width, m_tex_height, m_tex_depth, 0, ara::getExtType(m_depthType),
                         ara::getPixelType(m_depthType), nullptr);
            break;

        case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
            glTexImage3DMultisample(m_target, m_nrSamples, m_depthType, m_tex_width, m_tex_height, m_tex_depth,
                                    GL_FALSE);
            break;

        default: break;
    }
#endif
}

/**
 * attach color and depth textures to the FrameBufferObject
 */
void FBO::attachTextures(bool doCheckFbo) const {
    // Attach the texture to the s_fbo,  iterate through the different types
    for (auto i = 0; i < m_nrAttachments; i++) {
        switch (m_target) {
#ifndef ARA_USE_GLES31
            case GL_TEXTURE_1D:
                glFramebufferTexture1D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, m_target, m_textures[i], 0);
                break;
#endif
            case GL_TEXTURE_2D:
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, m_target, m_textures[i], 0);
                break;
#ifdef ARA_USE_GLES31
            case GL_TEXTURE_2D_ARRAY:
                glFramebufferTextureEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, m_textures[i], 0);
                break;
            case GL_TEXTURE_3D:
                if (m_layered)
                    glFramebufferTexture3DOES(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, m_target, m_textures[i], 0, i);
                else
                    glFramebufferTextureEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, m_textures[i], 0);
                break;
#else
            case GL_TEXTURE_2D_MULTISAMPLE:
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, m_target, m_textures[i], 0);
                break;
            case GL_TEXTURE_1D_ARRAY:
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, m_textures[i], 0);
                break;
            case GL_TEXTURE_3D:
                if (m_layered)
                    glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, m_target, m_textures[i], 0, i);
                else
                    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, m_textures[i], 0);
                break;
            case GL_TEXTURE_2D_ARRAY:
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, m_textures[i], 0);
                break;
            case GL_TEXTURE_2D_MULTISAMPLE_ARRAY: {
                // for (auto j = 0; j < m_tex_depth; j++)
                //  glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0
                //  +i, m_target, m_textures[i], 0, i);
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, m_textures[i], 0);
            } break;
            case GL_TEXTURE_RECTANGLE:
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, m_target, m_textures[i], 0);
                break;
#endif
            default: LOGE << "FBO Error couldn't find target m_format"; break;
        }
    }

    if (m_hasDepthBuf)
#ifdef ARA_USE_GLES31
        glFramebufferTextureEXT(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, m_depthBuf, 0);
#elif __APPLE__
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depthBuf, 0);
#else
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, m_depthBuf, 0);
#endif

    if (doCheckFbo) checkFbo();

    // clear
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FBO::assignTex(int attachmentNr, GLuint tex) const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachmentNr, m_target, tex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FBO::bind(bool saveStates) {
    if (saveStates) {
        getActStates();
        glGetIntegerv(GL_VIEWPORT, &m_csVp[0]);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_tex_width, m_tex_height);
    glScissor(0, 0, m_tex_width, m_tex_height);

#ifndef ARA_USE_GLES31
    if (m_target == GL_TEXTURE_2D_MULTISAMPLE || m_target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY) {
        glEnable(GL_MULTISAMPLE);
        glEnable(GL_SAMPLE_SHADING);
    } else
        glDisable(GL_MULTISAMPLE);
#endif

    if (m_hasDepthBuf && !m_nrAttachments) {
#ifndef ARA_USE_GLES31
        glDrawBuffer(GL_NONE);
#endif
    } else if (m_nrAttachments > 1) {
        glDrawBuffers(m_nrAttachments, &m_bufModes[0]);
    }
}

void FBO::unbind(bool doRestoreStates) {
    if (doRestoreStates) {
        restoreStates();
        glViewport(m_csVp[0], m_csVp[1], m_csVp[2], m_csVp[3]);
        glScissor(m_csVp[0], m_csVp[1], m_csVp[2], m_csVp[3]);  // wichtig!!!
    }

    // if using mimaplevels, generate new mipmaps
    if (m_mipMapLevels > 1) {
        for (short i = 0; i < m_nrAttachments; i++) {
            glBindTexture(m_target, m_textures[i]);
            glGenerateMipmap(m_target);
            glBindTexture(m_target, 0);
        }
    }
}

// OpenGLES 3.0
void FBO::blit(uint scrWidth, uint scrHeight, GLenum interp) const {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_lastBoundFbo);
    glBlitFramebuffer(0, 0, m_tex_width, m_tex_height, 0, 0, scrWidth, scrHeight, GL_COLOR_BUFFER_BIT, interp);
    if (m_hasDepthBuf)
        glBlitFramebuffer(0, 0, m_tex_width, m_tex_height, 0, 0, scrWidth, scrHeight, GL_DEPTH_BUFFER_BIT, interp);
}

void FBO::blitTo(const FBO *dst) const {
    if (!dst) {
        return;
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->getFbo());
    glBlitFramebuffer(0, 0, m_tex_width, m_tex_height, 0, 0, dst->getWidth(), dst->getHeight(), GL_COLOR_BUFFER_BIT,
                      GL_NEAREST);
    if (m_hasDepthBuf) {
        glBlitFramebuffer(0, 0, m_tex_width, m_tex_height, 0, 0, dst->getWidth(), dst->getHeight(), GL_DEPTH_BUFFER_BIT,
                          GL_NEAREST);
    }
}

void FBO::resize(float width, float height, float depth, bool checkStates) {
    resize( static_cast<uint>(width), static_cast<uint>(height), static_cast<uint>(depth), checkStates);
}

/**
 * Best practise is to use OpenGL immutable storage (glTexStorage*) which can't be resized. So for resizing all textures
 * get destroyed and reallocated with the same parameters
 */
void FBO::resize(uint width, uint height, uint depth, bool checkStates) {
    if (!width) {
        return;
    }

    m_tex_width    = width;
    m_tex_height   = height;
    m_f_tex_width  = static_cast<float>(m_tex_width);
    m_f_tex_height = static_cast<float>(m_tex_height);
    m_f_tex_depth  = static_cast<float>(m_tex_depth);

    if ((m_tex_width != 0 || m_tex_height != 0) && !m_inited) {
        init();
    } else if (!m_isShared) {
        if (checkStates) {
            getActStates();
        }

        // destroy and recreate textures
        deleteColorTextures();
        allocColorTexture();

        if (m_hasDepthBuf) {
            deleteDepthTextures();
            allocDepthTexture();
        }

        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

        // attach the new textures to the old FBO which can be reused
        attachTextures(false);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if (checkStates) {
            restoreStates();
        }
    }
}

void FBO::getActStates() {
    // if there is any FBO bound, remember it now, and rebind it later again
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &m_lastBoundFbo);
#ifndef ARA_USE_GLES31
    glGetBooleanv(GL_MULTISAMPLE, &m_lastMultiSample);
#endif
    for (GLuint i = 0; i < m_maxNrDrawBuffers; i++) {
        glGetIntegerv(GL_DRAW_BUFFER0 + i, &m_lastDrawBuffers[i]);
    }
}


void FBO::restoreStates() {
    // rebind last s_fbo
    glBindFramebuffer(GL_FRAMEBUFFER, m_lastBoundFbo);

    if (m_lastBoundFbo != 0) {
        m_restAttachments.clear();
        m_restNrValidBuffers = 0;
        for (GLuint i = 0; i < m_maxNrDrawBuffers; i++) {
            if (m_lastDrawBuffers[i]) {
                m_restAttachments.emplace_back(GL_COLOR_ATTACHMENT0 + i);
                m_restNrValidBuffers++;
            }
        }
        glDrawBuffers(m_restNrValidBuffers, &m_restAttachments[0]);
    }

#ifndef ARA_USE_GLES31
    if (!m_lastMultiSample) {
        glDisable(GL_MULTISAMPLE);
    } else {
        glEnable(GL_MULTISAMPLE);
    }
#endif
}

void FBO::deleteColorTextures() const {
    if (m_inited) {
        glDeleteTextures(m_nrAttachments, &m_textures[0]);
    }
}

void FBO::deleteDepthTextures() const {
    if (m_inited && m_hasDepthBuf) {
        glDeleteTextures(1, &m_depthBuf);
    }
}

void FBO::clearAlpha(float alpha, float col) const {
    glClearColor(col, col, col, 1.f - alpha);
    glClear(GL_COLOR_BUFFER_BIT);

    if (m_hasDepthBuf) {
        glClear(GL_DEPTH_BUFFER_BIT);
        glClearDepthf(1.f);
    }
}

void FBO::clearToAlpha(float alpha) const {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    m_toAlphaShader->begin();
    m_toAlphaShader->setIdentMatrix4fv("m_pvm");
    m_toAlphaShader->setUniform1f("alpha", alpha);

    // don't use the VAO itself since it's not gl context save
    m_quad->draw();

    Shaders::end();

    if (m_hasDepthBuf || m_type == GL_DEPTH24_STENCIL8) {
        glClear(GL_DEPTH_BUFFER_BIT);
        glClearDepthf(1.f);
    }
}

void FBO::clearToColor(float r, float g, float b, float a) const {
    if (m_nrAttachments > 0 && m_type != GL_DEPTH24_STENCIL8) {
        glDrawBuffers(m_nrAttachments, &m_bufModes[0]);
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    if (m_hasDepthBuf) {
        glClear(GL_DEPTH_BUFFER_BIT);
        glClearDepthf(1.f);
    }
}

void FBO::clearToColor(float r, float g, float b, float a, size_t bufIndx) const {
    if (m_nrAttachments >= static_cast<int>(bufIndx) && m_type != GL_DEPTH24_STENCIL8) {
        std::array col = {r, g, b, a};
        glClearBufferfv(GL_COLOR, static_cast<GLint>(bufIndx), &col[0]);
    }
}

void FBO::clear() const {
    if (m_nrAttachments > 0 && m_type != GL_DEPTH24_STENCIL8) {
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    if (m_nrAttachments > 1 && m_type != GL_DEPTH24_STENCIL8) {
        glDrawBuffers(m_nrAttachments, &m_bufModes[0]);
        for (int i = 1; i < m_nrAttachments; i++) {
            glClearBufferfv(GL_COLOR, i, m_transparent);
        }
    }

    if (m_hasDepthBuf) {
        glClearDepthf(1.f);
        glClear(GL_DEPTH_BUFFER_BIT);
    }
}

void FBO::clearDepth() const {
    if (m_hasDepthBuf) {
        glClearDepthf(1.f);
        glClear(GL_DEPTH_BUFFER_BIT);
    }
}

void FBO::clearWhite() const {
    if (m_nrAttachments > 0 && m_type != GL_DEPTH24_STENCIL8) {
        glClearColor(1.0, 1.0, 1.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    if (m_hasDepthBuf) {
        glClear(GL_DEPTH_BUFFER_BIT);
        glClearDepthf(1.f);
    }
}

void FBO::setMinFilter(GLint type) {
    m_minFilterType = type;

    for (int i = 0; i < m_nrAttachments; i++) {
        glBindTexture(m_target, m_textures[i]);
        glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, type);
    }
}

void FBO::setMagFilter(GLint type) {
    m_magFilterType = type;

    for (int i = 0; i < m_nrAttachments; i++) {
        glBindTexture(m_target, m_textures[i]);
        glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, type);
    }
}

void FBO::setMinFilter(GLint type, int attNr) const {
    if (attNr < m_nrAttachments) {
        glBindTexture(m_target, m_textures[attNr]);
        glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, type);
    }
}

void FBO::setMagFilter(GLint type, int attNr) const {
    if (attNr < m_nrAttachments) {
        glBindTexture(m_target, m_textures[attNr]);
        glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, type);
    }
}

void FBO::set3DLayer(int attachment, int offset) const {
#ifndef ARA_USE_GLES31
    if (m_target == GL_TEXTURE_3D && static_cast<GLuint>(offset) < m_tex_depth) {
        // glFramebufferTextureLayer(GL_FRAMEBUFFER,
        // GL_COLOR_ATTACHMENT0+attachment, textures[attachment], 0, offset);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
        glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment, m_target, m_textures[attachment], 0,
                               offset);

        if (m_hasDepthBuf) {
            glFramebufferTexture3D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_target, m_depthBuf, 0, offset);
            glFramebufferTexture3D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, m_target, m_depthBuf, 0, offset);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
#endif
}

void FBO::printFramebufferLimits() {
    int res;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &res);
    LOG << "Max Color Attachments: " << res;

    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &res);
    LOG << "Max Framebuffer width: " << res << " height: " << res;
}

void FBO::checkFbo() {
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    if (GL_FRAMEBUFFER_COMPLETE != status) {
        switch (status) {
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: LOGE << "FBO Error: Attachment Point Unconnected"; break;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: LOGE << "FBO Error: Missing Attachment"; break;
#ifndef ARA_USE_GLES31
            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
                LOGE << "FBO Error: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
                LOGE << "FBO Error: GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
                break;
#endif
            case GL_FRAMEBUFFER_UNSUPPORTED:
                LOGE << "Framebuffer Object %d Error: Unsupported Framebuffer "
                        "Configuration";
                break;
            default: break;
        }
    }
}

bool FBO::saveToFile(const filesystem::path &filename, size_t attachNr, GLenum intFormat) {
#ifdef ARA_USE_FREEIMAGE
    if (!m_inited || static_cast<int>(attachNr) >= m_nrAttachments) return false;

    FREE_IMAGE_FORMAT fileFormat = {};
    FIBITMAP         *bitmap = nullptr;
    int               flags = 0;

    if (!filename.extension().compare(".gif")) {
        fileFormat = FIF_GIF;
    } else if (!filename.extension().compare(".jpg") || !filename.extension().compare(".jpeg")) {
        fileFormat = FIF_JPEG;
    } else if (!filename.extension().compare(".png")) {
        fileFormat = FIF_PNG;
    } else if (!filename.extension().compare(".bmp")) {
        fileFormat = FIF_BMP;
    } else if (!filename.extension().compare(".tif")) {
        fileFormat = FIF_TIFF;
        flags |= TIFF_DEFLATE;
    } else if (!filename.extension().compare(".tga")) {
        fileFormat = FIF_TARGA;
    }

    GLenum format     = ara::getExtType(intFormat);
    m_extType         = format;
    m_pixType         = ara::getPixelType(intFormat);
    int    nrChan     = getNrColChans(intFormat);
    GLuint texId      = m_textures[attachNr];
    GLenum saveTarget = m_target;

    // in case we are dealing with a multisample texture, first render it into a temporary single sample texture,
    // then download
    if (m_isMultiSample) {
        saveTarget = GL_TEXTURE_2D;
        auto tempFbo = make_unique<FBO>(FboInitParams{
            m_glbase,
            static_cast<int>(m_tex_width),
            static_cast<int>(m_tex_height),
            1,
            intFormat,
            saveTarget,
            false, 1, 1, 1,
            GL_REPEAT, false
        });

        auto copyShader = m_shCol->getStdTexAlpha(true);

        glDisable(GL_BLEND);
        tempFbo->bind();
        copyShader->begin();
        copyShader->setIdentMatrix4fv("m_pvm");
        copyShader->setUniform1f("alpha", 1.f);
        copyShader->setUniform1i("tex", 0);
        copyShader->setUniform1i("nrSamples", m_nrSamples);
        copyShader->setUniform2f("scr_size", m_f_tex_width, m_f_tex_height);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(m_target, m_textures[attachNr]);

        m_quad->draw();
        tempFbo->unbind();

        glEnable(GL_BLEND);

        texId = tempFbo->getColorImg();
    }

    glBindTexture(saveTarget, texId);

    switch (m_pixType) {
        case GL_UNSIGNED_SHORT: {
            if (format == GL_RED) {
                bitmap = FreeImage_AllocateT(FIT_UINT16, m_tex_width, m_tex_height);
            } else {
                LOGE << "Texture::saveTexToFile2D Error: unknown m_format";
            }

            if (bitmap) {
                auto bits = FreeImage_GetBits(bitmap);
#ifdef ARA_USE_GLES31
                glesGetTexImage(texId, GL_TEXTURE_2D, GL_RED, GL_UNSIGNED_SHORT, m_tex_width, m_tex_height, bits);
#else
                glGetTexImage(saveTarget, 0, format, m_pixType, bits);
#endif
            } else
                LOGE << "Texture::saveTexToFile2D Error: could not allocate "
                        "bitmap";
            break;
        }
        case GL_UNSIGNED_BYTE: {
            bitmap = FreeImage_Allocate(m_tex_width, m_tex_height, nrChan * 8);
            if (bitmap) {
                FreeImage_SetTransparent(bitmap, ara::getExtType(m_type) == GL_RGBA);
                getTexImage(bitmap, saveTarget, format);
            } else
                LOGE << "Texture::saveTexToFile2D Error: could not allocate "
                        "bitmap";
            break;
        }
        case GL_FLOAT: {
            switch (m_type) {
                case GL_R32F: bitmap = FreeImage_AllocateT(FIT_FLOAT, m_tex_width, m_tex_height); break;
                case GL_RGB16F: bitmap = FreeImage_AllocateT(FIT_RGB16, m_tex_width, m_tex_height); break;
                case GL_RGBA16F: bitmap = FreeImage_AllocateT(FIT_RGBA16, m_tex_width, m_tex_height); break;
                case GL_RGB32F: bitmap = FreeImage_AllocateT(FIT_RGBF, m_tex_width, m_tex_height); break;
                case GL_RGBA32F: bitmap = FreeImage_AllocateT(FIT_RGBAF, m_tex_width, m_tex_height); break;
                default: LOGE << "Texture::saveTexToFile2D Error: unknown m_format"; break;
            }
            getTexImage(bitmap, saveTarget, format);
            break;
        }
        default: LOGE << "Texture::saveTexToFile2D Error: Unknown pixel m_format"; break;
    }

    if (!FreeImage_Save(fileFormat, bitmap, filename.string().c_str(), flags))
        LOGE << "Texture::saveTexToFile2D Error: FreeImage_Save failed";
    else
        FreeImage_Unload(bitmap);
#endif
    return true;
}

void FBO::getTexImage(FIBITMAP* bitmap, GLenum saveTarget, GLenum format) const {
    if (bitmap) {
        auto *bits = FreeImage_GetBits(bitmap);
#ifdef ARA_USE_GLES31
        glesGetTexImage(texId, saveTarget, format, m_pixType, m_tex_width, m_tex_height, bits);
#else
        glGetTexImage(saveTarget, 0, format, m_pixType, bits);
#endif
    } else
        LOGE << "Texture::saveTexToFile2D Error: could not allocate "
                "bitmap";
}

void FBO::download(void *ptr, GLenum intFormat, GLenum extFormat) const {
    glBindTexture(m_target, getColorImg());
    glPixelStorei(GL_PACK_ALIGNMENT, 4);  // should be 4

    // synchronous, blocking command, no swap() needed
#ifdef ARA_USE_GLES31
    if (ptr)
        glesGetTexImage(getColorImg(), m_target, extFormat ? extFormat : ara::getExtType(intFormat), ara::getPixelType(intFormat),
                        m_tex_width, m_tex_height, (GLubyte *)ptr);
#else
    if (ptr)
        glGetTexImage(m_target, 0, extFormat ? extFormat : ara::getExtType(intFormat), ara::getPixelType(intFormat), ptr);
#endif
}

void FBO::genFbo() {
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
}

void FBO::deleteFbo() const {
    glDeleteFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint FBO::getColorImg() const {
    return (!m_textures.empty() && m_nrAttachments > 0 && m_type != GL_DEPTH24_STENCIL8) ? m_textures[0] : 0;
}

GLuint FBO::getColorImg(int index) const {
    return static_cast<int>(m_textures.size()) > index && m_nrAttachments > index && m_type != GL_DEPTH24_STENCIL8 ? m_textures[index] : 0;
}

}  // namespace ara
