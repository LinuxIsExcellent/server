#include "packet.h"
#include "../../utils/utils_string.h"

namespace base
{
    namespace dbo
    {
        namespace internal
        {
            using namespace std;

            Packet::~Packet()
            {
            }

            std::string Packet::Dump() const
            {
                std::string txt;
                uint32_t total = size();
                packet_data_t::const_iterator it = data().begin();
                while (it != data().end() && total > 0) {
                    int acc = 0;
                    for (uint32_t i = 0; i < total && i < (*it).count(); ++i) {
                        const memory::RefMemoryChunk& ck = *it;
                        base::utils::string_append_format(txt, "%u,", (uint8_t)(*(ck.data() + i)));
                        ++acc;
                    }
                    total -= acc;
                    ++it;
                }
                return txt;
            }

            void PacketOut::AquireMemory(uint32_t need_bytes)
            {
                mempool_.AquireByByteNeed(data_, need_bytes + 10);
                UpdateCapacity();
            }

            void PacketOut::AppendData(const packet_data_t& data)
            {
                if (data.empty()) {
                    return;
                }

                uint32_t s = size();
                uint32_t posY = 0u;
                for (uint32_t i = 0u; i < data_.size(); ++i) {
                    memory::RefMemoryChunk& ck = data_[i];
                    if (s == 0) {
                        break;
                    }
                    ++posY;
                    if (s >= ck.count()) {
                        s -= ck.count();
                    } else {
                        ck.ShrinkCount(ck.count() - s);
                        s = 0;
                        break;
                    }
                }
                pos_y_ = posY;
                while (pos_y_ + 1 < data_.size()) {
                    data_.pop_back();
                }
                for (const memory::RefMemoryChunk & ck : data) {
                    data_.push_back(ck);
                    pos_ += ck.count();
                    pos_y_ += 1;
                }
                UpdateCapacity();
                UpdateSize();
            }
        }
    }
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
