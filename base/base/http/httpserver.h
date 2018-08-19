#ifndef BASE_HTTP_HTTPSERVER_H
#define BASE_HTTP_HTTPSERVER_H

#include <string>
#include <functional>
#include <unordered_map>
#include "../observer.h"
#include "constant.h"

namespace base
{
    namespace http
    {
        class Response;

        class Request
        {
        public:
            int session;

            std::string path;

            HttpMethod method;

            std::unordered_map<std::string, std::string> queries;

            std::unordered_map<std::string, std::string> headers;

            std::string body;

            void ParseQueryString(const char* str, uint16_t len);

            void Reset() {
                path.clear();
                queries.clear();
                headers.clear();
                body.clear();
            }
        };

        class HttpServerImpl;

        class HttpServer
        {
        public:
            HttpServer();
            virtual ~HttpServer();

            void Start(const char* ip, int port);

            void BeginCleanup(std::function<void()> cb);

            // 设置静态资源目录
            void SetResourcesDir(const char* dir);

            // 添加请求处理函数
            void AddHandler(const std::string& path, std::function<bool(const Request&)> handler, Observer* observer);

            // 添加默认处理函数
            void SetDefaultHandler(std::function<bool(const Request&)> handler, Observer* observer);

            // 发送响应
            void SendResponse(int session, const Response& response);

        private:
            HttpServerImpl* m_impl;
        };
    }
}

#endif // HTTPSERVER_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
