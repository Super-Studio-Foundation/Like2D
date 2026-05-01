#include "Like2D.h"
#include <iostream>

int main(int argc, char* argv[]) {
    Like2D::Application app;
    
    if (!app.initialize(argc, argv)) {
        std::cerr << "Failed to initialize Like2D Application" << std::endl;
        return 1;
    }
    
    app.run();
    app.shutdown();
    
    return 0;
}
