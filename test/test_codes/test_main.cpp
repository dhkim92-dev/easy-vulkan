#include <gtest/gtest.h>
#include "easy-vulkan.h"

int main(int argc, char **argv) {
    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);
    
    // Parse command line arguments for Google Test
    int result = RUN_ALL_TESTS();
    
    // Return the result of the tests
    return result;
}