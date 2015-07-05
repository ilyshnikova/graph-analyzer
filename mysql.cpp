#include "mysql.h"
#include "mysql_connection.h"

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

#include <cppconn/prepared_statement.h>
#include <string>
#include <vector>
#include <unordered_map>

/*   StringType    */

StringType::StringType()
: value()
{}


StringType::StringType(const std::string& value)
: value(value)
{}


StringType::StringType(const int value)
: value(std::to_string(value))
{}


StringType::operator int() const {
	return std::stoi(value);
}

StringType::operator std::string() const {
	return value;
}


/*   Field    */


Field::Field(const std::string& field_name)
: field_name(field_name)
{}


/*   IntField    */


IntField::IntField(const std::string& field_name)
: Field(field_name)
, field_name(field_name)
{}


std::string IntField::GetSqlDef() const {
 	return "int(11) not null default 0";
}

std::string IntField::GetFieldName() const {
	return field_name;
}

/*   StringField    */


StringField::StringField(const std::string& field_name)
: Field(field_name)
, field_name(field_name)
{}


std::string StringField::GetSqlDef() const {
 	return "varbinary(255) not null default ''";
}


std::string StringField::GetFieldName() const {
	return field_name;
}


/*  Table   */


std::pair<std::string, std::string> Table::Split(const std::string& description, size_t index) const {
	std::string field_name;
	while (description[index] != ':' && description[index] != '|' && index < description.size()) {
		field_name.push_back(description[index++]);
	}

	++index;
	std::string field_type;

	while (description[index] != ',' && index < description.size() && description[index] != '|') {
		field_type.push_back(description[index++]);
	}

	return std::make_pair(field_name, field_type);

}

size_t Table::AddField(std::vector<Field*>* fields, const std::string& description, size_t index) {
	std::pair<std::string, std::string> field  = Split(description, index);
	std::cout << field.first << " " << field.second << "\n";
	if (field.second == "int") {
		fields->push_back(new IntField(field.first));
	} else if (field.second == "string") {
		fields->push_back(new StringField(field.first));
	}
	index += field.first.size() + field.second.size() + 1;
	return description[index] == '|' ? index : index + 1;

}

Table::Table(const std::string& description)
: table_name()
, primary()
, values()
, con()
, insert_query()
, select_query()
{

	sql::Driver *driver;
	driver = get_driver_instance();
	con = driver->connect("162.216.17.226", "root", "rootfps123987");

	con->setSchema("gandb");


	for (size_t i = 0; i < description.size() && description[i] != '|'; ++i) {
		table_name.push_back(description[i]);
	}


	std::string query = "create table if not exists " + table_name + " (\n";

	std::cout << "PRIMARY\n";

	size_t index = table_name.size() + 1;
	while (description[index] != '|') {
		index = AddField(&primary, description, index);

		Field* field = primary[primary.size() - 1];
		query += field->GetFieldName() + " " + field->GetSqlDef() + ",\n";
	}
	index++;

	std::cout << "VALUE\n";
	while(index < description.size()) {
		index = AddField(&values, description, index);

		Field* field = values[values.size() - 1];
		query += field->GetFieldName() + " " + field->GetSqlDef() + ",\n";
	}


	query += "primary key(";

	for (size_t i = 0; i < primary.size(); ++i) {
		query += primary[i]->GetFieldName();
		query += (i == primary.size() - 1 ? ")\n" : ", ");
	}

	query += ");";

	std::cout  << query << "\n";

	sql::Statement* stmt = con->createStatement();
	stmt->execute(query);

}


Table::Rows::Rows(const std::vector<std::unordered_map<std::string, StringType> >::const_iterator& iterator)
: iterator(iterator)
{}

bool Table::Rows::operator==(const Rows& other) const {
	return iterator == other.iterator;
}

bool Table::Rows::operator!=(const Rows& other) const {
	return iterator != other.iterator;
}

Table::Rows& Table::Rows::operator++() {
	++iterator;
	return *this;
}

StringType Table::Rows::operator[](const std::string& field) const {
	return (*iterator).at(field);
}



void Table::CreateQuery(std::string* query) const {}


Table& Table::Execute() {
	std::string query = "replace into " + table_name + " values ";
	for (size_t i = 0; i < insert_query.size(); ++i) {
		query += insert_query[i] + (i + 1 == insert_query.size() ? "" : ", ");
	}

	insert_query.clear();

	sql::Statement* stmt = con->createStatement();
	stmt->execute(query);
}


Table::Rows Table::SelectEnd() const {
	return Table::Rows(select_query.cend());
}



Table::Rows Table::Select(const std::string& query_where) {
	sql::PreparedStatement *pstmt = con->prepareStatement("select * from " + table_name + " where " + query_where);
	sql::ResultSet* res = pstmt->executeQuery();

	select_query.clear();

	while (res->next()) {
		select_query.push_back(std::unordered_map<std::string, StringType>());
		size_t index = select_query.size() - 1;
		for (size_t i = 0; i < primary.size(); ++i) {
			select_query[index][primary[i]->GetFieldName()] = std::string(res->getString(primary[i]->GetFieldName()));
		}

		for (size_t i = 0; i < values.size(); ++i) {
			select_query[index][values[i]->GetFieldName()] = std::string(res->getString(values[i]->GetFieldName()));
		}
	}

	return Rows(select_query.cbegin());
}


