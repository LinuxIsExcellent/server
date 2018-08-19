#ifndef CLIENT_EXCEPTION_H
#define CLIENT_EXCEPTION_H

#include <exception>
#include <string>

namespace client
{
    class Exception : public std::exception
    {
    public:
        Exception() {}
        explicit Exception(const char* what);
        explicit Exception(const std::string& what);
        virtual ~Exception() throw();
        virtual const char* what() const throw();

    protected:
        void dumpBacktrace();

        std::string what_;
    };
}

#endif // CLIENT_EXCEPTION_H
