#include <thread>

#include <iostream>

void helloFunction() {
    std::cout << "Hello from a thread!" << std::endl;
}

int main() {
    std::thread t(helloFunction);
    t.join();
    return 0;
}
