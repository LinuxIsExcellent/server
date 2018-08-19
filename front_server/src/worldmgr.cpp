#include "worldmgr.h"
#include "playersession.h"
#include "qo/commandstatsave.h"
#include <base/event/dispatcher.h>
#include <base/logger.h>
#include <base/gateway/packet.h>
#include <base/3rd/rapidjson/stringbuffer.h>
#include <base/3rd/rapidjson/writer.h>
#include <base/3rd/rapidjson/document.h>
#include <base/framework.h>
#include <base/cluster/nodemonitor.h>
#include <base/utils/utils_string.h>
#include <base/http/httpclient.h>
#include <base/http/urlbuilder.h>
#include <model/tpl/templateloader.h>
#include <model/tpl/miscconf.h>
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>

namespace fs
{
    using namespace std;
    using namespace base;
    using namespace model::tpl;
    using namespace base::http;

    class HttpRequestEventHandler : public base::http::HttpClientEventHandler
    {
    public:
        HttpRequestEventHandler(base::Observer* observer, std::function<void(const std::vector<MapServiceInfo>&)> cb)
            : HttpClientEventHandler(observer), m_cb(cb) {}

        virtual void OnHttpClose() override {
        }

        virtual void OnHttpResponse(HttpStatusCode code, const string& body) override {
            // cout << "code = " << (int)code << " body = " << body << endl;
            std::vector<MapServiceInfo> infos;
            if (code == HttpStatusCode::OK) {
                try {
                    rapidjson::Document doc;
                    doc.Parse<0>(body.c_str());
                    if (doc.IsArray()) {
                        for (size_t i = 0; i < doc.Size(); ++i) {
                            auto& t = doc[i];
                            MapServiceInfo info;
                            info.id = t.HasMember("id") ? t["id"].GetInt() : 0;
                            info.merge_to_id = t.HasMember("merge_to_id") ? t["merge_to_id"].GetInt() : 0;
                            info.ip = t.HasMember("lan_ip") ? t["lan_ip"].GetString() : "";
                            info.port = t.HasMember("ms_port") ? t["ms_port"].GetInt() : 0;
                            info.name = gWorldMgr->MsIdToName(info.id);

                            //printf("id=%d,merge_to_id=%d,ip=%s,port=%d\n", info.id, info.merge_to_id, info.ip.c_str(), info.port);

                            infos.push_back(info);
                        }
                    }
                } catch (exception& ex) {
                    LOG_ERROR("OnHttpResponse parse json: %s\n", ex.what());
                }
            }
            m_cb(infos);
        }

    private:
        std::function<void(const std::vector<MapServiceInfo>&)> m_cb;
    };

    WorldMgr* gWorldMgr = nullptr;

    WorldMgr* WorldMgr::Create(int msServiceId)
    {
        if (gWorldMgr == nullptr) {
            gWorldMgr = new WorldMgr(msServiceId);
        }

        return gWorldMgr;
    }

    void WorldMgr::Destroy()
    {
        SAFE_DELETE(gWorldMgr);
    }

    WorldMgr::WorldMgr(int msServiceId) : m_msServiceId(msServiceId)
    {
        ReLoadServerList();

        base::cluster::NodeMonitor::instance().evt_node_down.Attach(std::bind(&WorldMgr::OnNodeDown, this, std::placeholders::_1), m_autoObserver);
        base::cluster::NodeMonitor::instance().evt_named_mailbox_up.Attach(std::bind(&WorldMgr::OnNodeMailBoxUp, this, std::placeholders::_1, std::placeholders::_2), m_autoObserver);
        base::cluster::NodeMonitor::instance().evt_named_mailbox_down.Attach(std::bind(&WorldMgr::OnNodeMailBoxDown, this, std::placeholders::_1), m_autoObserver);

        m_lastLogClearTime = g_dispatcher->GetTimestampCache();
        g_dispatcher->quicktimer().SetInterval(std::bind(&WorldMgr::OnTimerUpdate, this), 1000, m_autoObserver);
    }

    WorldMgr::~WorldMgr()
    {
    }

    void WorldMgr::DisconnectAll(const std::function<void()>& cb)
    {
        PlayerSession* p = m_players.front();
        while (p) {
            p->SendLogout();
            p->Exit();
            p = p->list_next();
        }

        g_dispatcher->quicktimer().SetInterval([this, cb]() {
            if (GetPlayerCount() == 0u) {
                m_autoObserver.SetNotExist();
                cb();
            }
            LOG_WARN("GetPlayerCount > 0");
        }, 200, m_autoObserver);
    }

    PlayerSession* WorldMgr::AddPlayer(base::gateway::UserClient* client)
    {
        PlayerSession* ps = new PlayerSession(client, m_msServiceId);
        if (!ps->Setup()) {
            ps->Release();
            return nullptr;
        }
        m_players.push_back(ps);
        return ps;
    }

    void WorldMgr::RemovePlayer(PlayerSession* ps)
    {
        m_players.erase(ps);
        ps->Release();
    }

    int WorldMgr::GetTrueMsId(int msId) const
    {
        for (auto it = m_servers.begin(); it != m_servers.end(); ++it) {
            const MapServiceInfo& ms = *it;
            if (ms.id == msId) {
                return ms.merge_to_id > 0 ? ms.merge_to_id : ms.id;
            }
        }
        return msId;
    }

    const string WorldMgr::MsIdToName(int msId) const
    {
        string name = "";
        base::utils::string_append_format(name, "map.%d@ms", msId);
        return name;
    }

    void WorldMgr::OnTimerUpdate()
    {
        ++m_timerIndex;

        if (m_timerIndex % 1800 == 0) {
            malloc_trim(4096);
        }

        if (m_timerIndex % SAVE_PACKET_INTERVAL == 0) {
            SavePacketData();
        }

        if (m_timerIndex % RELOAD_SERVER_INTERVAL == 0) {
            ReLoadServerList();
        }
    }

