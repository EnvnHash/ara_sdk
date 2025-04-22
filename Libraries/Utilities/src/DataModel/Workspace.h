//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

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
    virtual void saveAs(const std::filesystem::path& filename, bool showInfo = true);
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
