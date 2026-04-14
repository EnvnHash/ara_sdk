//
// Created by sven on 13-04-26.
//

#include "DemoView.h"
#include "UIElements/DataBinding/NodeEdit.h"
#include "UIElements/DataBinding/PropSlider.h"

using namespace ara;
using namespace glm;
using namespace std;

DemoView_DataBinding::DemoView_DataBinding() : DemoView("Data Binding",vec4(.15f,.15f,.15f,1.f)) {
    setName(getTypeName<DemoView_DataBinding>());
    UINodeStyle::addStyleClass("demos.dataBinding");
    setScissorChildren(true);
}

void DemoView_DataBinding::init() {
    setupNodeEdit();
    setupPropertyEdit();
}

void DemoView_DataBinding::setupNodeEdit() {
    push<Label>({ .style = "demos.dataBinding.nodeEditHeadl" });

    auto& ne = push<NodeEdit>({
        .style = "demos.dataBinding.nodeEdit"
    });
    ne.setLineHeight(22);
    ne.setSpacing({10, 10});
    ne.setLabelWidth(100);
    ne.setOptPerKey(unordered_map<string, VariableEditOption<>>{ {
        "vectorFloat",
        VariableEditOption{
            arrange::vertical,
            0.f,
            1.f,
            0.1f
        }}
    });
    ne.setNode(m_node);

    m_node.setOnChangeCb(cbType::postChange, this, [this](Node*, const string&) {
        LOG << std::endl;
        LOG << "bool var: " << m_node.m_boolVal;
        LOG << "float var: " << m_node.m_floatVal;
        LOG << "int var: " << m_node.m_intVal;
        LOG << "string var: " << m_node.m_stringVal;
        LOG << "path var: " << m_node.m_pathVal;
        LOG << "float vector: " << m_node.m_vectorFloat[0] << ", " << m_node.m_vectorFloat[1] << ", " << m_node.m_vectorFloat[2];
        LOG << "int vector: " << m_node.m_vectorInt[0] << ", " << m_node.m_vectorInt[1] << ", " << m_node.m_vectorInt[2];
        LOG << "glm::vec4: " << glm::to_string(m_node.m_vec4);
        LOG << "glm::ivec4: " << glm::to_string(m_node.m_ivec4);
    });
}

void DemoView_DataBinding::setupPropertyEdit() {
    push<Label>({ .style = "demos.dataBinding.propHeadl" });

    m_prop = 0.5f;

    // listen for changes. when listening like this, the listener must be unregistered before his destructor is called
    m_prop.onPostChange([this] {
        LOG << "1st listener: value changed to " << m_prop();
    }, this); // in order to have multiple onChanged listeners, the listener needs to be identified somehow. the sdk uses void* ptr, so e.g., we can simply use this

    m_prop = 1.f; // change the properties value again, onchanged should fire

    // here goes an alternative way of registering an onchange listener. This one unregisters itself automatically
    // thus is safer to use, although the syntax is a bit more complicated
    onChanged<float>(m_prop, [](const std::any &val) {
        LOG << "2nd listener: value changed " << any_cast<float>(val);      // note: the value store inside the std::any is the same as the template class
    });

    //------------------------------------------------------------------------------------

    // add a slider
    auto& slider = push<PropSlider>({
        .style = "demos.dataBinding.propSlider"
    });
    slider.setLabel("Slider");
    slider.setProp(m_prop);
}