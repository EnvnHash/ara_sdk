#pragma once

#include <UIElements/Button/ImageButton.h>
#include <UIElements/Label.h>
#include <UIElements/UITable.h>
#include <UIElements/ScrollView.h>
#include <UIElements/List.h>
#include <UIElements/ComboBox.h>
#include <Dialoges/FloatingMenuDialog.h>
#include <Asset/AssetFont.h>
#include <Asset/ResGlFont.h>

namespace ara {

class DemoView : public Div {
public:
    DemoView(std::string title, glm::vec4 bk_color);
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

    UITable*    addTable();
    UITable*    addNestedTable(ScrollView* node);
    void        addLabels(UITable* nt);
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
    virtual void            init();
    ScrollView*			    ui_SV=nullptr;
    List<std::string>*      m_list=nullptr;
    std::list<std::string>  m_data;
};

class DemoView_ComboBox : public DemoView {
public:
    DemoView_ComboBox();
    void                init() override;
private:
    ComboBox*           m_combo = nullptr;
};

class DemoView_Resources : public DemoView {
public:
    DemoView_Resources();
    void                init() override;
    bool                draw(uint32_t& objId) override;
    void                drawColors(ResNode* node, int& it);

private:
    Image*              m_img=nullptr;
    FontList   	fontList;
    Shaders*       m_glyphShader=nullptr;
};

class DemoView_Edit : public DemoView {
public:
    DemoView_Edit();
    void                init() override;

    TypoGlyphMap*	tfont=nullptr;
private:
    Shaders*       m_glyphShader=nullptr;
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
