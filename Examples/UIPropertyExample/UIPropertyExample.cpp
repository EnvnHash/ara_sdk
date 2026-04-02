
#include "UIPropertyExample.h"
#include "UIElements/DataBinding/PropSlider.h"

using namespace std;
using namespace ara;
using namespace glm;

void UIPropertyExample::init(const std::function<void()>&) {
    UIApplication::init([this](UINode& root) {

        //--- basic usage -------------------------------------------------------------------

        prop = 0.5f;            // set the property to a value
        LOG << prop();          // read a value

        // listen for changes. when listening like this the listener must be unregistered before his destructor is called
        prop.onPostChange([this] {
            LOG << "1st listener: value changed to " << prop();
        }, this); // in order to have multiple onChanged listeners, the listener needs to be identified somehow. the sdk uses void* ptr, so e.g. we can simply use this

        prop = 1.f;             // change the properties value again, onchanged should fire

        // here goes an alternative way of registering an onchange listener. This one unregisters itself automatically
        // thus is safer to use, although the syntax is a bit more complicated
        onChanged<float>(&prop, [](const std::any &val) {
            LOG << "2nd listener: value changed " << any_cast<float>(val);      // note: the value store inside the std::any is the same as the template class
        });

        prop = 2.f;             // change the properties value again, both listeners should fire

        // properties have min and max values, which are needed in order to use them e.g with a slider
        prop.setMinMax(-1.f, 2.f);

        //------------------------------------------------------------------------------------

        // add a slider
        auto& slider = root.push<PropSlider>({
            .pos = ivec2{ 100, 100 },
            .size = ivec2{ 300, 30 }
        });
        slider.setLabel("Slider");
        slider.setProp(prop);

        prop.removeOnPostChange(this);
    });
}
