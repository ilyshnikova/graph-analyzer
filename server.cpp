#include <iostream>
#include "daemons.h"


int main() {
	EchoDaemon daemon("127.0.0.1", "8081");
	return 0;
}
