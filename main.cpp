#include <iostream>
#include "mysql.h"


int main() {
	Table table("test2|K1:int,K2:string|");
	table.Insert(1,std::string("efr"));
	table.Execute();
	table.Insert(2,std::string("efeed")).Execute();

	std::cout << "\nMAIN\n\n";

	for (Table::rows it = table.Select("K1 = 1"); it != table.SelectEnd(); ++it) {
		std::cout << "K1: " <<int(it["K1"])  << " K2: " << std::string(it["K2"]) << "\n";
//		       	<< " V2: " << std::string(it["V1"]) << " V2: " << std::string(it["V2"]) << "\n";
	}

	int max  = table.MaxValue("K1");

	std::cout << "max value " << max << " " << max + 1 <<  "\n";

	std::cout << "delete\n";
	table.Delete("K1 = 1");

	std::cout << "print:\n";
	for (Table::rows it = table.Select("K1 = 1"); it != table.SelectEnd(); ++it) {
		std::cout << "K1: " <<int(it["K1"])  << " K2: " << std::string(it["K2"])  << "\n";
//			<< " V2: " << std::string(it["V1"]) << " V2: " << std::string(it["V2"]) << "\n";
	}

	return 0;
}
