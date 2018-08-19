#ifndef BASE_GATEWAY_PACKET_BASE_H
#define BASE_GATEWAY_PACKET_BASE_H

#include "../global.h"
#include "../memory/memorychunk.h"
#include "../exception.h"
#include <vector>
#include <list>
#include <array>
#include <string>
#include <limits>

namespace base
{
    namespace gateway
    {
        typedef std::vector<memory::RefMemoryChunk> packet_data_t;

        // 数据封包
        class Packet
        {
        public:
            DISABLE_COPY(Packet)
            Packet() : size_(0), pos_(0), pos_y_(0), capacity_(0) {}
            virtual ~Packet();

            uint32_t size() const {
                return size_;
            }
            uint32_t capacity() const {
                return capacity_;
            }
            uint32_t pos() const {
                return pos_;
            }
            const std::vector<memory::RefMemoryChunk>& data() const {
                return data_;
            }

            // 跳转到指定光标
            void SkipTo(uint32_t pos) {
                assert(pos <= capacity_);
                pos_ = pos;
                UpdateSize();
                pos_y_ = 0;
                for (packet_data_t::iterator it = data_.begin(); it != data_.end(); ++it) {
                    if (pos == 0) {
                        (*it).SkipTo(pos);
                    } else {
                        if (pos < (*it).count()) {
                            (*it).SkipTo(pos);
                            pos = 0;
                        } else {
                            (*it).SkipTo((*it).count());
                            pos -= (*it).count();
                            ++pos_y_;
                        }
                    }
                }
            }

            std::string Dump(bool extInfo = false) const;

        protected:
            packet_data_t data_;        // 数据
            uint32_t size_;             // 实际数据量
            uint32_t pos_;              // 光标
            uint32_t pos_y_;            // 光标y
            uint32_t capacity_;         // 容量

            // 更新实际数据量
            inline void UpdateSize() {
                if (size_ < pos_) {
                    size_ = pos_;
                }
            }
            // 更新容量
            inline void UpdateCapacity() {
                capacity_ = 0;
                for (packet_data_t::iterator it = data_.begin(); it != data_.end(); ++it) {
                    capacity_ += (*it).count();
                }
            }
        };

        // 只读的协议包
        class PacketInBase : public Packet
        {
        public:
            DISABLE_COPY(PacketInBase)
            PacketInBase(packet_data_t& data)
                : Packet() {
                data_.swap(data);
                UpdateCapacity();
                size_ = capacity();
                SkipTo(0);
            }
            virtual ~PacketInBase() {}

            bool ReadBoolean() {
                return ReadUInt8() == 1;
            }

            int8_t ReadInt8() {
                int8_t ret;
                GetByte(reinterpret_cast<char*>(&ret), 1);
                return ret;
            }

            uint8_t ReadUInt8() {
                uint8_t ret;
                GetByte(reinterpret_cast<char*>(&ret), 1);
                return ret;
            }

            int16_t ReadInt16() {
                int16_t ret;
                GetByte(reinterpret_cast<char*>(&ret), 2);
                return ret;
            }

            uint16_t ReadUInt16() {
                uint16_t ret;
                GetByte(reinterpret_cast<char*>(&ret), 2);
                return ret;
            }

            int32_t ReadInt32() {
                int32_t ret;
                GetByte(reinterpret_cast<char*>(&ret), 4);
                return ret;
            }

            uint32_t ReadUInt32() {
                uint32_t ret;
                GetByte(reinterpret_cast<char*>(&ret), 4);
                return ret;
            }

            int64_t ReadInt64() {
                int64_t ret;
                GetByte(reinterpret_cast<char*>(&ret), 8);
                return ret;
            }

            uint64_t ReadUInt64() {
                uint64_t ret;
                GetByte(reinterpret_cast<char*>(&ret), 8);
                return ret;
            }

            float ReadFloat() {
                float ret;
                GetByte(reinterpret_cast<char*>(&ret), 4);
                return ret;
            }

