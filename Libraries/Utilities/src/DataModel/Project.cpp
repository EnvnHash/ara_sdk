#include <DataModel/Workspace.h>

using namespace std;
using namespace std::filesystem;
using namespace pugi;
namespace fs = std::filesystem;

namespace ara {

Project::Project() {
    name = "Empty Project";
    setTypeName<Project>();
}

void Project::init() {
    clearChildren();
    init_project_template();
    reassignIds();
}

void Project::saveAs(const fs::path& filename) {
    m_fileName = filename;
    save();
}

void Project::save() {
    reassignIds();
    saveState();
    checkWorkDir();
    //m_undoBufIt->save_file(m_fileName.string().c_str());
}

/** if a project file was loaded we have to actualize the defaultDirectory which
 * should point to a directory with the same name as the project, relative to
 * the .pd file */
std::string Project::checkWorkDir() {
    std::error_code ec;

    // get the base path from the filename, this corresponds to the working
    // directory
    auto workdirPath = parentDir();

    // create the workDir if necessary
    if (!fs::create_directory(workdirPath, ec))
        if (0 != ec.value())  // already exist
        {
            cerr << "could not create directory " << workdirPath << ". Reason: " << ec.message() << endl;
            throw std::system_error(ec);
        }

    // extract the projectName from the filename
    fs::path projName = fs::path(m_fileName).stem();
    m_projectPath     = workdirPath / projName;
    if (!fs::create_directory(m_projectPath, ec))
        if (0 != ec.value()) {  // already exist
            cerr << "could not create directory " << m_projectPath << ". Reason: " << ec.message() << endl;
            throw std::system_error(ec);
        }

    return m_projectPath.string();
}

path Project::parentDir() {
    auto filename = m_fileName.string();
#ifdef _WIN32
    std::replace(filename.begin(), filename.end(), '/',
                 '\\');  // be sure that it doesn't contain any wrong slashes
#endif
    auto parentPath = path(filename).parent_path();
    return path(filename).parent_path();
}

path Project::workDir() {
    auto parent = Project::parentDir();
    return parent / fs::path(name());
}

}  // namespace ara
