#ifndef BASE_GATEWAY_WEBSOCKETHEADERPARSER_H
#define BASE_GATEWAY_WEBSOCKETHEADERPARSER_H

#include "../memory/memorychunk.h"
#include <unordered_map>
#include <vector>

namespace base
{
    namespace gateway
    {
        class WebSocketHeadParser
        {
        public:
            WebSocketHeadParser();
            ~WebSocketHeadParser();

            bool finish() const {
                return m_state == State::COMPLETED;
            }

            bool error() const {
                return m_state == State::ERROR;
            }

            void ParseData(const  std::vector<base::memory::RefMemoryChunk>& data);

            std::string CalculateSecKey();

        private:

            void UpdateSize() {
                m_size = 0;
                for (std::vector<base::memory::RefMemoryChunk>::iterator it = m_data.begin(); it != m_data.end(); ++it) {
                    m_size += (*it).count();
                }
            }

            void SkipTo(uint32_t pos) {
                assert(pos <= m_size);
                m_pos = pos;
                UpdateSize();
                m_posY = 0;
                for (std::vector<base::memory::RefMemoryChunk>::iterator it = m_data.begin(); it != m_data.end(); ++it) {
                    if (pos == 0) {
                        (*it).SkipTo(pos);
                    } else {
                        if (pos < (*it).count()) {
                            (*it).SkipTo(pos);
                            pos = 0;
                        } else {
                            (*it).SkipTo((*it).count());
                            pos -= (*it).count();
                            ++m_posY;
                        }
                    }
                }
            }

            bool GetByte(char* dst, uint32_t count) {
                if (m_pos + count > m_size) {
                    return false;
                }
                m_pos += count;
                while (count > 0 && m_posY < m_data.size()) {
                    uint32_t readed = m_data[m_posY].Read(dst, count);
                    count -= readed;
                    dst += readed;
                    if (m_data[m_posY].FreeCount() == 0) {
                        ++m_posY;
                    }
                }
                return true;
            }

            enum class State
            {
                DATA,
                LB_R1,
                LB_N1,
                LB_R2,
                COMPLETED,
                ERROR,
            };
            std::string m_tmpLine;
            State m_state = State::DATA;

            std::string m_path;
            std::unordered_map<std::string, std::string> m_headers;

            uint32_t m_size = 0u;
            uint32_t m_pos = 0u;
            uint32_t m_posY = 0u;;
            std::vector<base::memory::RefMemoryChunk> m_data;
        };
    }
}

#endif // WEBSOCKETHEADERPARSER_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
