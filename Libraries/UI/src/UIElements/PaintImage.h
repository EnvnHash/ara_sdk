#pragma once

#include "UIElements/Image.h"
#include <Utils/FBO.h>
#include <Utils/UniformBlock.h>

namespace ara {

class PaintImage : public Image {
public:
    enum class mode : int32_t { add=0, subtract };
    struct Brush {
        float size = 10.0f;
        float hardness = 0.5f;
        glm::vec4 color = glm::vec4(1.0f);
        float opacity = 1.0f;
    };

    PaintImage();
    ~PaintImage() override = default;

    void init() override;

    void mouseDown(hidData& data) override;
    void mouseDrag(hidData& data) override;
    void mouseUp(hidData& data) override;

    void undo() override;
    void redo() override;
    void clearBackupPath() const;

    Brush& getBrush() { return m_brush; }

    void lock(const bool val) { m_locked = val; }
    void setBrush(const Brush& brush) { m_brush = brush; }
    void setMode(const mode& mode) { m_paintMode = mode; }
    void setBackupPath(const std::filesystem::path& p);
    void setBrushSize(float brushSize);
    void setBrushHardness(float hardness);
    void saveToFile(const std::filesystem::path &filename) const;
    virtual void setSharedTexture(const std::shared_ptr<Texture>& tex);

    void setMaxBackupCopies(const size_t maxCopies) { m_maxBackupCopies = maxCopies; }
    [[nodiscard]] size_t getMaxBackupCopies() const { return m_maxBackupCopies; }
    [[nodiscard]] const std::vector<std::filesystem::path>& getBackupCopies() const { return m_backupCopies; }

protected:
    void initFbo();
    void createBackup();
    void saveCurrentState() const;
    virtual void paint(const glm::vec2& mousePos);

    Brush                               m_brush{};
    std::unique_ptr<FBO>                m_fbo;
    std::shared_ptr<Texture>            m_ownedTex;
    Shaders*                            m_paintShader = nullptr;
    UniformBlock                        m_brushBlock{};
    float                               m_scaledBrush{};
    bool                                m_locked{};
    mode                                m_paintMode = mode::add;
    size_t                              m_maxBackupCopies = 20;
    std::vector<std::filesystem::path>  m_backupCopies;
    std::filesystem::path               m_backupPath;
    static inline std::filesystem::path m_backupSubPath = "paintImage";
    int                                 m_historyIndex = -1;
};

} // namespace ara
