#include "httpclient.h"
#include <http_parser/http_parser.h>
#include "../gateway/packet_base.h"
#include "../net/client.h"
#include "../logger.h"
#include "hostnameresolver.h"
#include "urlbuilder.h"
#include "../thread/threadpool.h"
#include "../event/dispatcher.h"
#include "../utils/utils_string.h"
#include <netdb.h>
#include <stdio.h>
#include <iostream>

namespace base
{
    namespace http
    {
        using namespace std;

        static struct http_parser_settings settings;

        class PacketHttpRequest : public gateway::PacketOutBase
        {
        public:
            PacketHttpRequest(uint32_t approx_size, memory::MemoryPool& mempool)
                : gateway::PacketOutBase(approx_size, mempool) {
                SkipTo(0);
            }
        };

        /// HttpConnection
        class HttpConnection : public net::Client
        {
        public:
            HttpConnection(memory::MemoryPool& mempool, HttpClientEventHandler* handler)
                : net::Client(mempool), req_(64, mempool), handler_(handler) {
                SAFE_RETAIN(handler_);
                http_parser_init(&parser_, HTTP_RESPONSE);
                parser_.data = this;
            }

            virtual ~HttpConnection() {
                SAFE_RELEASE(handler_);
            }

            void WriteRequestHead(HttpMethod method, const string& host, const string& path) {
                if (m_hasHead) {
                    return;
                }
                m_hasHead = true;
                if (method == HttpMethod::GET) {
                    req_.WriteRaw("GET ", 4);
                    req_.WriteRaw(path.c_str(), path.length());
                    req_.WriteRaw(" HTTP/1.1\r\n", 11);
                    req_.WriteRaw("HOST: ", 6);
                    req_.WriteRaw(host.c_str(), host.length());
                    req_.WriteRaw("\r\n", 2);
                    req_.WriteRaw("Content-Length: 0\r\n", 19);
                    req_.WriteRaw("\r\n", 2);
                    m_hasBody = true;
                } else {
                    req_.WriteRaw("POST ", 5);
                    req_.WriteRaw(path.c_str(), path.length());
                    req_.WriteRaw(" HTTP/1.1\r\n", 11);
                    req_.WriteRaw("HOST: ", 6);
                    req_.WriteRaw(host.c_str(), host.length());
                    req_.WriteRaw("\r\n", 2);
                }
            }

            void WriteRequestBody(const char* contentType, const std::string& content) {
                if (!m_hasHead || m_hasBody) {
                    return;
                }
                m_hasBody = true;
                string tmp;
                base::utils::string_append_format(tmp, "Content-Type: %s\r\n", contentType);
                req_.WriteRaw(tmp.c_str(), tmp.length());
                tmp.clear();
                base::utils::string_append_format(tmp, "Content-Length: %llu\r\n", content.size());
                req_.WriteRaw(tmp.c_str(), tmp.length());
                req_.WriteRaw("\r\n", 2);
                req_.WriteRaw(content.c_str(), content.size());
            }

            void WriteRequestBodyWithForm(const std::string& content) {
                WriteRequestBody("application/x-www-form-urlencoded", content);
            }

            void WriteRequestBodyWithJson(const std::string& content) {
                WriteRequestBody("application/json", content);
            }

            void OnReceive(std::size_t count) override {
                vector<memory::RefMemoryChunk> data;
                PopReceive(data, count);
                for (vector<memory::RefMemoryChunk>::iterator it = data.begin(); it != data.end(); ++it) {
                    size_t r = http_parser_execute(&parser_, &settings, (*it).data(), (*it).count());
                    if (r != (*it).count()) {
                        if (handler_ && handler_->IsExist()) {
                            handler_->OnHttpResponse(HttpStatusCode::Not_Acceptable, HTTP_STATUS_STRING(HttpStatusCode::Not_Acceptable));
                        }
                        return;
                    }
                }
            }

            void OnConnect() override {
                Flush();
            }

            void OnConnectFail(int eno, const char* reason) override {
                if (handler_ && handler_->IsExist()) {
                    handler_->OnHttpResponse(HttpStatusCode::Request_Timeout, HTTP_STATUS_STRING(HttpStatusCode::Request_Timeout));
                }
            }

            void OnClose() override {
                if (handler_ && handler_->IsExist()) {
                    handler_->OnHttpClose();
                }
            }

