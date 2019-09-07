#include <iostream>

/**
 Do nothing. This is useful for 'meta' compilation targets (such as ALL).
 In the future, this could become a CPU-only test suite.
 \return an error/success code
 \ingroup Tools
 */
int main(int, char **) {
	std::cout << "Successful execution." << std::endl;
	return 0;
}
