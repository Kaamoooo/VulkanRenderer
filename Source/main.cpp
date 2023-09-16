#include "../Headers/Application.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

int main() {
	Kaamoo::Application app{};

	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
