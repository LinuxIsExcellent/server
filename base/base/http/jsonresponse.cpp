#include "jsonresponse.h"

namespace base
{
    namespace http
    {
        using namespace std;

        JsonResponse::~JsonResponse()
        {
        }

        void JsonResponse::Flush(gateway::PacketOutBase& pktout) const
        {
            const char* str = m_stringBuffer.GetString();
            size_t size = m_stringBuffer.GetSize();
            m_content.assign(str, size);

            base::http::Response::Flush(pktout);
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