            double ReadDouble() {
                double ret;
                GetByte(reinterpret_cast<char*>(&ret), 8);
                return ret;
            }

            void ReadString(std::string& str) {
                uint16_t strlen = 0u;
                ReadVarInteger(&strlen);
                str.resize(strlen);
                char* dst = const_cast<char*>(str.c_str());
                GetByte(dst, strlen);
            }

            PacketInBase& operator >> (bool& v) {
                uint8_t b;
                (*this) >> b;
                v = (b == 1);
                return *this;
            }

            PacketInBase& operator >> (int8_t& v) {
                ReadVarInteger(&v);
                return *this;
            }

            PacketInBase& operator >> (uint8_t& v) {
                ReadVarInteger(&v);
                return *this;
            }

            PacketInBase& operator >> (int16_t& v) {
                ReadVarInteger(&v);
                return *this;
            }

            PacketInBase& operator >> (uint16_t& v) {
                ReadVarInteger(&v);
                return *this;
            }

            PacketInBase& operator >> (int32_t& v) {
                ReadVarInteger(&v);
                return *this;
            }

            PacketInBase& operator >> (uint32_t& v) {
                ReadVarInteger(&v);
                return *this;
            }

            PacketInBase& operator >> (int64_t& v) {
                ReadVarInteger(&v);
                return *this;
            }

            PacketInBase& operator >> (uint64_t& v) {
                ReadVarInteger(&v);
                return *this;
            }

            PacketInBase& operator >> (float& v) {
                GetByte(reinterpret_cast<char*>(&v), 4);
                return *this;
            }

            PacketInBase& operator >> (double& v) {
                GetByte(reinterpret_cast<char*>(&v), 8);
                return *this;
            }

            PacketInBase& operator >> (std::string& v) {
                uint16_t len;
                ReadVarInteger(&len);
                v.resize(len);
                GetByte(const_cast<char*>(v.c_str()), len);
                return *this;
            }

            template<typename T>
            typename std::enable_if < !std::is_enum<T>::value, PacketInBase& >::type
            operator >> (T& v) {
                v.ReadFromPacketIn(*this);
                return *this;
            }

            template<typename T>
            typename std::enable_if<std::is_enum<T>::value, PacketInBase&>::type
            operator >> (T& v) {
                int vi = ReadVarInteger<int>();
                v = (T)vi;
                return *this;
            }

            template<typename T>
            void ReadVarInteger(T* v) {
                uint8_t first = 0;
                GetByte(reinterpret_cast<char*>(&first), 1);
                switch (first) {
                    case 255:
                        *v = ReadInt8();
                        break;
                    case 254:
                        *v = ReadInt16();
                        break;
                    case 253:
                        *v = ReadUInt16();
                        break;
                    case 252:
                        *v = ReadInt32();
                        break;
                    case 251:
                        *v = ReadUInt32();
                        break;
                    case 250:
                        *v = ReadInt64();
                        break;
                    case 249:
                        *v = ReadUInt64();
                        break;
                    default:
                        *v = first;
                        break;
                }
            }

            template<typename T>
            PacketInBase& operator >> (std::vector<T>& list) {
                std::size_t size = ReadVarInteger<std::size_t>();
                for (std::size_t i = 0u; i < size; ++i) {
                    list.emplace_back();
                    *this >> list.back();
                }
                return *this;
            }

            template<typename T>
            PacketInBase& operator >> (std::vector<T*>& list) {
                std::size_t size = ReadVarInteger<std::size_t>();
                for (std::size_t i = 0u; i < size; ++i) {
                    T* o = new T();
                    *this >> *o;
                    list.push_back(o);
                }
                return *this;
            }

            template<typename T>
            PacketInBase& operator >> (std::list<T>& list) {
                std::size_t size = ReadVarInteger<std::size_t>();
                for (std::size_t i = 0u; i < size; ++i) {
                    list.emplace_back();
                    *this >> list.back();
                }
                return *this;
            }

            template<typename T>
            T ReadVarInteger() {
                T v;
                ReadVarInteger<T>(&v);
                return v;
            }

