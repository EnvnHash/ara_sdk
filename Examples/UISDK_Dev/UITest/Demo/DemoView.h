#pragma once

#include "UI/Button/ImageButton.h"
#include <UI/Label.h>
#include <UI/UITable.h>
#include <UI/ScrollView.h>
#include <UI/List.h>
#include <UI/ComboBox.h>
#include <UI/Dialoges/FloatingMenuDialog.h>
#include "../../../../Libraries/GLBase/src/Res/ResFont.h"
#include "../../../../Libraries/GLBase/src/Res/ResGlFont.h"

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

        void                init() override;
        ScrollView*	ui_SV=nullptr;
    };

    class DemoView_ScrollView_3 : public DemoView {
    public:
        class Unit : public Div {
        public:
            Unit(): Div() {}

            void            init() override;
            void            setColor(glm::vec4& col) { m_fontColor = col; }
            void            setColor(float r, float g, float b, float a) { m_fontColor.r = r; m_fontColor.g = g; m_fontColor.b = b; m_fontColor.a = a; }

            std::string		m_Title;
            glm::vec4		m_fontColor;
        };

        DemoView_ScrollView_3();

        void                init() override;
        ScrollView*	ui_SV=nullptr;
    };

    class DemoView_ScrollViewList : public DemoView {
    public:
        DemoView_ScrollViewList();

        virtual void                init();
        ScrollView*			ui_SV=nullptr;
        List<std::string>*    m_list=nullptr;
        std::list<std::string>       m_data;
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
        bool                draw(uint32_t* objId) override;

    private:
        Image*              m_img=nullptr;
        FontList   	fontList;
        Shaders*       m_glyphShader=nullptr;
    };

    class DemoView_ContentTrans : public DemoView {
    public:

        DemoView_ContentTrans();

        void                init() override;
        bool                draw(uint32_t* objId) override;
        bool                drawIndirect(uint32_t *objId) override;

    private:
        Div*                  m_viewCtr=nullptr;
        bool                        m_updt=true;
    };

    class DemoView_Edit : public DemoView {
    public:
        DemoView_Edit();

        void                init() override;
        bool				draw(uint32_t* objId) override;

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
