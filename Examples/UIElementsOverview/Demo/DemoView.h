#pragma once

#include <Asset/AssetFont.h>
#include <Asset/FontList.h>
#include <Dialoges/FloatingMenuDialog.h>
#include <UIElements/Button/ImageButton.h>
#include <UIElements/DataBinding/List.h>
#include <UIElements/Menu/ComboBox.h>
#include <UIElements/ScrollView.h>
#include <UIElements/Text/Label.h>
#include <UIElements/UITable.h>

#include "Transitions/Carrousel.h"

namespace ara {

class DemoView : public Div {
public:
    DemoView(const std::string &title, glm::vec4 bk_color);
private:
    Label*                      m_label=nullptr;
    glm::vec4                   m_textcolor=glm::vec4{1.f};
};

class DemoView_Spinner : public DemoView {
public:
    DemoView_Spinner();
    void                init() override;
};

class DemoView_Table : public DemoView {
public:
    DemoView_Table();
    void                init() override;
    UITable*			ui_Table=nullptr;
    glm::vec4           m_textColor{1.f};
};

class DemoView_Table_2 : public DemoView {
public:
    DemoView_Table_2();
    void                init() override;
    UITable*		ui_Table=nullptr;
};

class DemoView_ScrollView : public DemoView {
public:
    DemoView_ScrollView();
    void                init() override;
    ScrollView*	ui_SV=nullptr;
};

class DemoView_ScrollView_2 : public DemoView {
public:
    DemoView_ScrollView_2();

    UITable&    addTable();
    UITable&    addNestedTable(ScrollView* node);
    void        addLabels(UITable& nt);
    void        init() override;
};

class DemoView_ScrollView_3 : public DemoView {
public:
    class Unit : public Div {
    public:
        Unit(): Div() {}
        void            init() override;
        std::string		m_Title;
    };

    DemoView_ScrollView_3();

    void                init() override;
    ScrollView*	ui_SV=nullptr;
};

class DemoView_ScrollViewList : public DemoView {
public:
    DemoView_ScrollViewList();
    virtual void                    init();
    ScrollView*			            ui_SV=nullptr;
    List<std::list<std::string>>*   m_list=nullptr;
    ListProperty<std::string>       m_data3;
};

class DemoView_Collapsibles : public DemoView {
public:
    DemoView_Collapsibles();
    void                init() override;
private:
    ComboBox*           m_combo = nullptr;
    Node                m_node;
};

class DemoView_Carrousel : public DemoView {
public:
    DemoView_Carrousel();
    void                init() override;
    void                addCarrousel(CarrouselMode cm, int yOffs);
};

class DemoView_Resources : public DemoView {
public:
    DemoView_Resources();
    void                init() override;

private:
    Image*              m_img=nullptr;
    FontList   	        fontList;
    Shaders*            m_glyphShader=nullptr;
};

class DemoView_ZoomView : public DemoView {
public:
    DemoView_ZoomView();
    void                init() override;

private:
};

class DemoView_Edit : public DemoView {
public:
    DemoView_Edit();
    void                init() override;

    TypoGlyphMap*	tfont=nullptr;
private:
    Shaders* m_glyphShader=nullptr;
};

#ifndef __ANDROID__
class DemoView_FloatingMenu : public DemoView {
public:
    DemoView_FloatingMenu();
    virtual void                init();
private:
};
#endif
}
