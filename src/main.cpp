#include <GLFW/glfw3.h>
#include <iostream>

int main() {
    std::cout << "A-T: node active\n";

    if (!glfwInit()) {
        std::cerr << "GLFW init failed\n";
        return 1;
    }
    std::cout << "GLFW " << glfwGetVersionString() << '\n';
    glfwTerminate();
    return 0;
}
