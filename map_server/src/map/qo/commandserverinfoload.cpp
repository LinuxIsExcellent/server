#include "commandserverinfoload.h"
#include "../mapMgr.h"
#include <base/dbo/preparedstatement.h>
#include <base/logger.h>
#include <base/http/httpclient.h>
#include <base/http/urlbuilder.h>
#include <base/event/dispatcher.h>
#include <rapidjson/document.h>
#include <model/tpl/templateloader.h>
#include <model/tpl/miscconf.h>

namespace ms
{
    namespace map
    {
        namespace qo
        {
            using namespace std;
            using namespace base::dbo;
            using namespace base::http;
            using namespace model::tpl;
            
            class HttpRequestEventHandler : public base::http::HttpClientEventHandler
            {
            public:
                HttpRequestEventHandler(base::Observer* observer, std::function<void(HttpStatusCode, const string&)> cb)
                    : HttpClientEventHandler(observer), m_cb(cb) {}

                virtual void OnHttpClose() override {
                }

                virtual void OnHttpResponse(HttpStatusCode code, const string& body) override {
                    // cout << "code = " << (int)code << " body = " << body << endl;
                    m_cb(code, body);
                }

            private:
                std::function<void(HttpStatusCode, const string&)> m_cb;
            };

            void CommandServerInfoLoad::OnCommandExecute()
            {
                /*
                PreparedStatement* pstmt = PreparedStatement::Create(4, "SELECT name, ost, pwt FROM s_area WHERE id = ?");
                pstmt->SetInt32(g_mapMgr->localServiceId());
                pstmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        LOG_ERROR("CommandServerInfoLoad Error");
                        m_cb(false);
                    } else {
                        while (rs.Next()) {
                            int i = 0;
                            g_mapMgr->m_serverName = rs.GetString(++i);
                            g_mapMgr->m_openServerTime = rs.GetInt64(++i);
                            g_mapMgr->m_palaceWarTime = rs.GetInt64(++i);
                        }
                        m_cb(true);
                    }
                    Stop();
                }, m_autoObserver);
                */
                
                const MiscConf& conf = g_tploader->miscConf();
                string url = "http://" + conf.hubSite.ip + ":" + to_string(conf.hubSite.port) + "/gms_api.php";
                // cout << "url = " << url << endl;

                UrlBuilder urlBuilder(url);
                urlBuilder.appendQueryParam("api", "ms_load_area");
                urlBuilder.appendQueryParam("id", to_string(g_mapMgr->m_localServiceId));
                urlBuilder.appendQueryParam("t", to_string(g_dispatcher->GetTickCache()));

                HttpRequestEventHandler* handler = new HttpRequestEventHandler(m_autoObserver.GetObserver(), [this](HttpStatusCode code, const string & body) {
                    if (code != base::http::HttpStatusCode::OK) {
                        LOG_ERROR("CommandServerInfoLoad Error");
                        m_cb(false);
                    } else {
                        try {
                            rapidjson::Document doc;
                            doc.Parse<0>(body.c_str());
                            if (doc.IsObject()) {
                                g_mapMgr->m_serverName = doc.HasMember("name") ? doc["name"].GetString() : g_mapMgr->m_serverName;
                                g_mapMgr->m_openServerTime = doc.HasMember("ost") ? doc["ost"].GetInt64() : g_mapMgr->m_openServerTime;
                                // cout << "=====================openTime = " << g_mapMgr->m_openServerTime << endl;
                                if (g_mapMgr->m_openServerTime == 0) {
                                    g_mapMgr->m_openServerTime = g_dispatcher->GetTimestampCache();
                                }
                                if (doc.HasMember("merged_ids") && doc["merged_ids"].IsArray()) {
                                    g_mapMgr->m_merged_ids.clear();
                                    auto& temp = doc["merged_ids"];
                                    for (size_t i = 0; i < temp.Size(); ++i) {
                                        // 已合并的区ID
                                        std::cout << "merged_ids = " << temp[i].GetInt() << std::endl;
                                        g_mapMgr->m_merged_ids.push_back(temp[i].GetInt());
                                    }
                                    if(g_mapMgr->m_merged_ids.empty()) {
                                        g_mapMgr->m_merged_ids.push_back(g_mapMgr->m_localServiceId);
                                    }
                                }
                                printf("name=%s,ost=%ld\n", g_mapMgr->m_serverName.c_str(), g_mapMgr->m_openServerTime);
                            }  else
                            {
                                LOG_ERROR("CommandServerInfoLoad::OnHttpResponse parse json is null");
                            }

                            m_cb(true);
                        } catch (exception& ex) {
                            LOG_ERROR("CommandServerInfoLoad::OnHttpResponse parse json: %s\n", ex.what());
                        }
                    }
                    Stop();
                });
                handler->AutoRelease();
                HttpClient::instance()->GetAsync(urlBuilder.getResult(), handler);
            }
        }
    }
}
