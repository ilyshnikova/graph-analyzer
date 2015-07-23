#include "gan-exception.h"

GANException::GANException(const int id, const std::string& information)
: std::exception()
, id(id)
, information(information)
{}

const char * GANException::what() const throw() {
	return ("ERROR " + std::to_string(id) + " : " + information).c_str();
}

GANException::~GANException() throw() {}
