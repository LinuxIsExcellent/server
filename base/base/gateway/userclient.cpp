#include "userclient.h"
#include "../net/utils.h"
#include "../logger.h"
#include "usersession.h"
#include "../event/dispatcher.h"
#include "websocketheadparser.h"
#include "websocketframeparser.h"
#include "../memory/memorypool.h"

namespace base
{
    namespace gateway
    {
        using namespace std;

        UserClient::~UserClient()
        {
            SAFE_DELETE(websocket_head_parser_);
            SAFE_DELETE(websocket_frame_parser_);
        }

        bool UserClient::ProcessPacketIn(PacketIn& pktin)
        {
            try {
                pktin.ReadHead();
                if (session_) {
                    session_->ProcessUserClientReceivePacket(pktin);
                }
                return true;
            } catch (std::exception& ex) {
                LOG_ERROR("handle receive packet catch exception: %s, code=%u\n", ex.what(), pktin.code());
                Close();
                return false;
            }
        }
        static inline uint16_t myntohs(uint16_t n)
        {
            return ((n & 0xff00) >> 8) | ((n & 0x00ff) << 8);
        }

        void UserClient::Send(PacketOut& pktout)
        {
            pktout.WriteHead();
            std::vector<memory::RefMemoryChunk> data = pktout.FetchData();
            if (websocket_frame_parser_ != nullptr) {
                // write websocket head
                size_t size = pktout.size();
                base::gateway::PacketOutBase header(30, mempool());
                uint8_t opCode = (uint8_t)WebSocketFrameType::Binary;
                opCode |= 0x1 << 7;
                header.WriteUInt8(opCode);
                if (size <= 125) {
                    header.WriteUInt8(size);
                } else if (size <= 65535) {
                    header.WriteUInt8(126);
                    size = myntohs((uint16_t)size);
                    header.WriteUInt16(size);
                } else {
                    header.WriteUInt8(127);
                    header.WriteUInt64(size);
                }
                std::vector<memory::RefMemoryChunk> data0 = header.FetchData();
                PushSend(data0);
            }
            PushSend(data);
            //TODO orange Debug
            if (pktout.code() != 8) {
                //cout << "UserClient::Send pktout code = " << pktout.code() << " size = " << pktout.size() << endl;
            }

            if (session_) {
                session_->OnUserSessionSend(pktout);
            }
        }

        void UserClient::OnReceive(std::size_t count)
        {
            if (websocket_frame_parser_ != nullptr) {
                websocket_frame_parser_->OnReceive(count);
                return;
            }

            while (count >= PacketIn::HEAD_SIZE) {
                CopyReceive(pkthead_.data, sizeof(pkthead_));
                if ((pkthead_.head.identifier == PacketIn::IDENTIFIER || pkthead_.head.identifier == PacketOut::IDENTIFIER)
                        && pkthead_.head.length >= PacketIn::HEAD_SIZE) {
                    if (count >= pkthead_.head.length) {
                        std::vector<memory::RefMemoryChunk> data;
                        PopReceive(data, pkthead_.head.length);
                        PacketIn pktin(data);
                        if (ProcessPacketIn(pktin)) {
                            count -= pkthead_.head.length;
                        } else {
                            break;
                        }
                    } else {
                        break;
                    }
                } else {
                    char buff[PacketIn::HEAD_SIZE];
                    CopyReceive(buff, PacketIn::HEAD_SIZE);
                    if (strncmp(buff, "GET /gateway", PacketIn::HEAD_SIZE) == 0 && websocket_head_parser_ == nullptr && websocket_frame_parser_ == nullptr) {
                        websocket_head_parser_ = new WebSocketHeadParser();
                        // WebSocket
                        std::vector<memory::RefMemoryChunk> data;
                        PopReceive(data, count);
                        websocket_head_parser_->ParseData(data);
                        if (websocket_head_parser_->error()) {
                            LOG_ERROR("parse websocket head fail");
                            Close();
                        } else if (websocket_head_parser_->finish()) {
                            // send handshake
                            PacketOutBase pktout(250, mempool());
                            pktout.WriteRaw("HTTP/1.1 101 Switching Protocols\r\n");
                            pktout.WriteRaw("Upgrade: WebSocket\r\n");
                            pktout.WriteRaw("Connection: Upgrade\r\n");
                            string accptedKey = websocket_head_parser_->CalculateSecKey();
                            if (!accptedKey.empty()) {
                                pktout.WriteRaw("Sec-WebSocket-Accept: ");
                                accptedKey.append("\r\n");
                                pktout.WriteRaw(accptedKey.c_str(), accptedKey.length());
                            }
                            pktout.WriteRaw("\r\n");
                            packet_data_t response = pktout.FetchData();
                            PushSend(response);
                            websocket_frame_parser_ = new WebSocketFrameParser(*this);
                        }
                        count = 0;
                    } else {
                        if (strncmp(buff, "GET / HTTP", PacketIn::HEAD_SIZE) != 0 && strncmp(buff, "GET /favicon", PacketIn::HEAD_SIZE) != 0 && strncmp(buff, "@PJL INFO", 5) != 0) {
                            std::string text = net::dump_raw_data(buff, PacketIn::HEAD_SIZE);
                            LOG_ERROR("detected broken packet from %s:%d: count=%u, identifier=%u, length=%u, data=%s\n",
                                      ipaddr().c_str(), port(), count, pkthead_.head.identifier, pkthead_.head.length, text.c_str());
                        }
                        Close();
                    }
                    break;
                }
            }
        }

        void UserClient::OnConnect()
        {
            connect_ts_ = g_dispatcher->GetTimestampCache();
            event_handler_.OnUserClientConnect(this);
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
