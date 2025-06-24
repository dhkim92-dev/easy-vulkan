# easy-vulkan-project

## Overview
The **easy-vulkan-project** is a C++ library designed to provide a simplified interface for working with Vulkan, the modern graphics and compute API. This project aims to make it easier for developers to utilize Vulkan's capabilities without dealing with the complexities of the API directly.

## Project Structure
```
easy-vulkan-project
├── CMakeLists.txt          # Main CMake configuration file
├── easy-vulkan             # Subproject for the easy-vulkan library
│   ├── CMakeLists.txt      # CMake configuration for easy-vulkan
│   ├── include             # Header files for easy-vulkan
│   │   └── easy-vulkan
│   │       └── easy_vulkan.h  # Header file for easy-vulkan library
│   └── src                 # Source files for easy-vulkan
│       └── easy_vulkan.cpp  # Implementation of easy-vulkan library
├── test                    # Subproject for testing the easy-vulkan library
│   ├── CMakeLists.txt      # CMake configuration for tests
│   └── test_main.cpp       # Main file for running tests
└── README.md               # Project documentation
```

## Requirements
- CMake version 3.10 or higher
- Vulkan SDK installed on your system

## Building the Project
1. Clone the repository:
   ```bash
   git clone <repository-url>
   cd easy-vulkan-project
   ```

2. Create a build directory:
   ```bash
   mkdir build
   cd build
   ```

3. Configure the project using CMake:
   ```bash
   cmake ..
   ```

4. Build the project:
   ```bash
   cmake --build .
   ```

## Usage
After building the project, you can link against the `easy-vulkan` library in your own applications. Include the header file in your source code:
```cpp
#include <easy-vulkan/easy_vulkan.h>
```

## Running Tests
To run the tests for the `easy-vulkan` library, navigate to the `test` directory and execute:
```bash
cd test
./test_executable
```

## Contributing
Contributions are welcome! Please open an issue or submit a pull request for any improvements or bug fixes.

## License
This project is licensed under the MIT License. See the LICENSE file for more details.# Easy Vulkan