    void WorldMgr::OnNodeDown(const base::cluster::NodeInfo& node)
    {
        //std::cout << "WorldMgr::OnNodeDown " << node << std::endl;
        if (node.node_name == "cs") {
            // kick all players and shutdown all front_server
            LOG_WARN("front_server will shutdown because the center_server shutdown!");
            framework.Stop();
        }
    }

    void WorldMgr::OnNodeMailBoxUp(const char* name, const base::cluster::MailboxID& mbid)
    {
        if (strncmp(name, "map.", 3) == 0) {
            // std::cout << "WorldMgr::OnNodeMailBoxUp " << name << std::endl;
            if (m_msMbids.find(name) == m_msMbids.end()) {
                m_msMbids.emplace(name, mbid);
            }
        }
    }

    void WorldMgr::OnNodeMailBoxDown(const char* name)
    {
        if (strncmp(name, "map.", 3) == 0) {
            // std::cout << "WorldMgr::OnNodeMailBoxDown " << name << std::endl;
            m_msMbids.erase(name);
        }
    }

    void WorldMgr::ReLoadServerList()
    {
        const MiscConf& conf = model::tpl::g_tploader->miscConf();
        string url = "http://" + conf.hubSite.ip + ":" + to_string(conf.hubSite.port) + "/gms_api.php";
        // cout << "url = " << url << endl;

        UrlBuilder urlBuilder(url);
        urlBuilder.appendQueryParam("api", "fs_server_list");
        urlBuilder.appendQueryParam("t", to_string(g_dispatcher->GetTickCache()));

        HttpRequestEventHandler* handler = new HttpRequestEventHandler(m_autoObserver.GetObserver(), [this](const vector<MapServiceInfo>& servers) {
            m_servers = servers;
            for (auto it = servers.begin(); it != servers.end(); ++it) {
                const MapServiceInfo& ms = *it;
                if (ms.id != m_msServiceId && ms.merge_to_id == 0 && !IsMsUp(ms.name)) {
                    base::cluster::NodeMonitor::instance().PingNode(ms.ip, ms.port);
                    //cout << "PingNode ip = " << ms.ip << " port = " << ms.port << endl;
                }
            }
        });
        handler->AutoRelease();
        HttpClient::instance()->GetAsync(urlBuilder.getResult(), handler);
    }

    void WorldMgr::OnRecvPacket(base::gateway::PacketIn& pktin)
    {
        m_recvInfo.size += pktin.size();
        ++m_recvInfo.count;

        if (PacketInfo* info = FindPacketInfo(pktin.code())) {
            ++info->count;
            info->size += pktin.size();
        } else {
            PacketInfo pi;
            pi.code = pktin.code();
            pi.isSend = false;
            pi.count = 1;
            pi.size = pktin.size();
            m_packetInfos.emplace(pi.code, pi);
        }
    }

    void WorldMgr::OnSendPacket(base::gateway::PacketOut& pktout)
    {
        m_sendInfo.size += pktout.size();
        ++m_sendInfo.count;

        if (PacketInfo* info = FindPacketInfo(pktout.code())) {
            ++info->count;
            info->size += pktout.size();
        } else {
            PacketInfo pi;
            pi.code = pktout.code();
            pi.isSend = true;
            pi.count = 1;
            pi.size = pktout.size();
            m_packetInfos.emplace(pi.code, pi);
        }
    }

    void WorldMgr::SavePacketData()
    {
        try {
            string k, jsonString;
            rapidjson::StringBuffer jsonbuffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);

            writer.StartObject();

            writer.String("sendSize");
            writer.Int64(m_sendInfo.size - m_sendInfo.lastSize);
            writer.String("sendCount");
            writer.Int64(m_sendInfo.count - m_sendInfo.lastCount);

            writer.String("recvSize");
            writer.Int64(m_recvInfo.size - m_recvInfo.lastSize);
            writer.String("recvCount");
            writer.Int64(m_recvInfo.count - m_recvInfo.lastCount);

            writer.EndObject();

            jsonString = jsonbuffer.GetString();
            k.append("bandwidth_");
            k.append(to_string(framework.server_rule().id()));
            m_runner.PushCommand(new qo::CommandStatSave(k, jsonString));
            m_sendInfo.LogLast();
            m_recvInfo.LogLast();
        } catch (std::exception& ex) {
            LOG_ERROR("save bandwidth json fail: %s\n", ex.what());
        }

        try {
            string k, jsonString;
            rapidjson::StringBuffer jsonbuffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
            writer.StartArray();

            for (auto it = m_packetInfos.begin(); it != m_packetInfos.end(); ++it) {
                const PacketInfo& info = it->second;
                writer.StartObject();

                writer.String("code");
                writer.Int(info.code);
                writer.String("isSend");
                writer.Bool(info.isSend);
                writer.String("count");
                writer.Int(info.count);
                writer.String("size");
                writer.Int64(info.size);

                writer.EndObject();
            }

            writer.EndArray();
            jsonString = jsonbuffer.GetString();
            k.append("code_packet_");
            k.append(to_string(framework.server_rule().id()));
            m_runner.PushCommand(new qo::CommandStatSave(k, jsonString));
        } catch (std::exception& ex) {
            LOG_ERROR("save packetInfos json fail: %s\n", ex.what());
        }

        int64_t now = g_dispatcher->GetTimestampCache();
        if (now - m_lastLogClearTime > 6 * 3600) {
            m_packetInfos.clear();
            m_sendInfo.Clear();
            m_recvInfo.Clear();
            m_lastLogClearTime = now;
        }
    }

}

