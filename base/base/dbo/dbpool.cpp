#include "dbpool.h"
#include "internal/configure.h"
#include "../framework.h"
#include "../utils/file.h"
#include "../memory/memorypool.h"
#include "../memory/memorypoolmgr.h"
#include "../logger.h"
#include "../event/dispatcher.h"
#include "connection.h"
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <functional>
#include <algorithm>

namespace base
{
    namespace dbo
    {
        using namespace std;

        namespace internal
        {
            class ConnectionPool : public Connection::EventHandler
            {
            public:
                ConnectionPool(const ConnectionPoolInfo& conf)
                    : conf_(conf), stat_(conf) {}

                ~ConnectionPool() {
                    for (vector<Connection*>::iterator it = conns_.begin(); it != conns_.end(); ++it) {
                        (*it)->Release();
                    }
                    conns_.clear();

                    while (!q_stmts_.empty()) {
                        q_stmts_.front()->Release();
                        q_stmts_.pop_front();
                    }
                    while (!q_aquire_.empty()) {
                        delete q_aquire_.front();
                        q_aquire_.pop();
                    }
                }

                const ConnectionPoolInfo& conf() const {
                    return conf_;
                }

                void Setup() {
                    AdjustPoolSize(conf_.poolsize);
                }

                void AquireConnection(const callback_aquire_connection_t& cb,
                                      const AutoObserver& observer) {

                    Connection* conn = PickFreeConnection();
                    if (conn != nullptr) {
                        cb(ConnectionLockScope(conn));
                    } else {
                        AquireJob* job = new AquireJob(cb, observer.GetObserver());
                        q_aquire_.push(job);
                    }
                }


                void IntervalCheck() {
                    int64_t now_tick = g_dispatcher->GetTickCache();

                    if (q_stmts_.empty()) {
                        for (vector<Connection*>::iterator it = conns_.begin(); it != conns_.end(); ++it) {
                            Connection* c = (*it);
                            if (c->status() == Connection::CONN_FREE) {
                                if ((now_tick - c->stat().last_active_tick > DBPool::CONN_SPARE_TICK)) {
                                    c->Disconnect();
                                    LOG_WARN("db conn[%d] is spare, so close it", conf_.poolid);
                                } else if (c->stat().execute_count > DBPool::CONN_MAX_EXECUTE_COUNT) {
                                    c->Disconnect();
                                    LOG_WARN("db conn[%d] execute too much times, so close it", conf_.poolid);
                                }
                            }
                        }
                    } else if (q_stmts_.size() > 5) {
                        // 异常繁忙, 加大连接池
                        UpdateStatistic();
                        int diff = conf_.max_poolsize - stat_.ActiveCount();
                        if (diff > 0) {
                            if (diff > 2) {
                                diff = 2;
                            }
                            // 一点一点增加，每次加２，直到达到最大连接数
                            AdjustPoolSize(stat_.ActiveCount() + diff);
                            UpdateStatistic();
                            LOG_WARN("db pool[%d] is too busy, auto increment connection to=%d", conf_.poolid, stat_.ActiveCount());
                        }
                    }

                    // 检查语句是否等待超时
                    for (deque<StatementBase*>::iterator it = q_stmts_.begin(); it != q_stmts_.end();) {
                        StatementBase* stmt = *it;
                        if (now_tick - stmt->create_tick() > DBPool::STMT_TIMEOUT_TICK) {
                            it = q_stmts_.erase(it);
                            stmt->HandleError(-2, "wait timeout");
                            stmt->Release();
                        } else {
                            ++it;
                        }
                    }
                }

                void DisconnectAll() {
                    for (vector<Connection*>::iterator it = conns_.begin(); it != conns_.end(); ++it) {
                        if ((*it)->locked()) {
                            LOG_ERROR("db connection is still locked when close it!");
                        }
                        (*it)->Disconnect();
                    }
                    if (!q_stmts_.empty()) {
                        LOG_WARN("still exist pending stmt when close db connection, size = % lu", q_stmts_.size());
                    }
                }

                const StatisticInfo& UpdateStatistic() {
                    stat_.Reset();
                    for (vector<Connection*>::const_iterator it = conns_.begin(); it != conns_.end(); ++it) {
                        Connection::ConnectionStatus status = (*it)->status();
                        ++stat_.total;
                        switch (status) {
                            case Connection::CONN_BUSY:
                                ++stat_.busy;
                                break;
                            case Connection::CONN_FREE:
                                ++stat_.free;
                                break;
                            case Connection::CONN_INIT:
                                break;
                            case Connection::CONN_CLOSED:
                                ++stat_.closed;
                                break;
                            case Connection::CONN_CLOSING:
                                break;
                        }
                    }
                    return stat_;
                }

