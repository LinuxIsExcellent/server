#ifndef BASE_HTTP_HTTPACCEPTOR_H
#define BASE_HTTP_HTTPACCEPTOR_H

#include "../net/client.h"
#include "../utils/utils_string.h"
#include <http_parser/http_parser.h>
#include "../gateway/packet_base.h"
#include "../logger.h"



namespace base
{
    namespace memory
    {
        class MemoryPool;
    }

    namespace http
    {

        using namespace std;

        enum class HttpMethod
        {
            GET = 1,
            POST = 2,
        };

        struct HttpRequest {
        public:
            HttpRequest() {}
            HttpRequest(HttpMethod m,string& p,string& b)
                :path(p),body(b),method(m) {
            }
            void ReSet() {
                path.clear();
                query.clear();
                body.clear();
                method = HttpMethod::POST;
            }
            string path;
            string query;
            string body;
            HttpMethod method;
        };

        class HttpReponse : public gateway::PacketOutBase
        {
        public:
            HttpReponse(const uint64_t code,uint32_t approx_size, memory::MemoryPool& mempool)
                :gateway::PacketOutBase(approx_size, mempool),code_(code) {
                SkipTo(0);
            }

            gateway::packet_data_t Fetch() {
                size_t index = data_.find_last_of(',');                //去掉最后一个逗号
                if (index!=string::npos) {
                    data_.erase(index);
                }
                data_= "{"+data_;
                data_.append("}");
                WriteResponse(data_);
                return FetchData();
            }


            void WriteResponse(const string& body) {
                WriteRaw("HTTP/1.1", 8);
                char tempCode[32];
                sprintf(tempCode," %lu", code_);

                //tempCode = base::utils::string_append_format("%u",code_);
                WriteRaw(tempCode, strlen(tempCode));
                if (200 == code_) {
                    WriteRaw(" OK",3);
                }
                WriteRaw("\r\n", 2);
                const char* date =format_http_time_str();
                WriteRaw(date, strlen(date));
                WriteRaw("\r\n",2);
                WriteRaw("Content-Type: application/Json;utf-8\r\n", 38);
                WriteRaw("Content-Length: ", 16);
                char temp[32];
                sprintf(temp, "%lu", body.length());
                //tempCode = base::utils::string_append_format("%u",body.length());
                WriteRaw(temp, strlen(temp));
                WriteRaw("\r\n\r\n", 4);
                WriteRaw(body.c_str(), body.length());
                std::cout<<"response:"<<body<<"\n";
            }
        private:
            uint64_t code_;
            string data_;


            inline const char* format_http_time_str(time_t tick = 0) {
                if (tick == 0) {
                    tick = time(NULL);
                }
                struct tm* timenow; //实例化tm结构指针

                timenow = localtime(&tick);
                char daytime[50];

                asctime_r(timenow, daytime);

                const std::string time = daytime;
                std::vector<std::string> date = base::utils::string_split(time,' ');
                if (date.size()!=5) {
                    return nullptr;
                }
                static char buff[64];
                sprintf(buff,"Date: %s, %s %s %s %s GMT", date[0].c_str(), date[2].c_str(), date[1].c_str(), date[3].c_str(),date[4].c_str());
                return buff;
            }
        };



        class HttpAcceptor;

        class HttpDispatcher
        {
        public:
            virtual void Process(HttpAcceptor* client, HttpRequest& request) = 0;

        };

        class HttpAcceptor : public net::Client
        {
        public:
            HttpAcceptor(memory::MemoryPool& mempool,HttpDispatcher& server);
            virtual ~HttpAcceptor();
            void SendResponse(HttpReponse& response);
            void SendResponse(string& response);

            void UpdateLastActiveTs();

            virtual void OnReceive(std::size_t count);
            virtual void OnConnect() {};
            virtual void OnConnectFail(int eno, const char* reason) {};
            virtual void OnClose() {};

            int64_t last_active_ts() {
                return last_active_ts_;
            }


            static int on_headers_complete_cb(http_parser* parser) {
                HttpAcceptor* client = static_cast<HttpAcceptor*>(parser->data);
                if (parser->method == 1) {
                    client->request.method = HttpMethod::GET;
                } else if (parser->method == 3) {
                    client->request.method = HttpMethod::POST;
                } else {
                    //  client->SendResponse();
                }
                return 0;
            }


            static int on_url_cb(http_parser* parser, const char* at, size_t len) {
                HttpAcceptor* client = static_cast<HttpAcceptor*>(parser->data);
                client->request.path.append(at,len);
                return 0;
            }
            static int on_message_begin_cb(http_parser* parser) {
                HttpAcceptor* client = static_cast<HttpAcceptor*>(parser->data);
                client->request.body.reserve(1024);
                return 0;
            }

            static int on_body_cb(http_parser* parser, const char* at, size_t len) {
                HttpAcceptor* client = static_cast<HttpAcceptor*>(parser->data);
                client->request.body.append(at, len);
                return 0;
            }

            static int on_message_complete_cb(http_parser* parser) {
                HttpAcceptor* client = static_cast<HttpAcceptor*>(parser->data);
                string url = base::utils::urldecode(client->request.path.c_str(),client->request.path.length());
                client->request.path.clear();
                size_t pathSize = url.find_first_of("?");
                if (pathSize == string::npos) {
                    client->request.path = url;
                    client->request.query.clear();
                } else {
                    client->request.path = url.substr(0,pathSize);
                    client->request.query = url.substr(pathSize+1,url.length()-pathSize-1);
                }

                client->Dispatch();
                return 0;
            }

        private:
            void Dispatch();

            HttpDispatcher& dispatcher;
            HttpRequest request;
            http_parser parser_;
            int64_t last_active_ts_;
        };


    }
}

#endif
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
