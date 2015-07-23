#ifndef MYSQL
#define MYSQL

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

#include <cppconn/prepared_statement.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <ctime>

#include "mysql_connection.h"
#include "gan-exception.h"

class StringType {
private:
	std::string value;
public:
	StringType();

	StringType(const std::string& value);

	StringType(const int value);

	operator int() const;

	operator std::string() const;

};


class Field {
private:
	std::string field_name;

public:

	Field(const std::string& field_name);

	virtual std::string GetSqlDef() const = 0;

	std::string GetFieldName() const;
};


class IntField : public Field {
public:
	IntField(const std::string& field_name);

	std::string GetSqlDef() const;

};


class StringField : public Field {
public:
	StringField(const std::string& field_name);

	std::string GetSqlDef() const;

};


class Table {
private:
	std::string table_name;
	std::vector<Field*> primary;
	std::vector<Field*> values;
	sql::Connection *con;
	std::vector<std::string> insert_query;
	std::vector<std::unordered_map<std::string, StringType> > select_query;
	std::time_t time_of_last_execute;

	class Rows {
	private:
		typename std::vector<std::unordered_map<std::string, StringType> >::const_iterator iterator;

	public:

		Rows(const std::vector<std::unordered_map<std::string, StringType> >::const_iterator& iterator);

		bool operator==(const Rows& other) const;

		bool operator!=(const Rows& other) const;

		Rows& operator++();

		StringType operator[](const std::string& field) const;
	};


public:

	typedef Rows rows;

	std::vector<std::string> Split(const std::string& string, const char separator) const;

	void AddFields(std::vector<Field*>* fields, const std::string& part, std::string* query);

	Table(const std::string& description);


	void CreateQuery(std::string* query) const;

	template
	<typename ...Args>
	Table& CreateQuery(std::string* query, const StringType& value, Args... args) {
		*query += "'" + std::string(value) + "',";
		CreateQuery(query, args...);
		return *this;
	}


	template
	<typename... Args>
	Table& Insert(Args... args) {
		std::string query = "(";
		CreateQuery(&query, args...);
		query[static_cast<int>(query.size()) - 1] = ')';

		insert_query.push_back(query);

		if (std::time(0) - time_of_last_execute > 10 * 60 || insert_query.size() > 10) {
			Execute();
		}
		return *this;
	}


	Table& Execute();

	Rows Select(const std::string& query_where);

	Rows SelectEnd() const;

	void Delete(const std::string& query_where) const;

	StringType MaxValue(const std::string& field_name) const;


};


#endif
