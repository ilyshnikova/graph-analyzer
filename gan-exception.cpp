#include "gan-exception.h"

GANException::GANException(const int id, const std::string& information)
: std::exception()
, id(id)
, information(std::string("ERROR ") + std::to_string(id) + std::string(" : ") + information)
{}

const char * GANException::what() const throw() {
	return information.c_str();
}

GANException::~GANException() throw() {}