            void Flush() {
                std::vector<memory::RefMemoryChunk> data = req_.FetchData();
                PushSend(data);
            }
        private:
            bool m_hasHead = false;
            bool m_hasBody = false;
            PacketHttpRequest req_;
            http_parser parser_;
            std::string resp_body_;
            HttpClientEventHandler* handler_;

        public:
            static int on_message_begin_cb(http_parser* parser) {
                HttpConnection* client = static_cast<HttpConnection*>(parser->data);
                client->resp_body_.reserve(1024);
                return 0;
            }

            static int on_status(http_parser* parser, const char* at, size_t len) {
                return 0;
            }

            static int on_header_field(http_parser* parser, const char* at, size_t len) {
                return 0;
            }

            static int on_header_value(http_parser* parser, const char* at, size_t len) {
                return 0;
            }

            static int on_headers_complete(http_parser* parser) {
                return 0;
            }

            static int on_body_cb(http_parser* parser, const char* at, size_t len) {
                HttpConnection* client = static_cast<HttpConnection*>(parser->data);
                client->resp_body_.append(at, len);
                return 0;
            }

            static int on_message_complete_cb(http_parser* parser) {
                HttpConnection* client = static_cast<HttpConnection*>(parser->data);
                if (client->handler_ && client->handler_->IsExist()) {
                    client->handler_->OnHttpResponse((HttpStatusCode)parser->status_code, client->resp_body_.c_str());
                }
                client->Close();
                return 0;
            }
        };

        static struct http_parser_settings_initializer {
            http_parser_settings_initializer() {
                memset(&settings, 0, sizeof(settings));
                settings.on_status = HttpConnection::on_status;
                settings.on_header_field = HttpConnection::on_header_field;
                settings.on_header_value = HttpConnection::on_header_value;
                settings.on_headers_complete = HttpConnection::on_headers_complete;
                settings.on_body = HttpConnection::on_body_cb;
                settings.on_message_begin = HttpConnection::on_message_begin_cb;
                settings.on_message_complete = HttpConnection::on_message_complete_cb;
            }
        } _settings_initializer;


        HttpClient::~HttpClient()
        {
        }

        static HttpClient* s_instance = nullptr;

        HttpClient* HttpClient::Create(memory::MemoryPool& mempool)
        {
            if (s_instance == nullptr) {
                s_instance = new HttpClient(mempool);
            }
            return s_instance;
        }

        void HttpClient::Destroy()
        {
            SAFE_DELETE(s_instance);
        }

        HttpClient* HttpClient::instance()
        {
            return s_instance;
        }

        void HttpClient::ResolveHostname(const std::string& hostname, const std::function<void(const DnsRecord&)>& cb, Observer* observer)
        {
            // find from cache
            auto it = dns_cache_.find(hostname);
            if (it != dns_cache_.end()) {
                const DnsRecord& record = it->second;
                if (g_dispatcher->GetTimestampCache() - record.createTs < 30 * 60) {
                    if (observer && observer->IsExist()) {
                        cb(it->second);
                    }
                    return;
                }
            }

            CallbackObserver<void(const DnsRecord&)> cb2(cb, observer);
            CallbackObserver<void(const DnsRecord&)> cb3([cb2, this](const DnsRecord & result) {
                // cache result
                if (!result.empty()) {
                    dns_cache_[result.hostname] = result;
                } else {
                    dns_cache_.erase(result.hostname);
                }
                cb2(result);
            }, autoObserver_);
            shared_ptr<HostnameResolver> resolver(new HostnameResolver(hostname, cb3));
            thread::ThreadPool::getInstance()->queueUserWorkItem(resolver);
        }

        static HttpClient::Error parseUrl(const string& url, string& host, string& path, int* port)
        {
            http_parser_url urlParser;
            http_parser_url_init(&urlParser);
            int err = http_parser_parse_url(url.c_str(), url.length(), 0, &urlParser);
            if (err) {
                return HttpClient::Error::BAD_URL;
            }
            if ((urlParser.field_set & (1 << UF_HOST)) == 0) {
                return HttpClient::Error::BAD_URL;
            }

            host.assign(url.c_str() + urlParser.field_data[UF_HOST].off, urlParser.field_data[UF_HOST].len);
            *port = urlParser.port == 0 ? 80 : urlParser.port;

            path.assign("/");
            if ((urlParser.field_set & (1 << UF_PATH)) != 0) {
                path.assign(url.c_str() + urlParser.field_data[UF_PATH].off, urlParser.field_data[UF_PATH].len);
            }
            if ((urlParser.field_set && (1 << UF_QUERY)) != 0) {
                path.append("?");
                path.append(url.c_str() + urlParser.field_data[UF_QUERY].off, urlParser.field_data[UF_QUERY].len);
            }
            return HttpClient::Error::OK;
        }

