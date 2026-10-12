#include "PaintImage.h"
#include <UISharedRes.h>
#include <Shaders/ShaderCollector.h>
#include <Utils/Texture.h>
#include <GLBase.h>

using namespace glm;
using namespace std;

namespace ara {

PaintImage::PaintImage() {
    setTypeName<PaintImage>();
    setName(getTypeName<PaintImage>());
}

void PaintImage::init() {
    Image::init();

    if (m_sharedRes && m_sharedRes->shCol) {
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
            layout(std140) uniform BrushBlock {
                Brush brush;
            };
            void main() {
                if (texCoord.x < limits.x || texCoord.x > limits.z || texCoord.y > limits.y || texCoord.y < limits.w) {
                    discard;
                }
                float dist = distance(texCoord, pos);
                float edge = clamp((1.0 - brush.hardness), 0.0001, 1.0);
                float falloff = 1.0 - smoothstep(brush.size * (1.0 - edge), brush.size, dist);
                if (falloff == 0.0) {
                    discard;
                }
                fragColor = vec4(brush.color.rgb, brush.opacity * falloff * brush.color.a);
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

    initFbo();
}

void PaintImage::initFbo() {
    if (!m_fbo) {
        m_fbo = std::make_unique<FBO>();
    }

    m_fbo->setGlbase(getSharedRes()->glbase);

    if (m_tex) {
        m_fbo->fromTexMan(m_tex);
    } else if (m_texId) {
        m_fbo->fromTex(m_texId, m_texSize.x, m_texSize.y, GL_RGBA8, 1, GL_LINEAR, GL_LINEAR);
    }
}

void PaintImage::setSharedTexture(const std::shared_ptr<Texture>& tex) {
    m_tex = tex.get();
    m_ownedTex = tex;
    initFbo();
}

void PaintImage::mouseDown(hidData& data) {
    Image::mouseDown(data);
    paint(data.mousePosNodeRel);
}

void PaintImage::mouseDrag(hidData& data) {
    Image::mouseDrag(data);
    if (data.dragStart) {
        createBackup();
    }
    paint(data.mousePosNodeRel);
}

void PaintImage::mouseUp(hidData& data) {
    Image::mouseUp(data);
    if (data.dragging) {
        saveCurrentState();
    }
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

void PaintImage::createBackup() {
    if (!m_fbo || m_imageFile.empty()) {
        return;
    }
    const auto p = AssetLoader::getAssetPath() / std::filesystem::path(m_imageFile);
    const auto ext = p.extension().empty() ? ".png" : p.extension().string();

    // If history had undone steps, truncate redo history and delete old redo files
    if (m_historyIndex >= 0 && m_historyIndex < static_cast<int>(m_backupCopies.size()) - 1) {
        for (size_t i = m_historyIndex + 1; i < m_backupCopies.size(); ++i) {
            std::error_code ec;
            std::filesystem::remove(m_backupCopies[i], ec);
            LOG << " truncate undo  " << m_backupCopies[i];
        }
        m_backupCopies.erase(m_backupCopies.begin() + (m_historyIndex + 1), m_backupCopies.end());
    }

    // Save old state from PingPongFbo memory backup
    const std::string uuidCurrent = generateUUID();
    const auto currentBackupPath = m_backupPath / m_backupSubPath / (uuidCurrent + ext);

    m_fbo->saveToFile(currentBackupPath, 0, GL_RGBA8);
    m_backupCopies.emplace_back(currentBackupPath);
    m_historyIndex++;
    LOG << "createBackup " << currentBackupPath << " m_historyIndex " << m_historyIndex;

    // If backup list exceeds maximum amount, delete oldest backup copies
    while (m_backupCopies.size() > m_maxBackupCopies) {
        const auto oldest = m_backupCopies.front();
        std::error_code ec;
        std::filesystem::remove(oldest, ec);
        m_backupCopies.erase(m_backupCopies.begin());
        if (m_historyIndex > 0) {
            m_historyIndex--;
        }
    }
}

void PaintImage::saveCurrentState() const {
    const auto p = AssetLoader::getAssetPath() / std::filesystem::path(m_imageFile);
    LOG << "saveCurrentState to " << p;
    m_fbo->saveToFile(p, 0, GL_RGBA8);
}

void PaintImage::undo() {
    if (m_backupCopies.empty() || m_historyIndex <= 0) {
        return;
    }

    m_historyIndex--;
    if (const auto& restorePath = m_backupCopies[m_historyIndex]; std::filesystem::exists(restorePath)) {
        setImg(restorePath);
        loadImg();
        if (!m_imageFile.empty()) {
            saveToFile(m_imageFile);
        }
        if (getSharedRes()) {
            getSharedRes()->reqRedraw();
        }
    }
}

void PaintImage::redo() {
    if (m_historyIndex < 0 || m_historyIndex >= static_cast<int>(m_backupCopies.size()) - 1) {
        return;
    }

    m_historyIndex++;
    if (const auto& restorePath = m_backupCopies[m_historyIndex]; std::filesystem::exists(restorePath)) {
        setImg(restorePath);
        loadImg();
        if (!m_imageFile.empty()) {
            saveToFile(m_imageFile);
        }
        if (getSharedRes()) {
            getSharedRes()->reqRedraw();
        }
    }
}

void PaintImage::setBackupPath(const std::filesystem::path& p) {
    m_backupPath = p;
    if (const auto bp = m_backupPath / m_backupSubPath; !std::filesystem::exists(bp)) {
        std::filesystem::create_directory(bp);
    }
}

void PaintImage::clearBackupPath() const {
    const auto p = m_backupPath / m_backupSubPath;
    for (const auto& entry : std::filesystem::directory_iterator(p))
        std::filesystem::remove_all(entry.path());
}

} // namespace ara