            std::string ReadString() {
                std::string str;
                ReadString(str);
                return std::move(str);
            }

            void ReadBits(std::vector<char>& dst) {
                uint16_t bitslen = 0u;
                ReadVarInteger(&bitslen);
                dst.resize(bitslen);
                GetByte(dst.data(), bitslen);
            }

            void ReadRaw(std::vector<char>& dst, uint16_t size) {
                dst.resize(size);
                GetByte(dst.data(), size);
            }

            packet_data_t ReadRaw(uint16_t count);

        private:
            void GetByte(char* dst, uint32_t count) {
                if (pos_ + count > size_) {
                    throw Exception("Packet::GetByte no more data");
                }
                pos_ += count;
                while (count > 0 && pos_y_ < data_.size()) {
                    uint32_t readed = data_[pos_y_].Read(dst, count);
                    count -= readed;
                    dst += readed;
                    if (data_[pos_y_].FreeCount() == 0) {
                        ++pos_y_;
                    }
                }
            }
        };

        // 只写的协议包
        class PacketOutBase : public Packet
        {
        public:
            DISABLE_COPY(PacketOutBase)
            PacketOutBase(uint32_t approx_size, memory::MemoryPool& mempool)
                : mempool_(mempool) {
                AquireMemory(approx_size);
            }
            PacketOutBase(packet_data_t& data, memory::MemoryPool& mempool): mempool_(mempool) {
                data_ = data;
                for (memory::RefMemoryChunk & ck : data_) {
                    size_ += ck.count();
                }
                pos_y_ = data.size();
                pos_ = size_;
            }

            void WriteBoolean(bool v) {
                if (v) {
                    WriteInt8(1);
                } else {
                    WriteInt8(0);
                }
            }

            void WriteInt8(int8_t v) {
                PutByte(reinterpret_cast<const char*>(&v), 1);
            }

            void WriteUInt8(uint8_t v) {
                PutByte(reinterpret_cast<const char*>(&v), 1);
            }

            void WriteInt16(int16_t v) {
                PutByte(reinterpret_cast<const char*>(&v), 2);
            }

            void WriteUInt16(uint16_t v) {
                PutByte(reinterpret_cast<const char*>(&v), 2);
            }

            void WriteInt32(int32_t v) {
                PutByte(reinterpret_cast<const char*>(&v), 4);
            }

            void WriteUInt32(uint32_t v) {
                PutByte(reinterpret_cast<const char*>(&v), 4);
            }

            void WriteInt64(int64_t v) {
                PutByte(reinterpret_cast<const char*>(&v), 8);
            }

            void WriteUInt64(uint64_t v) {
                PutByte(reinterpret_cast<const char*>(&v), 8);
            }

            void WriteFloat(float v) {
                PutByte(reinterpret_cast<const char*>(&v), 4);
            }

            void WriteDouble(double v) {
                PutByte(reinterpret_cast<const char*>(&v), 8);
            }

            void WriteString(const char* str) {
                uint16_t len = strlen(str);
                WriteVarInteger(len);
                PutByte(str, len);
            }

            void WriteString(const char* str, std::size_t len) {
                WriteVarInteger(len);
                PutByte(str, len);
            }

            void WriteString(const std::string& str) {
                WriteVarInteger(str.length());
                PutByte(str.c_str(), str.length());
            }

            void WriteBits(const char* v, uint16_t len) {
                WriteVarInteger(len);
                PutByte(v, len);
            }

            void WriteRaw(const char* str) {
                PutByte(str, strlen(str));
            }
            void WriteRaw(const char* data, uint16_t size) {
                PutByte(data, size);
            }
            void WriteRaw(const packet_data_t& data);

            template<typename T>
            typename std::enable_if<std::is_unsigned<T>::value>::type
            WriteVarInteger(const T& v) {
                if (v < 249ul) {
                    WriteUInt8(v);
                } else if (v <= std::numeric_limits<uint16_t>::max()) {
                    WriteUInt8(253);
                    WriteUInt16(v);
                } else if (v <= std::numeric_limits<uint32_t>::max()) {
                    WriteUInt8(251);
                    WriteUInt32(v);
                } else {
                    WriteUInt8(249);
                    WriteUInt64(v);
                }
            }