        HttpClient::Error HttpClient::GetAsync(const string& url, HttpClientEventHandler* evt_handler)
        {
            string host;
            string path;
            int port;
            Error error = parseUrl(url, host, path, &port);
            if (error != Error::OK) {
                return error;
            }

            SharedObject<Object> wp;
            wp.Ref(evt_handler);
            ResolveHostname(host, [evt_handler, wp, this, host, path, port](const DnsRecord & dns) {
                string ip = dns.getIP();
                if (ip.empty()) {
                    if (evt_handler && evt_handler->IsExist()) {
                        evt_handler->OnHttpResponse(HttpStatusCode::No_Content, "dns resolve fail");
                    }
                } else {
                    //cout << "req:" << host << ":" << port << path << endl;
                    HttpConnection* conn = new HttpConnection(mempool_, evt_handler);
                    conn->WriteRequestHead(HttpMethod::GET, host, path);
                    conn->Connect(ip.c_str(), port);
                    conn->AutoRelease();
                }
            }, autoObserver_.GetObserver());
            return Error::OK;
        }

        HttpClient::Error HttpClient::PostFormAsync(const string& url, const vector< pair< string, string > >& formParams, HttpClientEventHandler* evtHandler)
        {
            string host;
            string path;
            int port;
            Error error = parseUrl(url, host, path, &port);
            if (error != Error::OK) {
                return error;
            }

            string content;
            for (size_t i = 0u; i < formParams.size(); ++i) {
                const pair<string, string>& param  = formParams[i];
                content.append(UrlBuilder::UrlEncode(param.first));
                content.append("=");
                content.append(UrlBuilder::UrlEncode(param.second));
                if (i < formParams.size() - 1u) {
                    content.append("&");
                }
            }

            SharedObject<Object> wp;
            wp.Ref(evtHandler);
            ResolveHostname(host, [evtHandler, wp, this, host, content, path, port](const DnsRecord & dns) {
                string ip = dns.getIP();
                if (ip.empty()) {
                    if (evtHandler && evtHandler->IsExist()) {
                        evtHandler->OnHttpResponse(HttpStatusCode::No_Content, "dns resolve fail");
                    }
                } else {
                    //cout << "req:" << host << ":" << port << path << endl;
                    HttpConnection* conn = new HttpConnection(mempool_, evtHandler);
                    conn->WriteRequestHead(HttpMethod::POST, host, path);
                    conn->WriteRequestBodyWithForm(content);
                    conn->Connect(ip.c_str(), port);
                    conn->AutoRelease();
                }
            }, autoObserver_.GetObserver());
            return Error::OK;
        }

        HttpClient::Error HttpClient::PostJsonAsync(const string& url, const string& json, HttpClientEventHandler* evtHandler)
        {
            string host;
            string path;
            int port;
            Error error = parseUrl(url, host, path, &port);
            if (error != Error::OK) {
                return error;
            }

            SharedObject<Object> wp;
            wp.Ref(evtHandler);
            ResolveHostname(host, [evtHandler, wp, this, host, json, path, port](const DnsRecord & dns) {
                string ip = dns.getIP();
                if (ip.empty()) {
                    if (evtHandler && evtHandler->IsExist()) {
                        evtHandler->OnHttpResponse(HttpStatusCode::No_Content, "dns resolve fail");
                    }
                } else {
                    //cout << "req:" << host << ":" << port << path << endl;
                    HttpConnection* conn = new HttpConnection(mempool_, evtHandler);
                    conn->WriteRequestHead(HttpMethod::POST, host, path);
                    conn->WriteRequestBodyWithJson(json);
                    conn->Connect(ip.c_str(), port);
                    conn->AutoRelease();
                }
            }, autoObserver_.GetObserver());
            return Error::OK;
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