                void QueueStatement(StatementBase* stmt) {
                    stmt->Retain();
                    q_stmts_.push_back(stmt);
                    TryExecute();
                }

            private:
                void AdjustPoolSize(int count) {
                    UpdateStatistic();
                    if (stat_.ActiveCount() < count) {
                        for (vector<Connection*>::iterator it = conns_.begin(); it != conns_.end(); ++it) {
                            Connection* c = (*it);
                            if (c->status() == Connection::CONN_CLOSED) {
                                c->Connect();
                                --count;
                                if (count == 0) {
                                    return;
                                }
                            }
                        }
                    }
                    UpdateStatistic();
                    for (int i = stat_.ActiveCount(); i < count; ++i) {
                        Connection* conn = new Connection(conf_.poolid, conf_.serverinfo, *this);
                        conns_.push_back(conn);
                        conn->Connect();
                    }
                }

                virtual void OnConnectBecomeFree(Connection* conn) {
                    while (!q_aquire_.empty()) {
                        Connection* conn = PickFreeConnection();
                        if (conn == nullptr) {
                            break;
                        } else {
                            AquireJob* job = q_aquire_.front();
                            q_aquire_.pop();
                            if (job->observer->IsExist()) {
                                job->cb(ConnectionLockScope(conn));
                            }
                            delete job;
                        }
                    }
                    TryExecute();
                }

                Connection* PickFreeConnection() {
                    for (vector<Connection*>::iterator it = conns_.begin(); it != conns_.end(); ++it) {
                        if ((*it)->status() == Connection::CONN_FREE && !(*it)->locked()) {
                            return *it;
                        }
                    }
                    return nullptr;
                }

                void TryExecute() {
                    if (q_stmts_.empty()) {
                        return;
                    }
                    Connection* conn = PickFreeConnection();
                    if (conn != nullptr) {
                        while (!q_stmts_.empty()) {
                            StatementBase* stmt = q_stmts_.front();
                            q_stmts_.pop_front();
                            if (stmt->IsValidate()) {
                                conn->ExecuteStatement(stmt);
                                stmt->Release();
                                break;
                            } else {
                                // 如果已经失效，则不再执行
                                stmt->Release();
                            }
                        }
                    } else {
                        UpdateStatistic();
                        if (stat_.ActiveCount() < conf_.poolsize) {
                            AdjustPoolSize(conf_.poolsize);
                        }
                    }
                }

                ConnectionPoolInfo conf_;
                StatisticInfo stat_;
                std::deque<StatementBase*> q_stmts_;
                vector<Connection*> conns_;
                struct AquireJob {
                    AquireJob(const callback_aquire_connection_t& _cb, Observer* _observer)
                        : cb(_cb), observer(_observer) {
                        observer->Retain();
                    }
                    ~AquireJob() {
                        observer->Release();
                    }
                    callback_aquire_connection_t cb;
                    Observer* observer;
                };
                std::queue<AquireJob*> q_aquire_;
            };
        }

        /// DBPool

        DBPool* g_dbpool = nullptr;

        const int64_t DBPool::CHECK_INTERVAL_TICK = 10 * 1000;          // 10s
        const int64_t DBPool::STMT_TIMEOUT_TICK = 60 * 1000;            // 60s
        const int64_t DBPool::CONN_SPARE_TICK = 60 * 60 * 1000;         // 1hour
        const uint32_t DBPool::CONN_MAX_EXECUTE_COUNT = 5000000;        // 500w

        DBPool::DBPool() : mempool_(nullptr), linker_(nullptr)
        {
            g_dbpool = this;
        }

        DBPool::~DBPool()
        {
            g_dbpool = nullptr;
            SAFE_RELEASE(linker_);
            for (vector<internal::ConnectionPool*>::iterator it = pools_.begin(); it != pools_.end(); ++it) {
                delete *it;
            }
            pools_.clear();
        }

        bool DBPool::ParseConfigure()
        {
            vector<internal::ConnectionPoolInfo> configs;

            string conf_file = framework.priv_dir() + "/dbo.conf";
            if (base::utils::file_is_exist(conf_file.c_str())) {
                if (!internal::ParseDBConfigureFile(conf_file.c_str(), configs)) {
                    return false;
                }
            }

            conf_file = framework.resource_dir() + "/dbo.conf";
            if (base::utils::file_is_exist(conf_file.c_str())) {
                if (!internal::ParseDBConfigureFile(conf_file.c_str(), configs)) {
                    return false;
                }
            }

            if (configs.empty()) {
                LOG_ERROR("empty db config!");
                return false;
            }

            for (vector<internal::ConnectionPoolInfo>::iterator it = configs.begin(); it != configs.end(); ++it) {
                const internal::ConnectionPoolInfo& conf = *it;
                if (FindConnectionPoolByID(conf.poolid)) {
                    continue;
                }
                internal::ConnectionPool* pool = new internal::ConnectionPool(conf);
                pools_.push_back(pool);
                pool->Setup();
            }

            return true;
        }

