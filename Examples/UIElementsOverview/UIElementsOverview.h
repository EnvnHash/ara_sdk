#pragma once

#include <UIApplication.h>

namespace ara {

class UIElementsOverview : public UIApplication {
public:
    void init(const std::function<void()>& initCb);

private:
    Property<float> floatProp = { 0.5f, 0.f, 1.f, 0.01f };

    void addMainWindow();
    static void addDiv(UINode& root, int heightStep);
    static void addLabel(UINode& root, int heightStep);
    void addButton(UINode& root, int heightStep) const;
    static void addSlider(UINode& root, int heightStep);
    void addPropSlider(UINode& root, int heightStep);
    void addComboBox(UINode& root, int heightStep) const;
    static void addImage(UINode& root, int heightStep);
    static void addImageButton(UINode& root, int heightStep);
    static void addSpinner(UINode& root, int heightStep);

    static void addEditor(UINode& root, int heightStep);
    void addInfoDialogButton(UINode& root, int heightStep);
    static void addTable(UINode& root);
    static void addTabView(UINode& root);
};

}
