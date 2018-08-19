#include "httpacceptor.h"
#include "../event/dispatcher.h"


namespace base
{
    namespace http
    {
        using namespace std;

        static struct http_parser_settings serverSettings;

        static struct http_parser_settings_initializer {
            http_parser_settings_initializer() {
                memset(&serverSettings, 0, sizeof(serverSettings));
                serverSettings.on_url = HttpAcceptor::on_url_cb;
                serverSettings.on_headers_complete = HttpAcceptor::on_headers_complete_cb;
                serverSettings.on_message_complete = HttpAcceptor::on_message_complete_cb;
                serverSettings.on_message_begin = HttpAcceptor::on_message_begin_cb;
                serverSettings.on_body = HttpAcceptor::on_body_cb;
            }
        } _settings_initializer;

        HttpAcceptor::HttpAcceptor(memory::MemoryPool& mempool, HttpDispatcher& disp): Client(mempool), dispatcher(disp)
        {
            UpdateLastActiveTs();
            http_parser_init(&parser_, HTTP_REQUEST);
            parser_.data = this;
        }
        HttpAcceptor::~HttpAcceptor()
        {
        }

        void HttpAcceptor::UpdateLastActiveTs()
        {
            last_active_ts_ = base::event::Dispatcher::instance().GetTimestampCache();
        }


        void HttpAcceptor::SendResponse(HttpReponse& response)
        {
            auto data = response.Fetch();
            PushSend(data);
        }

        void HttpAcceptor::SendResponse(string& body)
        {
            HttpReponse response(200,400,mempool());
            response.WriteResponse(body);
            SendResponse(response);
        }


        void HttpAcceptor::OnReceive(std::size_t count)
        {
            vector<memory::RefMemoryChunk> data;
            PopReceive(data, count);
            for (vector<memory::RefMemoryChunk>::iterator it = data.begin(); it != data.end(); ++it) {
                size_t r = http_parser_execute(&parser_, &serverSettings, (*it).data(), (*it).count());
                if (r != (*it).count()) {
                    return;
                }
            }
        }

        void HttpAcceptor::Dispatch()
        {
            dispatcher.Process(this,request);
        }


    }


}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