            template<typename T>
            typename std::enable_if < !std::is_unsigned<T>::value >::type
            WriteVarInteger(const T& v) {
                if (v >= 0) {
                    if (v < 249) {
                        WriteUInt8(v);
                    } else if (v <= std::numeric_limits<int16_t>::max()) {
                        WriteUInt8(254);
                        WriteInt16(v);
                    } else if (v <= std::numeric_limits<int32_t>::max()) {
                        WriteUInt8(252);
                        WriteInt32(v);
                    } else {
                        WriteUInt8(250);
                        WriteInt64(v);
                    }
                } else {
                    if (v > std::numeric_limits<int8_t>::min()) {
                        WriteUInt8(255);
                        WriteInt8(v);
                    } else if (v >= std::numeric_limits<int16_t>::min()) {
                        WriteUInt8(254);
                        WriteInt16(v);
                    } else if (v >= std::numeric_limits<int32_t>::min()) {
                        WriteUInt8(252);
                        WriteInt32(v);
                    } else {
                        WriteUInt8(250);
                        WriteInt64(v);
                    }
                }
            }

            PacketOutBase& operator << (bool v) {
                WriteBoolean(v);
                return *this;
            }

            PacketOutBase& operator << (int8_t v) {
                WriteVarInteger(v);
                return *this;
            }

            PacketOutBase& operator << (uint8_t v) {
                WriteVarInteger(v);
                return *this;
            }

            PacketOutBase& operator << (int16_t v) {
                WriteVarInteger(v);
                return *this;
            }

            PacketOutBase& operator << (uint16_t v) {
                WriteVarInteger(v);
                return *this;
            }

            PacketOutBase& operator << (int32_t v) {
                WriteVarInteger(v);
                return *this;
            }

            PacketOutBase& operator << (uint32_t v) {
                WriteVarInteger(v);
                return *this;
            }

            PacketOutBase& operator << (int64_t v) {
                WriteVarInteger(v);
                return *this;
            }

            PacketOutBase& operator << (uint64_t v) {
                WriteVarInteger(v);
                return *this;
            }

#ifdef __APPLE__
            PacketOutBase& operator << (unsigned long int v) {
                WriteVarInteger(v);
                return *this;
            }

            PacketOutBase& operator << (long int v) {
                WriteVarInteger(v);
                return *this;
            }
#endif

            PacketOutBase& operator << (float v) {
                WriteFloat(v);
                return *this;
            }

            PacketOutBase& operator << (double v) {
                WriteDouble(v);
                return *this;
            }

            PacketOutBase& operator << (const char* v) {
                WriteString(v);
                return *this;
            }

            PacketOutBase& operator << (const std::string& v) {
                WriteString(v);
                return *this;
            }

            template<typename T>
            PacketOutBase& operator << (const std::vector<T>& list) {
                WriteVarInteger(list.size());
                for (typename std::vector<T>::const_iterator it = list.begin(); it != list.end(); ++it) {
                    *this << *it;
                }
                return *this;
            }

            template<typename T, std::size_t N>
            PacketOutBase& operator << (const std::array<T, N>& list) {
                WriteVarInteger(list.size());
                for (typename std::array<T, N>::const_iterator it = list.begin(); it != list.end(); ++it) {
                    *this << *it;
                }
                return *this;
            }

            template<typename T>
            PacketOutBase& operator << (const std::list<T>& list) {
                WriteVarInteger(list.size());
                for (typename std::list<T>::const_iterator it = list.begin(); it != list.end(); ++it) {
                    *this << *it;
                }
                return *this;
            }

            template<typename T>
            typename std::enable_if<std::is_enum<T>::value, PacketOutBase&>::type
            operator << (const T& v) {
                WriteVarInteger((int)v);
                return *this;
            }

