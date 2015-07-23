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


	std::string a;
	table.Delete("K1 = 1 or K1 = 3 or K1 = 2 or K1 = 9 or K1 = 4");

	std::cin >> a;
	table.ChangeTimeout(2);
	table.Insert(7, std::string("awev"));
	std::cin >> a;
	table.Insert(3, std::string("edvfdv"));

	table.ChangeLineCount(2);
	table.Insert(4, std::string("edvsadfdv"));
	table.Insert(1, std::string("edavcvfdv"));
	table.Insert(9, std::string("edacfvfdv"));

	return 0;
}
