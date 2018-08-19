#ifndef BASE_DBO_DBTEMPLATE_H
#define BASE_DBO_DBTEMPLATE_H

#include "connection.h"
#include "../command/runner.h"

namespace base
{
    namespace dbo
    {
        class DBTemplate
        {
        public:
            static void Execute(command::Runner& runner, int poolId, const char* sql, std::function<void(ResultSet&)> resultHandler);
            static void ExecutePrepare(command::Runner& runner, int poolId, const char* sql, std::function<void(PreparedStatement&)> prepareHandler, std::function<void(ResultSet&)> resultHandler);
        };
    }
}

#endif // DBTEMPLATE_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
