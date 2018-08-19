#ifndef BASE_DBO_DBPOOL_H
#define BASE_DBO_DBPOOL_H

#include "preparedstatement.h"
#include "connection.h"
#include "../observer.h"
#include "../timer.h"
#include <unordered_map>

namespace base
{
    namespace memory
    {
        class MemoryPool;
    }

    namespace dbo
    {
        // 统计信息
        struct StatisticInfo {
            StatisticInfo(const internal::ConnectionPoolInfo& c)
                : conf(c) {
                Reset();
            }

            void Reset() {
                total = 0;
                free = 0;
                busy = 0;
                closed = 0;
            }

            // 活动中的连接数
            int ActiveCount() const {
                return total - closed;
            }

            // 是否为空
            bool IsEmpty() const {
                return total == closed;
            }

            // 配置
            const internal::ConnectionPoolInfo& conf;
            // 总数
            int total;
            // 空闲的
            int free;
            // 繁忙的
            int busy;
            // 已关闭的
            int closed;
        };

        typedef std::function<void(ConnectionLockScope)> callback_aquire_connection_t;

        namespace internal
        {
            class ConnectionPool;
        };


        // Mysql
        // 数据库连接池
        //
        class DBPool
        {
        public:
            // 定时检查时间间隔(毫秒）
            static const int64_t CHECK_INTERVAL_TICK;
            // 语句执行超时上限(毫秒)
            static const int64_t STMT_TIMEOUT_TICK;
            // 连接空闲超时关闭时间(毫秒)
            static const int64_t CONN_SPARE_TICK;
            // 连接最大执行上限
            static const uint32_t CONN_MAX_EXECUTE_COUNT;

            bool BeginSetup();
            void BeginCleanup();

            memory::MemoryPool& mempool() {
                return *mempool_;
            }

            const std::unordered_map<std::string, int64_t>& sqlCounts() const {
                return sqlCounts_;
            }

            std::vector<const StatisticInfo*> UpdateAndFetchStatistics();

            // 申请一个独占式的连接，有可能返回空
            // 在需要一个固定的链接上执行多条
            void AquireConnection(int pool_id,
                                  const callback_aquire_connection_t& cb,
                                  const AutoObserver& observer);
            void QueueStatement(int pool_id, StatementBase* stmt);

            void DisconnectAll(int pool_id);

            std::string SqlCountsToJson();

        private:
            DBPool();
            ~DBPool();

            bool ParseConfigure();
            internal::ConnectionPool* FindConnectionPoolByID(int pool_id);
            void IntervalCheck();
            void AddSqlCount(const std::string& sql);

        private:
            friend class ModuleDBO;
            std::vector<internal::ConnectionPool*> pools_;
            memory::MemoryPool* mempool_;
            TimeoutLinker* linker_;

            //sql统计
            bool enableSqlCounts = false;
            struct SqlCount {
                std::string sql;
                int64_t count;
            };
            std::vector<SqlCount> orderedSqlCounts_;
            std::unordered_map<std::string, int64_t> sqlCounts_;
        };

        extern DBPool* g_dbpool;
    }
}

#endif // BASE_DBO_DBPOOL_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