            template<typename T>
            PacketOutBase& operator << (const T* v) {
                v->WriteToPacketOut(*this);
                return *this;
            }

            template<typename T>
            PacketOutBase& operator << (T* v) {
                ((const T*)v)->WriteToPacketOut(*this);
                return *this;
            }

            template<typename T>
            typename std::enable_if < !std::is_enum<T>::value, PacketOutBase& >::type
            //PacketOutBase&
            operator << (const T& v) {
                v.WriteToPacketOut(*this);
                return *this;
            }

        public:
            packet_data_t FetchData() {
                packet_data_t ret;
                uint32_t s = size();
                packet_data_t::iterator it = data_.begin();
                while (it != data_.end() && s > 0) {
                    memory::RefMemoryChunk& ck = (*it);
                    if (s < ck.count()) {
                        ret.push_back(ck);
                        ret.back().ShrinkCount(ck.count() - s);
                        s = 0;
                    } else {
                        ret.push_back(ck);
                        s -= ck.count();
                    }
                    ++it;
                }
                return std::move(ret);
            }

        private:
            void PutByte(const char* src, uint32_t count) {
                while (pos_ + count > capacity_) {
                    AquireMemory(count);
                }
                pos_ += count;
                UpdateSize();
                while (count > 0 && pos_y_ < data_.size()) {
                    uint32_t writed = data_[pos_y_].Write(src, count);
                    count -= writed;
                    src += writed;
                    if (data_[pos_y_].FreeCount() == 0) {
                        ++pos_y_;
                    }
                }
            }
            void AquireMemory(uint32_t need_bytes);
            memory::MemoryPool& mempool_;
        };

        ///
        /// serialize helper function and macro
        ///

        template<typename A>
        inline void MultiWrite(base::gateway::PacketOutBase& pktout, const A& a)
        {
            pktout << a;
        }

        template<typename A, typename...Args>
        inline void MultiWrite(base::gateway::PacketOutBase& pktout, const A& a, const Args& ... args)
        {
            pktout << a;
            MultiWrite(pktout, args...);
        }

        template<typename A>
        inline void MultiRead(base::gateway::PacketInBase& pktin, A& a)
        {
            pktin >> a;
        }

        template<typename A, typename...Args>
        inline void MultiRead(base::gateway::PacketInBase& pktin, A& a, Args& ...args)
        {
            pktin >> a;
            MultiRead(pktin, args...);
        }

#define PACKET_DEFINE_OUT(...) \
    public: \
    virtual void WriteToPacketOut(base::gateway::PacketOutBase& pktout) const {\
        base::gateway::MultiWrite(pktout, __VA_ARGS__);\
    }

#define PACKET_DEFINE_IN(...) \
    public: \
    virtual void ReadFromPacketIn(base::gateway::PacketInBase& pktin) {\
        base::gateway::MultiRead(pktin, __VA_ARGS__);\
    }

#define PACKET_DEFINE(...) \
    PACKET_DEFINE_OUT(__VA_ARGS__)\
    PACKET_DEFINE_IN(__VA_ARGS__)

#define PACKET_DEFINE_CHILD_OUT(parent, ...) \
    public: \
    virtual void WriteToPacketOut(base::gateway::PacketOutBase& pktout) const override {\
        parent::WriteToPacketOut(pktout);\
        base::gateway::MultiWrite(pktout, __VA_ARGS__);\
    }

#define PACKET_DEFINE_CHILD_IN(parent, ...) \
    public: \
    virtual void ReadFromPacketIn(base::gateway::PacketInBase& pktin) override {\
        parent::ReadFromPacketIn(pktin);\
        base::gateway::MultiRead(pktin, __VA_ARGS__);\
    }

#define PACKET_DEFINE_CHILD(parent, ...) \
    PACKET_DEFINE_CHILD_OUT(parent, __VA_ARGS__) \
    PACKET_DEFINE_CHILD_IN(parent, __VA_ARGS__)

    }
}

#endif // PACKET_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
