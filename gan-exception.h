#ifndef EXCEPTION
#define EXCEPTION

#include <exception>
#include <string>

class GANException : public std::exception {
private:
	int id;
	std::string information;
public:
	GANException(const int id, const std::string& information);

	const char * what() const throw();

	~GANException() throw();
};

#endif
