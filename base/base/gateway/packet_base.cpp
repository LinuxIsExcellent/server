#include "packet_base.h"
#include "../memory/memorypool.h"
#include "../utils/utils_string.h"

namespace base
{
    namespace gateway
    {
        using namespace std;

        Packet::~Packet()
        {
        }

        std::string Packet::Dump(bool extInfo /* = false */) const
        {
            std::string txt;
            if (extInfo) {
                base::utils::string_append_format(txt, "Packet: size=%u, capacity=%u, posY=%u, data=", size_, capacity_, pos_y_);
            }
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
            if (extInfo) {
                txt.append("\n------------------------------------\n");
                for (const memory::RefMemoryChunk & ck : data_) {
                    txt.append(ck.DebugDump());
                    txt.append("\n");
                }
                txt.append("------------------------------------\n");
            }
            return txt;
        }

        void PacketOutBase::AquireMemory(uint32_t need_bytes)
        {
            mempool_.AquireByByteNeed(data_, need_bytes + 10);
            UpdateCapacity();
        }

        void PacketOutBase::WriteRaw(const packet_data_t& data)
        {
            uint32_t s = size();
            uint32_t posY = 0u;
            for (uint32_t i = 0u; i < data_.size(); ++i) {
                memory::RefMemoryChunk& ck = data_[i];
                if (s == 0) {
                    break;
                }
                posY = i;
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

        packet_data_t PacketInBase::ReadRaw(uint16_t count)
        {
            if (pos_ + count > size_) {
                throw Exception("Packet::ReadRaw no more data");
            }
            pos_ += count;
            packet_data_t ret;
            while (count > 0u && pos_y_ < data_.size()) {
                memory::RefMemoryChunk& ck = data_[pos_y_];
                if (count < ck.FreeCount()) {
                    ret.push_back(ck);
                    memory::RefMemoryChunk& ck1 = ret.back();
                    ck1.ForwardOffset(ck1.pos());
                    ck1.ShrinkCount(ck.FreeCount() - count);
                    count = 0u;
                } else {
                    ret.push_back(ck);
                    memory::RefMemoryChunk& ck1 = ret.back();
                    ck1.ForwardOffset(ck1.pos());
                    count -= ck.FreeCount();
                    ++pos_y_;
                }
            }
            return std::move(ret);
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
