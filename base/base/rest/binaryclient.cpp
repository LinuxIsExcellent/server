#include "binaryclient.h"
#include "binaryserver.h"
#include "../logger.h"
#include "../event/dispatcher.h"
#include "../utils/crypto.h"
#include "../framework.h"
#include <cstdarg>

namespace base
{
    namespace rest
    {
        using namespace std;

        BinaryClient::BinaryClient(memory::MemoryPool& mempool,
                                   BinaryRestDispatcher& dispatcher, const string& auth_key)
            : Client(mempool), dispatcher_(dispatcher), last_active_ts_(0), auth_key_(auth_key)
        {
            UpdateLastActiveTs();
        }

        BinaryClient::~BinaryClient()
        {
        }

        void BinaryClient::UpdateLastActiveTs()
        {
            last_active_ts_ = base::event::Dispatcher::instance().GetTimestampCache();
        }

        void BinaryClient::OnConnect()
        {
        }

        void BinaryClient::OnClose()
        {
        }

        static bool check_if_match_half(const std::string& auth_key, const std::string& key)
        {
            size_t size = auth_key.length() / 2u;
            if (key.length() != size) {
                return false;
            }
            for (size_t i = 0; i < size; ++i) {
                if (auth_key[i] != key[i]) {
                    return false;
                }
            }
            return true;
        }

        void BinaryClient::OnReceive(std::size_t count)
        {
            while (count >= BinaryMessageIn::HEAD_SIZE) {
                uint32_t pktlen = 0u;
                CopyReceive((char*)&pktlen, 4);

                if (pktlen < BinaryMessageIn::HEAD_SIZE || pktlen > 1024 * 1024 * 5) {
                    LOG_WARN("the binary message size is too small or too big, size=%u\n", pktlen);
                    Close();
                    return;
                }
                if (pktlen >= BinaryMessageIn::HEAD_SIZE && count >= pktlen) {
                    gateway::packet_data_t data;
                    PopReceive(data, pktlen);
                    BinaryMessageIn msgin(data);
                    msgin.ReadHead();
                    try {
                        // 认证
                        switch (auth_pahse_) {
                            case AuthPhase::INIT: {
                                std::string key = msgin.ReadString();
                                if (!check_if_match_half(auth_key_, key)) {
                                    Close();
                                    return;
                                } else {
                                    auth_pahse_ = AuthPhase::CHALLENGE;
                                    auth_challenge_ = framework.random().GenRandomString(10);
                                    BinaryMessageOut msgout(0, 50, mempool());
                                    msgout.WriteString(auth_challenge_);
                                    Send(msgout);
                                }
                            }
                            break;
                            case AuthPhase::CHALLENGE: {
                                string key = msgin.ReadString();
                                if (base::utils::sha1hex(auth_key_ + auth_challenge_) == key) {
                                    auth_pahse_ = AuthPhase::OK;
                                } else {
                                    Close();
                                    return;
                                }
                            }
                            break;
                            case AuthPhase::OK:
                                dispatcher_.Process(this, msgin);
                                break;
                        }
                    } catch (exception& ex) {
                        LOG_ERROR("occurred exception when handle binary message: code=%u, %s\n", msgin.code(), ex.what());
                    }
                    count -= pktlen;
                } else {
                    break;
                }
            }
        }

        void BinaryClient::Send(BinaryMessageOut& msgout)
        {
            msgout.WriteHead();
            auto data = msgout.FetchData();
            PushSend(data);
        }

        void BinaryClient::SendError(int32_t rc, const char* msg, ...)
        {
            static char buff[512];
            va_list va;
            va_start(va, msg);
            vsnprintf(buff, 512, msg, va);
            va_end(va);
            BinaryMessageOut msgout(65535, 50, mempool());
            msgout.WriteInt32(rc);
            msgout.WriteString(buff);
            Send(msgout);
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
