#include "response.h"
#include "../gateway/packet_base.h"
#include "../utils/utils_string.h"
#include "../utils/file.h"
#include <string>

namespace base
{
    namespace http
    {
        using namespace std;

        Response::~Response()
        {
        }

        void Response::Flush(gateway::PacketOutBase& pktout) const
        {
            string tmp;

            pktout.SkipTo(0);

            tmp.clear();
            base::utils::string_append_format(tmp, "HTTP/1.1 %d %s\r\n", (int)m_statusCode, HTTP_STATUS_STRING(m_statusCode));
            pktout.WriteRaw(tmp.c_str(), tmp.length());

            for (const pair<string, string>& header : m_headers) {
                pktout.WriteRaw(header.first.c_str(), header.first.length());
                pktout.WriteRaw(": ", 2);
                pktout.WriteRaw(header.second.c_str(), header.second.length());
                pktout.WriteRaw("\r\n", 2);
            }

            tmp.clear();
            pktout.WriteRaw("Connection: keep-alive\r\n");

            tmp.clear();
            base::utils::string_append_format(tmp, "Content-Length: %llu\r\n", m_content.size());
            pktout.WriteRaw(tmp.c_str(), tmp.length());

            pktout.WriteRaw("\r\n", 2);
            pktout.WriteRaw(m_content.c_str(), m_content.size());
        }

        void FileResponse::Flush(gateway::PacketOutBase& pktout) const
        {
            //const char* name = basename(m_file.c_str());
            m_content = utils::file_get_content(m_file.c_str());
            // TODO 优化为循环分段读取，避免大文件占用内存过大

            base::http::Response::Flush(pktout);
        }

        Html404Response::Html404Response()
        {
            SetStatusCode(HttpStatusCode::Not_Found);
            static const char* defaultPage = "<html><head></head><body><h2>404 Not Found</h2></body></html>";
            AppendHtml(defaultPage);
        }

        Html500Response::Html500Response()
        {
            SetStatusCode(HttpStatusCode::Internal_Server_Error);
            static const char* defaultPage = "<html><head></head><body><h2>500 Internal Server Error</h2></body></html>";
            AppendHtml(defaultPage);
        }

        Html503Response::Html503Response()
        {
            SetStatusCode(HttpStatusCode::Service_Unavailable);
            static const char* defaultPage = "<html><head></head><body><h2>503 Service Unavailable</h2></body></html>";
            AppendHtml(defaultPage);
        }
    }
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
