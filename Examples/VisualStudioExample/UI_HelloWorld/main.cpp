#include <UIApplication.h>

using namespace ara;
using namespace glm;

int main(int, char**) {
    UIApplication app;

    app.init([](UINode& rootNode){
        // add a child to the root node, let's take the simplest element, a Div which is just a box
        rootNode.push<Div>({
            .size = ivec2{400,400},
            .bgColor = vec4{1.f, 0.f, 0.f, 1.f},
            .align = align::center,
            .valign = valign::center
        });
    });

    app.startEventLoop(); // blocking

    return 0;
}

