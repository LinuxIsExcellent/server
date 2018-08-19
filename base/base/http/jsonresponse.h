#ifndef BASE_HTTP_JSONRESPONSE_H
#define BASE_HTTP_JSONRESPONSE_H

#include "../serialize/jsonstream.h"
#include "response.h"

namespace base
{
    namespace http
    {
        class JsonResponse : public Response
        {
        public:
            JsonResponse() : m_jsonWriter(m_stringBuffer) {
                AddHeader("Content-Type", "application/json; charset=utf-8");
            }

            virtual ~JsonResponse();

            rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter() {
                return m_jsonWriter;
            }

            virtual void Flush(gateway::PacketOutBase& pktout) const override;

        private:
            rapidjson::StringBuffer m_stringBuffer;
            rapidjson::Writer<rapidjson::StringBuffer> m_jsonWriter;
        };

    }
}

#endif // JSONRESPONSE_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
