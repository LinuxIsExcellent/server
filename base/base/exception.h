#ifndef BASE_EXCEPTION_H
#define BASE_EXCEPTION_H

#include <exception>
#include <string>

namespace base
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

#endif // EXCEPTION_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
