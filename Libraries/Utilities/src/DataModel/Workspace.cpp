
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

#include <DataModel/Project.h>
#include <DataModel/Workspace.h>

using namespace std;

namespace fs = std::filesystem;

namespace ara {
Workspace *Workspace::m_inst = nullptr;

Workspace::Workspace() : Item() {
    setTypeName<Workspace>();
}

void Workspace::init() {
    m_cwd          = fs::current_path();
    m_settingsFile = m_cwd / "settings.xml";

    if (fs::exists(m_settingsFile)) {
        try {
            loadSettings(m_settingsFile);
        } catch (...) {
            std::cerr << "Invalid settings file" << std::endl;
        }
    } else {
        createNewSettings(m_settingsFile);
    }

    // create the standard folder for projects in the users Documents Folder
    std::error_code ec;
#ifdef _WIN32
    const char* homeDirName = "USERPROFILE";
#else
    const char* homeDirName = "HOME";
#endif
    auto sanitizedHomeDir = std::getenv(homeDirName)
                            ? std::filesystem::path(std::getenv(homeDirName))
                            : std::filesystem::current_path();
    m_homeDirectory = (sanitizedHomeDir / std::filesystem::path("Documents") / std::filesystem::path(m_appName)).string();

    if (!std::filesystem::create_directory(m_homeDirectory, ec)) {
        if (0 != ec.value())  // already exist
        {
            LOGE << "could not create directory " << m_homeDirectory << ". Reason: " << ec.message();
            throw std::system_error(ec);
        }
    }
}

void Workspace::load(const fs::path& filename) {
    try {
        // clear the actual project before loading
        // clear the item tree
        m_activeProject->clearChildren();  // this should kill all active rendering
        m_activeProject->setFileName(filename);
        auto projPath = m_activeProject->checkWorkDir();
        m_activeProject->load(filename);
        m_activeProject->onLoaded();
    } catch (exception &e) {
        LOGE << "Workspace::load error " << e.what();
    }
}

void Workspace::saveAs(const fs::path& filename, bool showInfo) {
    m_activeProject->name = filename.stem().string();
    m_activeProject->setFileName(filename);
    save(showInfo);
}

void Workspace::save(bool showInfo) {
    try {
        m_activeProject->checkWorkDir();
        m_activeProject->save();

        // check if this project exist in the recentProjects list
        auto p = std::find_if(m_recentProjects.begin(), m_recentProjects.end(), [this](const RecentProject &pr) {
            return !pr.path.compare(m_activeProject->getFileName());
        });

        // get the modification date
        auto ftime = fs::last_write_time(m_activeProject->getFileName());

        // update the values in case of an existing entry or create a new one
        if (p == m_recentProjects.end()) {
            m_recentProjects.emplace_back(
                RecentProject{to_time_t(ftime), m_activeProject->getFileName(), m_activeProject->name()});
        } else {
            p->mod_time = to_time_t(ftime);
            p->name     = m_activeProject->name();
        }

        saveSettings(m_settingsFile);
    } catch (exception &e) {
        LOGE << "Workspace::save error " << e.what();
    }
}

bool Workspace::createNewSettings(const fs::path& path) {
/*    xml_document doc;
    xml_node     root = doc.append_child(m_appName.c_str());

    return doc.save_file(path.string().c_str());
*/
    return true;
}

bool Workspace::loadSettings(const fs::path& path) {
/*    xml_document     doc;
    xml_parse_result result = doc.load_file(path.string().c_str(), parse_default | parse_escapes | parse_doctype);

    if (!result) {
        LOGE << "ERROR, couldn't parse config file " << path.string() << " aborting";
        return false;
    }

    xml_node root = doc.document_element();

    return saveSettings(path);
    */
    return true;
}

bool Workspace::saveSettings(const fs::path& path) {
    /*xml_document doc;
    xml_node     root = doc.append_child(m_appName.c_str());

    return doc.save_file(path.string().c_str());
    */
    return true;
}

}  // namespace ara
