#ifndef LUAMAPAGENT_H
#define LUAMAPAGENT_H

#include <base/cluster/mailbox.h>
#include <base/cluster/rpcstub.h>
#include <base/3rd/lua/lua.hpp>
#include <base/observer.h>

namespace fs
{
    typedef std::function<void(base::cluster::MessageIn&)> rpc_callback_t;
    typedef std::function<void()> rpc_error_callback_t;

    class RpcCallObject;
    class PlayerSession;
    static const char* MT_LUA_MAP_AGENT = "mt_lua_map_agent";

    int lua_get_map_registry(lua_State* L);

    class LuaMapAgent : public base::cluster::Mailbox::EventHandler
    {
    public:
        LuaMapAgent(PlayerSession* ps);
        ~LuaMapAgent();

        base::memory::MemoryPool& mempool() {
            return m_mailbox->mempool();
        }
        void SetUid(int64_t uid) {
            m_uid = uid;
        }
        void SetMapServiceId(int mid) {
            m_mapServiceId = mid;
            m_currentJoinMap = mid;
        }
        void ClearPlayerSession() {
            m_ps = nullptr;
        }
        virtual void OnMessageReceive(base::cluster::MessageIn& msgin) final;
        bool ConnectMapService(const std::string& service_name);

        void Cast(base::cluster::MessageOut& msgout);
        void Call(base::cluster::MessageOut& msgout, const std::function<void(base::cluster::MessageIn&)>& cb, const base::AutoObserver& atob);
        void Call(base::cluster::MessageOut& msgout, const rpc_callback_t& cb, const rpc_error_callback_t& error_cb, const base::AutoObserver& atob);
        void Call(base::cluster::MessageOut& msgout, const rpc_callback_t& cb, const rpc_error_callback_t& error_cb, base::Observer* observer);

    public:
        static int _luaCall(lua_State* L);
        static int _luaCast(lua_State* L);
        static int _forward(lua_State* L);
        static int _forwardOther(lua_State* L); // 其它区服
        static int _setMapEvtHandler(lua_State* L);
        static int _gc(lua_State* L) {
            if (LuaMapAgent* self = (LuaMapAgent*)luaL_checkudata(L, 1, MT_LUA_MAP_AGENT)) {
                self->~LuaMapAgent();
            }
            return 0;
        }
        static int _crossTeleport(lua_State* L);
        static int _checkMsExist(lua_State* L);
        static int _quitAllMap(lua_State* L);

    private:
        uint16_t GenSessionID() {
            uint16_t ret = session_cur_++;
            return ret;
        }
        void EnterMap(const base::cluster::MailboxID& mbid);
        void QuitMap(const base::cluster::MailboxID& mbid);
        
        // 接收到订阅消息
        void OnSubscribeArrive(base::cluster::MessageIn& msgin);
        
        void OnServiceUp(const base::cluster::MailboxID& mbid) ;
        void OnServiceDown();
        
    private:
        PlayerSession* m_ps;
        uint16_t session_cur_ = 1;
        base::cluster::Mailbox* m_mailbox = nullptr;
        base::cluster::MailboxID m_service_mbid;
        base::cluster::MailboxID m_other_service_mbid; // 其它游戏区地图服务器的 mbid
        base::AutoObserver m_observer;
        std::unordered_map<uint16_t, RpcCallObject*> calls_;
        
        int64_t m_uid = 0;
        int m_mapServiceId = 0;
        int m_currentJoinMap = 0;
        std::string m_service_name;
    };
}


#endif // LUAMAPAGENT_H