        std::vector<const StatisticInfo*> DBPool::UpdateAndFetchStatistics()
        {
            vector<const StatisticInfo*> ret;
            for (vector<internal::ConnectionPool*>::iterator it = pools_.begin(); it != pools_.end(); ++it) {
                const StatisticInfo& stat = (*it)->UpdateStatistic();
                ret.push_back(&stat);
            }
            return ret;
        }

        internal::ConnectionPool* DBPool::FindConnectionPoolByID(int pool_id)
        {
            for (vector<internal::ConnectionPool*>::iterator it = pools_.begin(); it != pools_.end(); ++it) {
                if ((*it)->conf().poolid == pool_id) {
                    return (*it);
                }
            }
            return nullptr;
        }

        void DBPool::AquireConnection(int pool_id, const callback_aquire_connection_t& cb, const AutoObserver& observer)
        {
            internal::ConnectionPool* pool = FindConnectionPoolByID(pool_id);
            if (pool != nullptr) {
                pool->AquireConnection(cb, observer);
            } else {
                cb(ConnectionLockScope(nullptr));
            }
        }

        void DBPool::QueueStatement(int pool_id, StatementBase* stmt)
        {
            internal::ConnectionPool* pool = FindConnectionPoolByID(pool_id);
            if (pool != nullptr) {
                pool->QueueStatement(stmt);
            } else {
                stmt->HandleError(-1, "not exist pool id");
            }

            if (enableSqlCounts) {
                AddSqlCount(stmt->sql());
            }
        }

        bool DBPool::BeginSetup()
        {
            mempool_ = memory::g_memory_pool_mgr->Aquire("dbpool", 256, 512);
            bool ok = ParseConfigure();
            linker_ = g_dispatcher->quicktimer().SetIntervalWithLinker(std::bind(&DBPool::IntervalCheck, this), CHECK_INTERVAL_TICK);
            linker_->Retain();
            return ok;
        }

        void DBPool::BeginCleanup()
        {
            SAFE_RELEASE(linker_);
            // 关闭所有连接
            for (vector<internal::ConnectionPool*>::iterator it = pools_.begin(); it != pools_.end(); ++it) {
                (*it)->DisconnectAll();
            }
        }

        void DBPool::IntervalCheck()
        {
            // 检查超时与空闲的链接
            for (vector<internal::ConnectionPool*>::iterator it = pools_.begin(); it != pools_.end(); ++it) {
                (*it)->IntervalCheck();
            }
        }

        void DBPool::DisconnectAll(int pool_id)
        {
            internal::ConnectionPool* pool = FindConnectionPoolByID(pool_id);
            if (pool != nullptr) {
                pool->DisconnectAll();
            }
        }

        void DBPool::AddSqlCount(const string& sql)
        {
            auto it = sqlCounts_.find(sql);
            if (it != sqlCounts_.end()) {
                ++it->second;
            } else {
                sqlCounts_.emplace(sql, 1);
            }
        }

        string DBPool::SqlCountsToJson()
        {
            orderedSqlCounts_.clear();
            for (auto it = sqlCounts_.begin(); it != sqlCounts_.end(); ++it) {
                if (it->second >= 10) {
                    SqlCount info;
                    info.sql = it->first;
                    info.count = it->second;
                    orderedSqlCounts_.emplace_back(info);
                }
            }
            sort(orderedSqlCounts_.begin(), orderedSqlCounts_.end(), [](const SqlCount& a, const SqlCount& b) {
                return a.count > b.count;
            });

            string jsonString;
            try {
                rapidjson::StringBuffer jsonbuffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                writer.StartArray();
                for (size_t i = 0; i < orderedSqlCounts_.size() && i < 15; ++i) {
                    writer.StartObject();
                    writer.String("sql");
                    writer.String(orderedSqlCounts_[i].sql.c_str());
                    writer.String("count");
                    writer.Int64(orderedSqlCounts_[i].count);
                    writer.EndObject();
                }
                writer.EndArray();
                jsonString = jsonbuffer.GetString();
            } catch (std::exception& ex) {
                LOG_ERROR("DBPool::SqlCountsToJson fail: %s\n", ex.what());
            }
            return jsonString;
        }

    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
