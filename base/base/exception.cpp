#include "exception.h"
#include "logger.h"
#include <execinfo.h>
#include <cxxabi.h>
#include <vector>

namespace base
{
    using namespace std;

    Exception::Exception(const char* what)
        : what_(what)
    {
        dumpBacktrace();
    }

    Exception::Exception(const std::string& what)
        : what_(what)
    {
        dumpBacktrace();
    }

    Exception::~Exception() throw()
    {
    }

    const char* Exception::what() const throw()
    {
        return what_.c_str();
    }

    void Exception::dumpBacktrace()
    {
#define BACKTRACE_SIZE 100
        void* buffer[BACKTRACE_SIZE];
        LOG_DEBUG("--- throw exception, what=%s", what_.c_str());
        int len = backtrace(buffer, BACKTRACE_SIZE);
        char** strings = backtrace_symbols(buffer, len);
        vector<string> realnames;
        LOG_DEBUG("backtrace: \n");
        for (int i = 0; i < len; ++i) {
            const char* begin = strchr(strings[i], '(');
            const char* end = strchr(strings[i], '+');
            char* realname = NULL;
            if (begin != NULL && end != NULL) {
                int status = 0;
                std::string s(begin + 1, end - begin - 1);
                realname = abi::__cxa_demangle(s.c_str(), 0, 0, &status);
            }
            LOG_DEBUG("#%d %s\n", i, strings[i]);
            if (realname != NULL) {
                realnames.push_back(realname);
                free(realname);
            } else {
                realnames.push_back("");
            }
        }
        free(strings);
        LOG_DEBUG("---\n");
        for (size_t i = 0u; i < realnames.size(); ++i) {
            LOG_DEBUG("#%u %s", i, realnames[i].c_str());
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
