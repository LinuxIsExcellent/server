#include "websocketframeparser.h"
#include "../logger.h"
#include "../net/utils.h"

namespace base
{
    namespace gateway
    {
        using namespace std;

        union FragementLength16 {
            char buffer[2];
            uint16_t len;
        };

        union FragementLength64 {
            char buffer[8];
            uint64_t len;
        };

        void WebSocketFrameParser::OnReceive(size_t count)
        {
            while (count > 10u) {
                uint32_t headerSize = 0u;
                char buffer[10];
                m_client.CopyReceive(buffer, 10);
                WebSocketFrameType opCode = (WebSocketFrameType)((int)buffer[0] & 0xf);
                bool isFin = (buffer[0] >> 7) & 0x01;
                bool isMasked = (buffer[1] >> 7) & 0x01;

                size_t payloadLength = 0;
                int lengthField = buffer[1] & 0x7f;
                char mask[4];

                headerSize += 2;

                if (lengthField <= 125) {
                    payloadLength = lengthField;
                } else if (lengthField == 126) {
                    FragementLength16 f;
                    f.buffer[0] = buffer[3];
                    f.buffer[1] = buffer[2];
                    payloadLength = f.len;
                    headerSize += 2;
                } else if (lengthField == 127) {
                    FragementLength64 f;
                    for (int i = 0; i < 8; ++i) {
                        f.buffer[i] = buffer[9 - i];
                    }
                    payloadLength = f.len;
                    headerSize += 8;
                } else {
                    m_client.Close();
                    LOG_ERROR("bad websocket payload length");
                    break;
                }

                if (isMasked) {
                    if (count > headerSize + 4u) {
                        for (int i = 0; i < 4; ++i) {
                            mask[i] = buffer[headerSize + i];
                        }
                        headerSize += 4;
                    } else {
                        break;
                    }
                }

                //cout << "receive=" << count << ", got websocket frame: opCode=" << (int)opCode << ", payloadLength=" << payloadLength << ", isFin=" << boolalpha << isFin << endl;

                if (!isFin) {
                    m_client.Close();
                    LOG_ERROR("can not handle not isFin frame");
                    break;
                }

                if (opCode == WebSocketFrameType::Text) {
                    m_client.Close();
                    LOG_ERROR("unsupported Text protocol");
                    break;
                }
                if (opCode == WebSocketFrameType::ConnectionClose) {
                    m_client.Close();
                    break;
                }

                if (payloadLength + headerSize > count) {
                    break;
                } else {
                    vector<memory::RefMemoryChunk> data;
                    size_t r = m_client.PopReceive(data, headerSize);
                    assert(r == headerSize);
                    count -= headerSize;
                    if (opCode == WebSocketFrameType::Ping || opCode == WebSocketFrameType::Pong) {
                        continue;
                    } else {
                        data.clear();
                        size_t r = m_client.PopReceive(data, payloadLength);
                        assert(r == payloadLength);
                        count -= payloadLength;
                        // unmask
                        size_t payloadIdx = 0u;
                        for (memory::RefMemoryChunk & ck : data) {
                            for (uint32_t i = ck.pos(); i < ck.count(); ++i) {
                                char* c = ck.data() + i;
                                *c = *c ^ mask[payloadIdx % 4];
                                ++payloadIdx;
                            }
                            recvq_.push_back(ck);
                            recvq_count_ += ck.FreeCount();
                        }
                    }
                }
            }

            // parse payload
            while (recvq_count_ >= PacketIn::HEAD_SIZE) {
                CopyReceive(pkthead_.data, sizeof(pkthead_));
                /*
                string debug =  net::dump_raw_data(pkthead_.data, 4);
                cout << "debug head=" << debug << endl;
                */
                if ((pkthead_.head.identifier == PacketIn::IDENTIFIER)
                        && pkthead_.head.length >= PacketIn::HEAD_SIZE) {
                    if (recvq_count_ >= pkthead_.head.length) {
                        std::vector<memory::RefMemoryChunk> data;
                        PopReceive(data, pkthead_.head.length);
                        PacketIn pktin(data);
                        if (m_client.ProcessPacketIn(pktin)) {
                            ;
                        } else {
                            break;
                        }
                    } else {
                        cout << "packet not enough.." << recvq_count_ << ", " << pkthead_.head.length << endl;
                        char buff[PacketIn::HEAD_SIZE];
                        CopyReceive(buff, PacketIn::HEAD_SIZE);
                        std::string text = net::dump_raw_data(buff, PacketIn::HEAD_SIZE);
                        cout << text << endl;
                        break;
                    }
                } else {
                    char buff[PacketIn::HEAD_SIZE];
                    CopyReceive(buff, PacketIn::HEAD_SIZE);
                    std::string text = net::dump_raw_data(buff, PacketIn::HEAD_SIZE);
                    LOG_ERROR("detected broken packet from %s:%d: count=%u, identifier=%u, length=%u, data=%s\n",
                              m_client.ipaddr().c_str(), m_client.port(), recvq_count_, pkthead_.head.identifier, pkthead_.head.length, text.c_str());
                    m_client.Close();
                    break;
                }
            }
        }

        size_t WebSocketFrameParser::CopyReceive(char* dst, size_t count)
        {
            size_t total = 0;
            std::deque<memory::RefMemoryChunk>::iterator it = recvq_.begin();
            while (it != recvq_.end() && count > 0) {
                uint32_t rd = (*it).Read(dst + total, count);
                (*it).Reset();
                count -= rd;
                total += rd;
                ++it;
            }
            return total;
        }

        size_t WebSocketFrameParser::PopReceive(vector< memory::RefMemoryChunk >& dst, size_t count)
        {
            size_t total = 0;
            while (!recvq_.empty() && count > 0) {
                memory::RefMemoryChunk& ck = recvq_.front();
                if (count >= ck.count()) {
                    dst.push_back(ck);
                    total += ck.count();
                    count -= ck.count();
                    recvq_.pop_front();
                } else {
                    dst.push_back(ck);
                    dst.back().ShrinkCount(ck.count() - count);
                    ck.ForwardOffset(count);
                    total += count;
                    count = 0;
                }
            }
            recvq_count_ -= total;
            return total;
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
