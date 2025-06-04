#include "GLBaseUnitTestCommon.h"

using namespace std;

namespace ara::GLBaseUnitTest::CreateGLCtx {

TEST(GLBaseTest, CreateGLCtx) {
   // instantiate a new opengl context
   unique_ptr<GLFWWindow> ctx = GLBase::createOpenGLCtx(true);
   EXPECT_TRUE(ctx);

    // if this operation fails the unique m_ptr will be empty
   bool ptrEmpty = ctx ? false : true;

   ctx->destroy();

    // expect the unique_ptr not to be empty
   EXPECT_FALSE(ptrEmpty);
}

}