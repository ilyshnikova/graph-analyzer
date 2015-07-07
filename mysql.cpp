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

std::string Field::GetFieldName() const {
	return field_name;
}


/*   IntField    */


IntField::IntField(const std::string& field_name)
: Field(field_name)
{}


std::string IntField::GetSqlDef() const {
 	return "int(11) not null default 0";
}

/*   StringField    */


StringField::StringField(const std::string& field_name)
: Field(field_name)
{}


std::string StringField::GetSqlDef() const {
 	return "varbinary(255) not null default ''";
}


/*  Table   */

Table::TableExceptions::TableExceptions(const std::string& reason)
: reason(reason)
{}


const char * Table::TableExceptions::what() const throw() {
	return reason.c_str();
}

Table::TableExceptions::~TableExceptions() throw()
{}

std::vector<std::string> Table::Split(const std::string& string, const char separator) const {
	std::vector<std::string> result;
	std::string substr;

	for (size_t i = 0; i < string.size(); ++i) {
		if (string[i] == separator) {
			result.push_back(substr);
			substr = "";
		} else {
			substr += string[i];
		}
	}

	result.push_back(substr);

	return result;
}


void Table::AddFields(std::vector<Field*>* fields, const std::string& part, std::string* query) {
	std::vector<std::string> separate_parts = Split(part, ',');

	for (size_t i = 0; i < separate_parts.size(); ++i) {
		std::vector<std::string> field = Split(separate_parts[i] ,':');
		if (field.size() != 2) {
			throw TableExceptions("field definition '"
				+ separate_parts[i]
				+ "' cannot be a field definition, there should be two words separatd by ':'"
			);
		}

		std::string field_name = field[0];
		std::string field_def = field[1];

		Field* new_field;

		if (field_def == "int") {
			new_field = new IntField(field_name);
		} else if (field_def == "string") {
			new_field = new StringField(field_name);
		} else {
			throw TableExceptions("Table is set incorrectly, incorrect field type " + field_def);
		}

		fields->push_back(new_field);

		(*query) += field_name
			+ " "
			+ new_field->GetSqlDef()
			+ ",\n";
	}

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

	std::vector<std::string> parts = Split(description, '|');

	if (parts.size() != 3) {
		throw TableExceptions(
			"String '"
			+ description
			+ "' is not a valid script description,because there should be three values, separated by '|'"
		);
	}

	table_name = parts[0];

	std::string query = "create table if not exists " + table_name + " (\n";

	AddFields(&primary, parts[1], &query);
	AddFields(&values, parts[2], &query);

	query += "primary key(";

	for (size_t i = 0; i < primary.size(); ++i) {
		query += primary[i]->GetFieldName();
		query += (i == primary.size() - 1 ? ")\n" : ", ");
	}

	query += ")";

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
	return iterator->at(field);
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
	sql::PreparedStatement *pstmt = con->prepareStatement(
		"select * from "
		+ table_name
		+ " where "
		+ query_where
	);
	sql::ResultSet* res = pstmt->executeQuery();

	select_query.clear();

	while (res->next()) {
		std::unordered_map<std::string, StringType> result;

		for (size_t i = 0; i < primary.size(); ++i) {
			result[primary[i]->GetFieldName()] =
				std::string(res->getString(primary[i]->GetFieldName()));
		}

		for (size_t i = 0; i < values.size(); ++i) {
			result[values[i]->GetFieldName()] =
				std::string(res->getString(values[i]->GetFieldName()));
		}
		select_query.push_back(result);
	}

	return Rows(select_query.cbegin());
}


