#include <iostream>

#include <nanogui/common.h>

int main(int argc, char const* argv[])
{
	nanogui::Vector3f v1(1);
	nanogui::Vector4f v2(0);
	store(v2.data(), v1);
	std::cout << v1 << std::endl;
	std::cout << v2 << std::endl;
	return 0;
}