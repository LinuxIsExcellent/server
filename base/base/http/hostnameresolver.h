#ifndef BASE_HTTP_HOSTNAMERESOLVER_H
#define BASE_HTTP_HOSTNAMERESOLVER_H

#include "../thread/task.h"
#include "../callback.h"
#include "httpclient.h"

namespace base
{
    namespace http
    {
        class HostnameResolver : public thread::AsyncTask
        {
        public:
            HostnameResolver(const std::string& host, const CallbackObserver<void(const DnsRecord&)>& cb);

        private:
            virtual void doInWorkThread();
            virtual void onPostExecute();

            std::string host_;
            DnsRecord result_;
            base::CallbackObserver<void(const DnsRecord&)> cb_;
        };
    }
}

#endif // HOSTNAMERESOLVER_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
