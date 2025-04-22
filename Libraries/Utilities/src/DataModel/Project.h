#pragma once

#include <DataModel/Item.h>
#include <util_common.h>

#include <utility>

// obsolete. to be changed to Node derivate

namespace ara {

class RecentProject {
public:
    std::time_t           mod_time;
    std::filesystem::path path;
    std::string           name;
};

class Project : public ItemUi {
public:
    Project();
    ~Project() override = default;

    void saveAs(const std::filesystem::path& fileName) override;
    void save() override;

    virtual std::string checkWorkDir();
    virtual std::filesystem::path parentDir();
    virtual std::filesystem::path workDir();
    virtual void init();
    virtual void init_project_template() {}

    void reassignIds() {
        m_idCtr = 0;
        Project::reassignIdsIt(&m_idCtr, this, &m_idItemMap);
    }

    static void reassignIdsIt(int *id, ItemUi *item, std::unordered_map<int, ItemUi *> *map) {
        item->id    = *id;
        (*map)[*id] = item;
        ++(*id);

        for (auto &it : reinterpret_cast<std::list<std::unique_ptr<ItemUi>> &>(item->children())) {
            Project::reassignIdsIt(id, it.get(), map);
        }
    }

    std::string            workDirString() { return workDir().string(); }
    void                   setFileName(std::filesystem::path fn) { m_fileName = std::move(fn); }
    std::filesystem::path &getFileName() { return m_fileName; }

    std::filesystem::path &getProjectPath() {
        if (m_projectPath.empty()) checkWorkDir();
        return m_projectPath;
    }

    int                              getIdCtr() const { return m_idCtr; }
    void                             incrIdCtr() { m_idCtr++; }
    std::unordered_map<int, ItemUi*> *getIdItemMap() { return &m_idItemMap; }

protected:
    std::filesystem::path               m_fileName;
    int                                 m_idCtr;
    std::unordered_map<int, ItemUi*>    m_idItemMap;
    std::filesystem::path               m_projectPath;
};

}  // namespace ara
