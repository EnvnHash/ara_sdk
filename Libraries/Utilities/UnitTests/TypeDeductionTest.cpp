//
// Created by sven on 02-04-26.
//

#include <UtilityUnitTestCommon.h>

namespace ara {

TEST(Functional_TypeDeduction, Basic) {
    EXPECT_EQ(tpi::tp_int32, getTpi<int32_t>());
    EXPECT_EQ(tpi::tp_float, getTpi<float>());
    EXPECT_EQ(tpi::tp_string, getTpi<std::string>());
    EXPECT_EQ(tpi::tp_vector_int32, getTpi< std::vector<int32_t> >());
    EXPECT_EQ(tpi::tp_vector_float, getTpi< std::vector<float> >());
    EXPECT_EQ(tpi::tp_vector_string, getTpi< std::vector<std::string> >());
}

}
