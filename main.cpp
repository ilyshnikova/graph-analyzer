#include <iostream>
#include "mysql.h"


int main() {
	Table table("test|K1:int,K2:string|V1:int,V2:int");
	table.Insert(1,std::string("efr"),3,4);
	table.Execute();
	table.Insert(2,std::string("efeed"),2,1);
	table.Execute();

	std::cout << "\n\n\n\nMAIN\n\n";

	for (Table::rows it = table.Select("1"); it != table.SelectEnd(); ++it) {
		std::cout << int(it["K1"])  << " " << std::string(it["K2"]) << " " << std::string(it["V1"]) << " " << std::string(it["V2"]) << "\n";
	}

	return 0;
}
