#include "writer.hpp"
#include <iostream>

int main() {
    Writer writer;
    
    std::cout << "Generating example.umdf..." << std::endl;
    
    if (writer.writeNewFile("example.umdf")) {
        std::cout << "Successfully generated example.umdf" << std::endl;
    } else {
        std::cerr << "Failed to generate example.umdf" << std::endl;
        return 1;
    }
    
    return 0;
} 