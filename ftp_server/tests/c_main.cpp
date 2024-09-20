#include <gtest/gtest.h>

extern std::map<std::string, std::string> ps_map;
// 自定义的 main 函数
int main(int argc, char **argv) {
    // 初始化 Google Test
    ::testing::InitGoogleTest(&argc, argv);
    ps_map["admin"] = "admin"; 
    // 如果需要，可以在这里进行一些全局初始化操作

    // 运行所有测试
    return RUN_ALL_TESTS();
}
