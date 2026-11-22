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
    const auto seedShdrSrc = ShaderCollector::getShaderHeader() + STRINGIFY(
    layout(local_size_x = 16, local_size_y = 16) in;

    layout(rgba8, binding = 0) uniform readonly image2D srcImage;
    layout(r8, binding = 1) uniform writeonly image2D maskOut;

    // Store the sampled target color in an SSBO so CPU doesn't have to glReadPixels
    layout(std430, binding = 0) buffer SeedData {
        vec4 targetColor;
    };

    uniform ivec2 mousePos; // Pixel coordinates [0, width-1], [0, height-1]

    void main() {
        ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
        ivec2 size = imageSize(srcImage);
        if (coord.x >= size.x || coord.y >= size.y) return;

        // Thread at mouse coordinate samples the target color into the SSBO
        if (coord == mousePos) {
            targetColor = imageLoad(srcImage, mousePos);
            imageStore(maskOut, coord, vec4(1.0));
        } else {
            imageStore(maskOut, coord, vec4(0.0));
        }
    });

    m_colMaskSeedShdr = m_shCol->add("PaintImageColMaskSeedShader", seedShdrSrc);

    auto shdrSrc = STRINGIFY(
        layout(rgba8, binding = 0) uniform readonly image2D srcImage;
        layout(r8, binding = 1) uniform readonly image2D maskIn;
        layout(r8, binding = 2) uniform writeonly image2D maskOut;

        layout(std430, binding = 0) buffer SeedData {
            vec4 targetColor;
        };

        layout(binding = 0, offset = 0) uniform atomic_uint changedCount;
        uniform float tolerance; // e.g., 0.15

        vec3 srgbToLinear(vec3 c) {
            return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(0.04045, c));
        }

        vec3 rgbToLab(vec3 rgb) {
            vec3 lin = srgbToLinear(rgb);
            mat3 toXYZ = mat3(
                0.4124564, 0.2126729, 0.0193339,
                0.3575761, 0.7151522, 0.1191920,
                0.1804375, 0.0721750, 0.9503041
            );
            vec3 xyz = (toXYZ * lin) / vec3(0.95047, 1.00000, 1.08883);
            vec3 f = mix((7.787 * xyz) + (16.0 / 116.0), pow(xyz, vec3(1.0 / 3.0)), step(0.008856, xyz));
            return vec3((116.0 * f.y) - 16.0, 500.0 * (f.x - f.y), 200.0 * (f.y - f.z));
        }

        // Check 4-connected neighbors (use 8 offsets if 8-way Magic Wand is desired)
        const ivec2 offsets[4] = ivec2[4](
            ivec2( 1,  0),
            ivec2(-1,  0),
            ivec2( 0,  1),
            ivec2( 0, -1)
        );

        void main() {
            ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
            ivec2 size = imageSize(srcImage);
            if (coord.x >= size.x || coord.y >= size.y) return;

            float currentMask = imageLoad(maskIn, coord).r;

            if (currentMask > 0.5) {
                imageStore(maskOut, coord, vec4(1.0, 0.0, 0.0, 0.0));
                return;
            }

            bool hasFilledNeighbor = false;
            for (int i = 0; i < 4; ++i) {
                ivec2 nCoord = coord + offsets[i];
                if (nCoord.x >= 0 && nCoord.x < size.x && nCoord.y >= 0 && nCoord.y < size.y) {
                    if (imageLoad(maskIn, nCoord).r > 0.5) {
                        hasFilledNeighbor = true;
                        break;
                    }
                }
            }

            if (hasFilledNeighbor) {
                vec3 pixelLab = rgbToLab(imageLoad(srcImage, coord).rgb);
                vec3 targetLab = rgbToLab(targetColor.rgb);
                float dist = distance(pixelLab, targetLab) / 100.0;

                if (dist <= tolerance) {
                    imageStore(maskOut, coord, vec4(1.0, 0.0, 0.0, 0.0));
                    atomicCounterIncrement(changedCount);
                    return;
                }
            }
            imageStore(maskOut, coord, vec4(0.0));
        }
    );

    const auto hdr = ShaderCollector::getShaderHeader() + "layout(local_size_x=16, local_size_y=16,local_size_z=1) in;\n";
    const auto src = hdr + shdrSrc;
    m_colMaskShdr = m_shCol->add("SpriteBBGenerator", src);
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
            it->allocate2D(m_ownedTex->getWidth(), m_ownedTex->getHeight(), GL_R8, GL_RED, GL_TEXTURE_2D, GL_UNSIGNED_BYTE);
            it->clear();
        }
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
        colorSelect(data.mousePos, col, m_selTolerance);
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
    const auto tex = getTexture();
    if (!tex) {
        return;
    }

    const auto mp = getMousePosInTexPixSpace(tex, mousePos);
    if (mp == std::nullopt) {
        return;
    }

    setMasking(true);

    const auto targetCol = make_unique<ShaderBuffer<vec4>>(1);
    const auto ptr = targetCol->map();
    for (int i=0; i<4; ++i) {
        (*ptr)[i] = col[i];
    }
    targetCol->unmap();


    // seeding
    m_colMaskSeedShdr->begin();
    m_colMaskSeedShdr->bindProgramPipeline();
    m_colMaskSeedShdr->setUniform2i("mousePos", mp.value().x, mp.value().y);

    glBindImageTexture(0, tex->getId(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
    glBindImageTexture(1, m_maskTex[0]->getId(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, targetCol->getBuffer());
    glDispatchCompute((tex->getWidth() + 15) / 16, (tex->getHeight() + 15) / 16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

    // 2. Ping-pong expand until wavefront stops
    m_colMaskShdr->begin();
    m_colMaskShdr->bindProgramPipeline();
    m_colMaskShdr->setUniform1f("tolerance", tolerance);

    m_maskSeedReadIdx = 0;
    GLuint changed = 1;
    GLuint atomicBuffer = 0;
    constexpr GLuint initZero = 0;

    glGenBuffers(1, &atomicBuffer);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicBuffer);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &initZero, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, atomicBuffer);

    while (changed > 0) {
        // Reset atomic counter to 0
        GLuint zero = 0;
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicBuffer);
        glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &zero);

        const int writeIdx = 1 - m_maskSeedReadIdx;
        glBindImageTexture(0, tex->getId(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
        glBindImageTexture(1, m_maskTex[m_maskSeedReadIdx]->getId(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8);
        glBindImageTexture(2, m_maskTex[writeIdx]->getId(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);

        glDispatchCompute((tex->getWidth() + 15) / 16, (tex->getHeight() + 15) / 16, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

        // Note: In real-time loops, check atomic counter every ~32 steps to avoid GPU-CPU sync stalls
        glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &changed);

        m_maskSeedReadIdx = writeIdx;
        LOG << "m_maskSeedReadIdx: " << m_maskSeedReadIdx;
    }

    Shaders::unbindProgramPipeline();
    Shaders::end();

    m_maskTex[m_maskSeedReadIdx]->saveTexToFile2D("mask.png", FIF_PNG,  m_maskTex[m_maskSeedReadIdx]->getWidth(), m_maskTex[m_maskSeedReadIdx]->getHeight(), GL_R8, m_maskTex[m_maskSeedReadIdx]->getId());


    if (m_showMask && m_maskUiImg) {
        LOG << "id: " << m_maskTex[m_maskSeedReadIdx]->getId();
        m_maskUiImg->setTexId(m_maskTex[m_maskSeedReadIdx]->getId(), m_maskTex[m_maskSeedReadIdx]->getWidth(), m_maskTex[m_maskSeedReadIdx]->getHeight(), 8);
    }
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

} // namespace ara
