#ifndef CALLBACK
#define CALLBACK
#include "mysql_connection.h"

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

#include <cppconn/prepared_statement.h>
#include <string>
#include <vector>
#include <unordered_map>

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

	virtual std::string GetFieldName() const = 0;
};



class IntField : public Field {
private:
	std::string field_name;
public:
	IntField(const std::string& field_name);

	std::string GetSqlDef() const;

	std::string GetFieldName() const;


};


class StringField : public Field {
private:
	std::string field_name;

public:
	StringField(const std::string& field_name);

	std::string GetSqlDef() const;

	std::string GetFieldName() const;
};

class Table {
private:
	std::string table_name;
	std::vector<Field*> primary;
	std::vector<Field*> values;
	sql::Connection *con;
	std::vector<std::string> insert_query;
	std::vector<std::unordered_map<std::string, StringType> > select_query;

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

	std::pair<std::string, std::string> Split(const std::string& description, size_t index) const;

	size_t AddField(std::vector<Field*>* fields, const std::string& description, size_t index);

	Table(const std::string& description);

	void CreateQuery(std::string* query) const;

	template
	<typename ...Args>
	Table& CreateQuery(std::string* query, const StringType& value, Args... args) {
		std::cout << std::string(value) << "\n";
		*query += "'" + std::string(value) + "',";
		CreateQuery(query, args...);
		return *this;
	}


	template
	<typename... Args>
	void Insert(Args... args) {
		std::string query = "(";
		CreateQuery(&query, args...);
		query[query.size() - 1] = ')';

		std::cout << query << "<- query\n";

		insert_query.push_back(query);
	}


	Table& Execute();

	Rows Select(const std::string& query_where);

	Rows SelectEnd() const;

};


#endif
