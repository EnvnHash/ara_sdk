#pragma once

#include <DataModel/Project.h>
#include <util_common.h>

// obsolete. to be changed to Node derivate

namespace ara {

class Workspace : public Item {
public:
    Workspace();

    static Workspace *inst() {
        if (!m_inst) {
            m_inst = new Workspace();
        }
        return m_inst;
    }

    static void deleteInst() {
        m_inst->exit();
        if (m_inst) {
            delete m_inst;
            m_inst = nullptr;
        }
    }

    static Project *activeProject() { return inst()->getActiveProject(); }

    virtual void init();
    virtual void load(const std::filesystem::path& filename);
    virtual void saveAs(std::filesystem::path filename, bool showInfo = true);
    virtual void save(bool showInfo = true);
    virtual bool createNewSettings(const std::filesystem::path& path);
    virtual bool loadSettings(const std::filesystem::path& path);
    virtual bool saveSettings(const std::filesystem::path& path);

    virtual void              exit() {}
    void                      setCWD(std::string const &cwd) { m_cwd = cwd; }
    std::string               getCWD() const { return m_cwd.string(); }
    std::string               homeDirectory() { return m_homeDirectory; }
    std::list<RecentProject> &getRecentItems() { return m_recentProjects; }
    Project *                 getActiveProject() { return m_activeProject ? m_activeProject.get() : nullptr; }

    std::unique_ptr<Project> m_activeProject;
    static Workspace        *m_inst;

protected:
    std::filesystem::path    m_cwd;
    std::filesystem::path    m_settingsFile;
    std::string              m_homeDirectory;
    std::string              m_appName;
    std::list<RecentProject> m_recentProjects;

    bool m_reqSaveXml = false;
};

}  // namespace ara
