#include <gtest/gtest.h>
#include "easy-vulkan.h"
#include "test_common.h"

int main(int argc, char **argv) {
    // Initialize Google Test
    init_executable_path(argv[0]); // Initialize the executable path
    ::testing::InitGoogleTest(&argc, argv);
    
    // Parse command line arguments for Google Test
    int result = RUN_ALL_TESTS();
    
    // Return the result of the tests
    return result;
}