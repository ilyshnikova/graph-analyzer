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
#include "base64.h"
#include "logger.h"



std::vector<std::string> Split(const std::string& string, const char separator);


class StringType {
private:
	std::string value;
public:
	StringType();

	std::string GetValue() const;

	StringType(const std::string& value);

	StringType(const int value);

	StringType(const double value);

	StringType(const std::time_t value);

	template
	<typename T>
	StringType(const std::vector<T>& vector)
	:value()
	{
		for (size_t i = 0; i < vector.size(); ++i) {
			std::string element = std::string(StringType(vector[i]));

			value +=
				base64_encode(
					reinterpret_cast<const unsigned char*>(element.c_str()),
					element.size()
				)
				+ (i + 1 == vector.size() ? "" : ",");
		}

	}

	template
	<typename K, typename V>
	StringType(const std::unordered_map<K,V>& map)
	:value()
	{
		std::vector<std::string> elements(map.size());
		size_t i = 0;
		for (auto it = map.cbegin(); it != map.cend(); ++it) {
			std::string key = std::string(StringType(it->first));
			std::string val = std::string(StringType(it->second));
			elements[i++] =
				base64_encode(
					reinterpret_cast<const unsigned char*>(key.c_str()),
					key.size()
				)
				+ std::string(":")
				+ base64_encode(
					reinterpret_cast<const unsigned char*>(val.c_str()),
					val.size()
				);
		}

		for (size_t i = 0; i < elements.size(); ++i) {
			value +=
				elements[i]
				+ (i + 1 == elements.size() ? "" : ",");
		}

	}


//	operator int() const;

	operator std::string() const;

	operator double() const;

	template
	<typename... Args, typename T>
	void ToString(std::vector<std::string>* parts, const T element, Args... args) {
		parts->push_back(std::string(StringType(element)));
		ToString(parts, args...);
	}

	std::string ToString(std::vector<std::string>* parts);

	template
	<typename... Args>
	StringType(Args... args)
	{
		std::vector<std::string> parts;
		ToString(&parts, args...);

		for (size_t i = 0; i < parts.size(); ++i) {
			value +=
				base64_encode(
					reinterpret_cast<const unsigned char*>(parts[i].c_str()),
					parts[i].size()
				)
				+ (i + 1 == parts.size() ? "" : ",");
		}
	}


	template
	<typename... Args>
	void FromString(Args... args) {
		CreateObjects(Split(value, ','), 0, args...);
	}


	template
	<typename... Args, typename T>
	void CreateObjects(const std::vector<std::string>& parts, const size_t index, T* element, Args... args) const  {

		CreateObject(parts[index], element);
		CreateObjects(parts, index + 1, args...);
	}


	void CreateObject(const std::string& st, int* element) const;

	void CreateObject(const std::string& st, double* element) const;

	void CreateObject(const std::string& st, std::string* element) const;

	void CreateObject(const std::string& st, std::time_t* element) const;


	template
	<typename T>
	void CreateObject(
		const std::string& st,
		std::vector<T>* element
	) const {
		logger << "vector " + st;
		std::vector<std::string> vectors_parts = Split(base64_decode(st), ',');
		logger << std::to_string(vectors_parts.size()) + " size";
		std::vector<T> vector(vectors_parts.size());
		for (size_t i = 0; i < vectors_parts.size(); ++i) {

			CreateObject(
				vectors_parts[i],
				&vector[i]
			);
		}
		*element = vector;
	}


	template
	<typename K, typename V>
	void CreateObject(
		const std::string& st,
		std::unordered_map<K,V>* element
	) const {
		std::vector<std::string> maps_parts = Split(base64_decode(st), ',');
		std::unordered_map<K,V> map;
		for (size_t i = 0 ; i < maps_parts.size(); ++i) {
			std::vector<std::string> key_and_element = Split(maps_parts[i], ':');
			K key;
			V val;
			CreateObject(key_and_element[0], &key);
			CreateObject(key_and_element[1], &val);
			map[key] = val;
		}

		*element = map;
	}



	void CreateObjects(const std::vector<std::string>& parts, const size_t index) const;


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
	std::time_t timeout;
	int line_count;

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
		if (std::time(0) - time_of_last_execute > timeout || static_cast<int>(insert_query.size()) > line_count) {
			Execute();
		}
		return *this;
	}


	Table& Execute();

	Rows Select(const std::string& query_where);

	Rows SelectEnd() const;

	void Delete(const std::string& query_where) const;

	StringType MaxValue(const std::string& field_name) const;

	void ChangeTimeout(const int new_timeout);

	void ChangeLineCount(const int new_line_count);


};


#endif
