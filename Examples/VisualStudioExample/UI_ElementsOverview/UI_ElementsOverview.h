#pragma once

#include <UIApplication.h>

namespace ara {

class UI_ElementsOverview : public UIApplication {
public:
    void init(std::function<void()> initCb);

private:
    void addMainWindow();
    void addDiv(UINode* root, int heightStep);
    void addLabel(UINode* root, int heightStep);
    void addButton(UINode* root, int heightStep);
    void addSlider(UINode* root, int heightStep);
    void addPropSlider(UINode* root, int heightStep);
    void addComboBox(UINode* root, int heightStep);
    void addImage(UINode* root, int heightStep);
    void addImageButton(UINode* root, int heightStep);
    void addSpinner(UINode* root, int heightStep);
    void addEditor(UINode* root, int heightStep);
    void addInfoDialogButton(UINode* root, int heightStep);
    void addTable(UINode* root);
    void addTabView(UINode* root);
};

}
