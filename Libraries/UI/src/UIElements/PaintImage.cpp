#include "PaintImage.h"
#include <UISharedRes.h>
#include <Shaders/ShaderCollector.h>
#include <Utils/Texture.h>
#include <GLBase.h>

#include "UIWindow.h"

using namespace glm;
using namespace std;

namespace ara {

PaintImage::PaintImage() {
    setTypeName<PaintImage>();
    setName(getTypeName<PaintImage>());
    m_maxUndoBufSize = 40;
}

void PaintImage::init() {
    Image::init();

    if (m_sharedRes && m_sharedRes->shCol) {
        initPaintShader();
        initColorMaskShader();
    }

    initFbo();
    saveState();
}

void PaintImage::initPaintShader() {
    const auto vert = ShaderCollector::getShaderHeader() + STRINGIFY(
            layout(location = 0) in vec3 position;
            uniform float size;
            uniform vec2 pos;
            out vec2 texCoord;
            void main() {
                const vec2[4] quadVertices = vec2[4](vec2(-1., -1.), vec2(1., -1.), vec2(-1., 1.), vec2(1., 1.));
                texCoord = quadVertices[gl_VertexID];
                gl_Position = vec4(quadVertices[gl_VertexID], 0.0, 1.0);
            }
        );

        const auto frag = ShaderCollector::getShaderHeader() + STRINGIFY(
            layout(location = 0) out vec4 fragColor;
            in vec2 texCoord;
            struct Brush {
                float size;
                float hardness;
                vec4 color;
                float opacity;
            };
            uniform vec2 pos;
            uniform vec4 limits;
            uniform int masked;
            uniform sampler2D maskTex;
            layout(std140) uniform BrushBlock {
                Brush brush;
            };
            void main() {
                if (texCoord.x < limits.x || texCoord.x > limits.z || texCoord.y > limits.y || texCoord.y < limits.w) {
                    discard;
                }
                float dist = distance(texCoord, pos);
                float edge = clamp((1.0 - brush.hardness), 0.0001, 1.0);
                float maskAlpha = 1.0;
                if (bool(masked)) {
                    maskAlpha = texture(maskTex, texCoord).a;
                    if (maskAlpha == 0.0) {
                        discard;
                    }
                }
                float falloff = 1.0 - smoothstep(brush.size * (1.0 - edge), brush.size, dist);
                if (falloff == 0.0) {
                    discard;
                }
                fragColor = vec4(brush.color.rgb, brush.opacity * falloff * brush.color.a * maskAlpha);
            }
        );

        m_paintShader = m_sharedRes->shCol->add("paintImageShader", vert, frag);

        if (m_paintShader) {
            m_scaledBrush = m_brush.size;
            m_brushBlock.init(m_paintShader->getProgram(), "BrushBlock");
            m_brushBlock.addVarName("brush.size", &m_scaledBrush, GL_FLOAT);
            m_brushBlock.addVarName("brush.hardness", &m_brush.hardness, GL_FLOAT);
            m_brushBlock.addVarName("brush.color", &m_brush.color, GL_FLOAT_VEC4);
            m_brushBlock.addVarName("brush.opacity", &m_brush.opacity, GL_FLOAT);
        }
}

void PaintImage::initColorMaskShader() {
    constexpr auto wandPropagate = STRINGIFY(
        layout(local_size_x = 16, local_size_y = 16) in;

        layout(rgba8, binding = 0) readonly uniform image2D uSourceImg;
        layout(r8ui, binding = 1) readonly uniform uimage2D uMaskIn;
        layout(r8ui, binding = 2) writeonly uniform uimage2D uMaskOut;

        layout(binding = 0, offset = 0) uniform atomic_uint uChangedCount;

        layout(location = 0) uniform vec4 uTargetColor;
        layout(location = 1) uniform float uTolerance;
        layout(location = 2) uniform int uEightWay;

        void main() {
         ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
         ivec2 size = imageSize(uSourceImg);
         if (pos.x >= size.x || pos.y >= size.y) return;

         uint state = imageLoad(uMaskIn, pos).r;
         // State 1 = Selected, State 2 = Rejected
         if (state != 0u) {
             imageStore(uMaskOut, pos, uvec4(state, 0, 0, 0));
             return;
         }

         // Check color distance
         vec4 col = imageLoad(uSourceImg, pos);
         if (distance(col.rgb, uTargetColor.rgb) > uTolerance) {
             imageStore(uMaskOut, pos, uvec4(2u, 0, 0, 0)); // Mark rejected permanently
             return;
         }

         // Check 4-way or 8-way neighbors
         const ivec2 offsets[8] = ivec2[](
             ivec2(-1, 0), ivec2(1, 0), ivec2(0, -1), ivec2(0, 1),
             ivec2(-1, -1), ivec2(1, -1), ivec2(-1, 1), ivec2(1, 1)
         );
         int count = uEightWay == 1 ? 8 : 4;

         for (int i = 0; i < count; ++i) {
             ivec2 nPos = pos + offsets[i];
             if (nPos.x >= 0 && nPos.y >= 0 && nPos.x < size.x && nPos.y < size.y) {
                 if (imageLoad(uMaskIn, nPos).r == 1u) {
                     imageStore(uMaskOut, pos, uvec4(1u, 0, 0, 0));
                     atomicCounterIncrement(uChangedCount);
                     return;
                 }
             }
         }

         imageStore(uMaskOut, pos, uvec4(0u, 0, 0, 0));
    });

    auto src = ShaderCollector::getShaderHeader() + wandPropagate;
    m_colMaskPropagateShdr = m_shCol->add("PaintImageColMaskShdr", src);

    constexpr auto updtIndir = STRINGIFY(
        layout(local_size_x = 1) in;
        layout(binding = 0, offset = 0) uniform atomic_uint uChangedCount;
        layout(std430, binding = 0) writeonly buffer IndirectCmd {
            uint num_groups_x;
            uint num_groups_y;
            uint num_groups_z;
        };

        layout(location = 0) uniform uvec2 uDispatchGroups;

        void main() {
            uint changed = atomicCounterExchange(uChangedCount, 0u);
            if (changed > 0u) {
                num_groups_x = uDispatchGroups.x;
                num_groups_y = uDispatchGroups.y;
                num_groups_z = 1u;
            } else {
                // Setting any group count to 0 turns future indirect dispatches into hardware no-ops
                num_groups_x = 0u;
                num_groups_y = 0u;
                num_groups_z = 0u;
            }
        }
    );

    src = ShaderCollector::getShaderHeader() + updtIndir;
    m_colMaskIndirUpdtShdr = m_shCol->add("PaintImageColMaskUpdtIndirShdr", src);
}

void PaintImage::initFbo() {
    if (!m_fbo) {
        m_fbo = std::make_unique<FBO>();
        m_fbo->setGlbase(getSharedRes()->glbase);
    }

    if (m_tex) {
        m_fbo->fromTexMan(m_tex);
    } else if (m_texId) {
        m_fbo->fromTex(m_texId, m_texSize.x, m_texSize.y, GL_RGBA8, 1, GL_LINEAR, GL_LINEAR);
    }
}

void PaintImage::loadImg(const bool forceReload) {
    Image::loadImg(forceReload);
    m_ownedTex = std::make_shared<Texture>(*m_tex);
    initFbo();
}

void PaintImage::setSharedTexture(const std::shared_ptr<Texture>& tex) {
    m_tex = tex.get();
    m_ownedTex = tex;
    initFbo();
    if (m_drawMasked) {
        initMaskTex();
    }
}

void PaintImage::setMasking(const bool val) {
    m_drawMasked = val;
    if (val) {
        initMaskTex();
    }
}

void PaintImage::initMaskTex() {
    if (!m_ownedTex || m_ownedTex->getWidth() <= 0 || m_ownedTex->getHeight() <= 0) {
        return;
    }

    if (!m_maskTex[0]) {
        for (auto& it : m_maskTex) {
            it = std::make_shared<Texture>();
            it->setGlbase(getSharedRes()->glbase);
            it->allocate2D(m_ownedTex->getWidth(), m_ownedTex->getHeight(), GL_R8UI, GL_RED_INTEGER, GL_TEXTURE_2D, GL_UNSIGNED_BYTE);
            it->setSamplerFiltering(GL_NEAREST, GL_NEAREST);
            it->clear();
        }

        glCreateBuffers(1, &m_atomicBuffer);
        constexpr GLuint zero = 0;
        glNamedBufferStorage(m_atomicBuffer, sizeof(GLuint), &zero, GL_DYNAMIC_STORAGE_BIT);

        glCreateBuffers(1, &m_indirectBuffer);
        glNamedBufferStorage(m_indirectBuffer, sizeof(DispatchIndirectCommand), nullptr, GL_DYNAMIC_STORAGE_BIT);
    }
}

void PaintImage::mouseDown(hidData& data) {
    Image::mouseDown(data);
    if (m_paintMode != mode::colorPick && m_paintMode != mode::mask) {
        paint(data.mousePosNodeRel);
    }
}

void PaintImage::mouseDrag(hidData& data) {
    Image::mouseDrag(data);
    if (m_paintMode != mode::colorPick && m_paintMode != mode::mask && data.mousePressed[mouseButt::left]) {
        paint(data.mousePosNodeRel);
    }
}

void PaintImage::mouseWheel(hidData& data) {
    setCursor();
}

void PaintImage::mouseUp(hidData& data) {
    Image::mouseUp(data);
    if (m_paintMode == mode::colorPick) {
        pickColor(data.mousePosNodeRel);
    } else if (m_paintMode == mode::mask) {
        auto col = pickColor(data.mousePosNodeRel);
        LOG << "sel col " << glm::to_string(col);
        colorSelect(data.mousePosNodeRel, col, m_selTolerance);
    } else if (data.dragging) {
        saveState();
    }
}

void PaintImage::mouseIn(hidData& data) {
    Image::mouseIn(data);
    setCursor();
}

void PaintImage::mouseOut(hidData& data) {
    Image::mouseIn(data);
    resetCursor();
}

void PaintImage::paint(const vec2& mousePos) {
    if (!(m_tex || m_texId) || !m_paintShader || !m_sharedRes || m_locked) {
        return;
    }

    if (!m_fbo) {
        initFbo();
    }

    m_fbo->bind();
    m_paintShader->begin();

    glBlendEquation(m_paintMode == mode::add ? GL_FUNC_ADD : GL_FUNC_REVERSE_SUBTRACT);
    glBlendFunc(m_paintMode == mode::add ? GL_ONE : GL_SRC_ALPHA, GL_ONE);

    const vec2 fboSize{ static_cast<float>(m_fbo->getWidth()), static_cast<float>(m_fbo->getHeight()) };

    // Set uniform block
    m_scaledBrush = m_brush.size * 2.f / fboSize.x;
    m_brushBlock.update();
    m_brushBlock.bind();

    std::array transPos = { mousePos, vec2(0.0), vec2(m_secSize) };
    if (m_secSize.x != 0 && m_secSize.y != 0) {
        for (auto &it : transPos) {
            it = vec2(m_secPos) + it / m_size * vec2(m_secSize); // fbo pos in pixels
            it = vec2{ it.x / fboSize.x * 2.f - 1.f,
                        it.y / fboSize.y * -2.f + 1.f }; // normalized fbo pos
        }
    }

    m_paintShader->setUniform2f("pos", transPos[0].x, transPos[0].y);
    m_paintShader->setUniform4f("limits", transPos[1].x, transPos[1].y, transPos[2].x, transPos[2].y);

    m_paintShader->setUniform1i("masked", m_drawMasked);
    if (m_maskTex[m_maskSeedReadIdx]) {
        m_maskTex[m_maskSeedReadIdx]->bind(0);
        m_paintShader->setUniform1i("maskTex", 0);
    }

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    m_fbo->unbind();

    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_sharedRes->setDrawFlag(true); // Request redrawing of the UI
}

void PaintImage::setBrushSize(const float brushSize) {
    m_brush.size = brushSize;
}

void PaintImage::setBrushHardness(const float hardness) {
    m_brush.hardness = hardness;
}

void PaintImage::saveToFile(const std::filesystem::path &filename) const {
    if (!m_fbo) {
        return;
    }
    m_fbo->saveToFile(filename, 0, GL_RGBA8);
}

void PaintImage::saveState() {
    if (m_undoing || !m_fbo || m_imageFile.empty()) {
        return;
    }

    const auto p = AssetLoader::getAssetPath() / std::filesystem::path(m_imageFile);
    const auto ext = p.extension().empty() ? ".png" : p.extension().string();
    std::error_code ec;

    // if there were new changes after one or several undos
    // remove all entries which lie in the future after the actual entry
    if (!m_backupCopies.empty() && m_backupCopiesIt != m_backupCopies.end() - 1) {
        for (auto it = std::next(m_backupCopiesIt, 1); it != m_backupCopies.end(); ++it) {
            std::filesystem::remove(*it, ec);
        }
        m_backupCopies.erase(std::next(m_backupCopiesIt, 1), m_backupCopies.end());
    }

    // if the undo queue is filled, delete the first element
    if (m_backupCopies.size() + 1 >= m_maxUndoBufSize) {
        std::filesystem::remove(m_backupCopies.front(), ec);
        m_backupCopies.erase(m_backupCopies.begin());
    }

    // serialize the actual state to a binary json and push it to the undo queue
    const std::string uuidCurrent = generateUUID();
    const auto currentBackupPath = AssetLoader::getAssetPath() / m_backupPath / (uuidCurrent + ext);
    m_fbo->saveToFile(currentBackupPath, 0, GL_RGBA8);
    m_fbo->saveToFile(p, 0, GL_RGBA8);
    m_backupCopies.emplace_back(currentBackupPath);
    m_backupCopiesIt = m_backupCopies.end() - 1; // set the undoBufPtr to the new entry
}

void PaintImage::undo() {
    if (m_backupCopies.empty() || m_backupCopiesIt == m_backupCopies.begin()) {
        return;
    }

    --m_backupCopiesIt;  // set back one entry

    m_undoing = true;
    if (const auto& restorePath = *m_backupCopiesIt; std::filesystem::exists(restorePath)) {
        restoreFile(restorePath);
    }
    m_undoing = false;

    for (auto &it : collectCallbacks(cbType::undo, true)) {
        it(this, "");
    }
}

void PaintImage::redo() {
    if (m_backupCopies.empty()) {
        return;
    }

    if (m_backupCopiesIt != std::prev(m_backupCopies.end())) {
        ++m_backupCopiesIt;
        m_undoing = true;
        if (const auto& restorePath = *m_backupCopiesIt; std::filesystem::exists(restorePath)) {
            restoreFile(restorePath);
        }
        m_undoing = false;
    }
}

void PaintImage::clear() {
    if (m_tex) {
        m_tex->clear();
    }
    if (m_fbo) {
        m_fbo->clear();
    }
    saveState();
}

void PaintImage::restoreFile(const std::filesystem::path &filename) {
    const auto tf = m_imageFile;
    setImg(filename.string());
    loadImg(true);
    m_imageFile = tf;
    initFbo();
    if (getSharedRes()) {
        getSharedRes()->reqRedraw();
    }
}

void PaintImage::setBackupPath(const std::filesystem::path& p) {
    m_backupPath = p;
    if (const auto bp = AssetLoader::getAssetPath() / m_backupPath; !std::filesystem::exists(bp)) {
        std::filesystem::create_directory(bp);
    }
}

void PaintImage::clearBackupPath() const {
    for (const auto p = AssetLoader::getAssetPath() / m_backupPath; const auto& entry : std::filesystem::directory_iterator(p)) {
        std::filesystem::remove_all(entry.path());
    }
}

std::optional<ivec2> PaintImage::getMousePosInTexPixSpace(const Texture* tex, const vec2& mousePos) {
    const auto width = tex->getWidth();
    const auto height = tex->getHeight();
    if (width <= 0 || height <= 0) {
        return std::nullopt;
    }

    // Convert mouse position to pixel coordinates in the texture
    const auto x = mousePos.x / getSize().x * static_cast<float>(width);
    const auto y = (1.f - mousePos.y / getSize().y) * static_cast<float>(height);

    // Clamp to valid range
    return ivec2{
        std::max(0.f, std::min(static_cast<float>(width) - 1.f, x)),
        std::max(0.f, std::min(static_cast<float>(height) - 1.f, y))
    };
}

glm::vec4 PaintImage::pickColor(const vec2& mousePos) {
    const auto tex = getTexture();
    if (!tex) {
        return {};
    }

    const auto mp = getMousePosInTexPixSpace(tex, mousePos);
    if (mp == std::nullopt) {
        return {};
    }

    // Create a temporary FBO to read from the texture
    FBO readFbo;
    readFbo.setGlbase(getSharedRes()->glbase);
    readFbo.fromTex(tex->getId(), tex->getWidthF(),  tex->getHeightF(), GL_RGBA8, 1, GL_NEAREST, GL_NEAREST);
    readFbo.bind();

    // Read the pixel color using glReadPixels
    std::array<GLubyte, 4> pixelData{};
    glReadPixels(mp.value().x, mp.value().y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixelData.data());
    readFbo.unbind();

    // Store the picked color (normalize to 0-1 range)
    m_pickColor = vec4(
        static_cast<float>(pixelData[0]) / 255.f,
        static_cast<float>(pixelData[1]) / 255.f,
        static_cast<float>(pixelData[2]) / 255.f,
        static_cast<float>(pixelData[3]) / 255.f
    );

    return m_pickColor;
}

void PaintImage::colorSelect(const vec2& mousePos, vec4& col, const float tolerance) {
    postGLError();

    const auto tex = getTexture();
    if (!tex) {
        return;
    }

    const auto mp = getMousePosInTexPixSpace(tex, mousePos);
    if (mp == std::nullopt) {
        return;
    }

    LOG << "mp " << glm::to_string(mp.value()) << " col " << glm::to_string(col);
    setMasking(true);

    constexpr GLuint zero = 0;
    constexpr GLuint one = 1;

    // Clear masks to 0 and seed the clicked pixel to 1
    glClearTexImage(m_maskTex[0]->getId(), 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &zero);
    glClearTexImage(m_maskTex[1]->getId(), 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &zero);
    glTextureSubImage2D(m_maskTex[0]->getId(), 0, mp.value().x, mp.value().y, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &one);

    // Initialize indirect buffer with full dispatch dimensions
    const GLuint groupsX = (tex->getWidth() + 15) / 16;
    const GLuint groupsY = (tex->getHeight() + 15) / 16;
    const DispatchIndirectCommand initCmd{ groupsX, groupsY, 1 };
    glNamedBufferSubData(m_indirectBuffer, 0, sizeof(initCmd), &initCmd);
    glNamedBufferSubData(m_atomicBuffer, 0, sizeof(GLuint), &zero);

    // Bind static resources
    glBindImageTexture(0, tex->getId(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, m_atomicBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_indirectBuffer);
    glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, m_indirectBuffer);


    m_colMaskIndirUpdtShdr->begin();
    m_colMaskIndirUpdtShdr->bindProgramPipeline();
    m_colMaskIndirUpdtShdr->setUniform2ui("uDispatchGroups", groupsX, groupsY);

    m_colMaskPropagateShdr->begin();
    m_colMaskPropagateShdr->bindProgramPipeline();
    m_colMaskPropagateShdr->setUniform1f("uTolerance", tolerance);
    m_colMaskPropagateShdr->setUniform4fv("uTargetColor", &col[0]);
    m_colMaskPropagateShdr->setUniform1i("uEightWay", 0);

    int readIdx = 0;
    // Max iterations bounded by worst-case Manhattan/Chebyshev path (e.g., width + height)
    // In practice, batches of 64-256 dispatches take < 0.2ms on modern GPUs
    const int maxIterations = tex->getWidth() + tex->getHeight();

    for (int i = 0; i < maxIterations; ++i) {
        const int writeIdx = 1 - readIdx;

        LOG << i;
        m_colMaskPropagateShdr->begin();
        m_colMaskPropagateShdr->bindProgramPipeline();

        glBindImageTexture(1, m_maskTex[readIdx]->getId(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8UI);
        glBindImageTexture(2, m_maskTex[writeIdx]->getId(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8UI);
        glDispatchComputeIndirect(0);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

        // 2. Update indirect command & reset atomic counter on GPU
        m_colMaskIndirUpdtShdr->begin();
        m_colMaskIndirUpdtShdr->bindProgramPipeline();
        glDispatchCompute(1, 1, 1);

        glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

        readIdx = writeIdx;
    }

    m_maskSeedReadIdx = readIdx;

    Shaders::unbindProgramPipeline();
    Shaders::end();

    postGLError();

    m_maskTex[m_maskSeedReadIdx]->saveTexToFile2D("mask.png", FIF_PNG,  m_maskTex[m_maskSeedReadIdx]->getWidth(), m_maskTex[m_maskSeedReadIdx]->getHeight(), GL_R8UI, m_maskTex[m_maskSeedReadIdx]->getId());

    postGLError();


    /*
    if (m_showMask && m_maskUiImg) {
        LOG << "id: " << m_maskTex[m_maskSeedReadIdx]->getId() << " W:" << m_maskTex[m_maskSeedReadIdx]->getWidth() << " H:" << m_maskTex[m_maskSeedReadIdx]->getHeight();
        m_maskUiImg->setTexId(m_maskTex[m_maskSeedReadIdx]->getId(), m_maskTex[m_maskSeedReadIdx]->getWidth(), m_maskTex[m_maskSeedReadIdx]->getHeight(), 8);
        getSharedRes()->reqRedraw();
    }*/
}

void PaintImage::setCursor() {
#ifdef ARA_USE_GLFW
    if (m_paintMode == mode::colorPick || m_paintMode == mode::mask) {
        getSharedRes()->winHandle->setMouseCursor(WinMouseIcon::crossHair);
    } else {
        setCircleCursor();
    }
#endif
}

void PaintImage::setCircleCursor() {
#ifdef ARA_USE_GLFW
    const auto glfwWin = getWindow()->getWinHandle();
    m_cursor = GLFWWindow::createCircularMouseCursor(static_cast<int>(m_brush.size * getParentContentScale().x), {0.7f, 0.7f, 0.7f, 0.6f});
    glfwSetCursor(static_cast<GLFWwindow*>(glfwWin->getWin()), m_cursor);
#endif
}

void PaintImage::resetCursor() {
#ifdef ARA_USE_GLFW
    if (m_cursor) {
        glfwDestroyCursor(m_cursor);
        m_cursor = nullptr;
    }
#endif
}

void PaintImage::setShowMask(const bool& val) {
    m_showMask = val;
    if (val) {
        if (!m_maskUiImg) {
            m_maskUiImg = &push<Image>(UINodePars{
                .pos = ivec2{0,0},
                .size = vec2{1.f, 1.f}
            });
        }
        if (m_maskTex[m_maskSeedReadIdx]) {
            m_maskUiImg->setTexId(m_maskTex[m_maskSeedReadIdx]->getId(), m_maskTex[m_maskSeedReadIdx]->getWidth(), m_maskTex[m_maskSeedReadIdx]->getHeight(), 8);
        }
    }
}

PaintImage::~PaintImage() {
    glDeleteBuffers(1, &m_atomicBuffer);
    glDeleteBuffers(1, &m_indirectBuffer);
}

} // namespace ara
