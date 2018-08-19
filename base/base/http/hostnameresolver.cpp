#include "hostnameresolver.h"
#include "../event/dispatcher.h"
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>

namespace base
{
    namespace http
    {
        using namespace std;

        HostnameResolver::HostnameResolver(const std::string& host, const CallbackObserver< void(const DnsRecord&)>& cb)
            : host_(host), cb_(cb)
        {
            result_.hostname = host_;
        }

        void HostnameResolver::doInWorkThread()
        {
#ifdef __APPLE__
            hostent* he = gethostbyname(host_.c_str());
#else
            struct hostent hbuf;
            size_t buflen = 256u;
            char* buf = (char*)malloc(buflen);

            int error = 0;
            int rc = 0;
            struct hostent* he;

            while ((rc = gethostbyname_r(host_.c_str(), &hbuf, buf, buflen, &he, &error)) == ERANGE) {
                buflen *= 2u;
                buf = (char*)realloc(buf, buflen);
            }
#endif

            if (he != NULL) {
                struct in_addr** addr_list = (struct in_addr**) he->h_addr_list;
                for (int i = 0; addr_list[i] != NULL; ++i) {
                    string ip(inet_ntoa(*addr_list[i]));
                    result_.ipList.emplace_back(std::move(ip));
                }
                if (!result_.empty()) {
                    result_.createTs = g_dispatcher->GetTimestampCache();
                }
            }
#ifndef __APPLE__
            free(buf);
#endif
        }

        void HostnameResolver::onPostExecute()
        {
            cb_(result_);
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
