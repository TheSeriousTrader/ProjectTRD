
#include <iostream>

int main(int, char*[])
{
    std::cout << “cout shouldn’t be used for logging” << std::endl;
    std::cin.get();
    return 0;
}