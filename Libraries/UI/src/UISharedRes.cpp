//
// Created by sven on 11-06-25.
//

#include <UISharedRes.h>
#include <WindowManagement/GLWindow.h>

namespace ara {

void UISharedRes::reqRedraw() {
    requestRedraw = true;
    winHandle->iterate();
}

}