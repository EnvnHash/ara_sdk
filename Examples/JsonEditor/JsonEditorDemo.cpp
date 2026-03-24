//
// Created by user on 24.11.2020.
//

#include "JsonEditorDemo.h"
#include <UIElements/Text/Label.h>
#include <UIElements/DataBinding/JsonEditor.h>

using namespace std;
using namespace ara;
using namespace glm;

void JsonEditorDemo::init(std::function<void(UINode&)>) {
    UIApplication::init([](UINode& rootNode){
        rootNode.setBackgroundColor(vec4(.1f, .1f, .1f, 1.f));
        rootNode.setPadding(20.f);

        auto& label = rootNode.push<Label>({ .size = ivec2{400,60} });
        label.setFont("regular", 28, align::left, valign::top, vec4{1.f});
        label.setText("Json Editor Demo");

        auto& edit = rootNode.push<JsonEditor>(UINodePars{
            .pos = ivec2{ 0, 40 },
            .bgColor = vec4(.1f, .1f, .8f, 0.3f),
        });
        edit.setSize(1.f, -40);
        edit.loadFile("test.json");
    });
}
