#ifndef BASE_LUA_LUAMESSAGE_H
#define BASE_LUA_LUAMESSAGE_H

#include "../cluster/mailboxid.h"
#include "../cluster/message.h"
#include "../utils/utils_string.h"
#include "../data.h"

namespace base
{
    namespace lua
    {
        enum class LuaMessageValueType : int8_t
        {
            NIL             =  0, /* LUA_TNIL,*/
            BOOLEAN         =  1, /* LUA_TBOOLEAN,*/
            LIGHTUSERDAT    =  2, /* LUA_TLIGHTUSERDATA,*/
            NUMBER          =  3, /* LUA_TNUMBER,*/
            STRING          =  4, /* LUA_TSTRING,*/
            TABLE           =  5, /* LUA_TTABLE,*/
            FUNCTION        =  6, /* LUA_TFUNCTION,*/
            USERDATA        =  7, /* LUA_TUSERDATA,*/
            THREAD          =  8, /* LUA_TTHREAD,*/

            EX_INTEGER,
        };

        class LuaMessageBadTypeException : public Exception
        {
        public:
            LuaMessageBadTypeException(LuaMessageValueType expect, LuaMessageValueType got) {
                base::utils::string_append_format(what_, "found wrong type when read lua message, expect %s, got %s", Type2String(expect), Type2String(got));
            }

        private:
            static const char* Type2String(LuaMessageValueType type);
        };

        class LuaMessageReader
        {
        public:
            LuaMessageReader(base::cluster::MessageIn& msgin) : m_msgin(msgin) {}

            bool ReadBoolean() {
                LuaMessageValueType type = (LuaMessageValueType)m_msgin.ReadInt8();
                if (type == LuaMessageValueType::BOOLEAN) {
                    return m_msgin.ReadBoolean();
                }
                throw LuaMessageBadTypeException(LuaMessageValueType::BOOLEAN, type);
            }

            double ReadDouble() {
                LuaMessageValueType type = (LuaMessageValueType)m_msgin.ReadInt8();
                if (type == LuaMessageValueType::NUMBER) {
                    return m_msgin.ReadDouble();
                }
                throw LuaMessageBadTypeException(LuaMessageValueType::NUMBER, type);
            }

            int64_t ReadVarInteger() {
                LuaMessageValueType type = (LuaMessageValueType)m_msgin.ReadInt8();
                if (type == LuaMessageValueType::EX_INTEGER) {
                    return m_msgin.ReadVarInteger<int64_t>();
                }
                throw LuaMessageBadTypeException(LuaMessageValueType::EX_INTEGER, type);
            }

            std::string ReadString() {
                LuaMessageValueType type = (LuaMessageValueType)m_msgin.ReadInt8();
                if (type == LuaMessageValueType::STRING) {
                    return m_msgin.ReadString();
                }
                throw LuaMessageBadTypeException(LuaMessageValueType::STRING, type);
            }

            int ReadArgumentsCount() {
                return m_msgin.ReadVarInteger<int>();
            }

            base::cluster::MailboxID ReadMailboxId();

            DataValue ReadValue();

            DataTable ReadAllArguments();

        private:
            base::cluster::MessageIn& m_msgin;
        };

        class LuaMessageWriter
        {
        public:
            LuaMessageWriter(base::cluster::MessageOut& msgout) : m_msgout(msgout) {}

            void WriteBoolean(bool v) {
                m_msgout.WriteInt8((int8_t)LuaMessageValueType::BOOLEAN);
                m_msgout.WriteBoolean(v);
            }

            void WriteString(const std::string& v) {
                m_msgout.WriteInt8((int8_t)LuaMessageValueType::STRING);
                m_msgout.WriteString(v);
            }

            void WriteVarInteger(int64_t v) {
                m_msgout.WriteInt8((int8_t)LuaMessageValueType::EX_INTEGER);
                m_msgout.WriteVarInteger(v);
            }

            void WriteMailboxId(const base::cluster::MailboxID& mbid) {
                m_msgout.WriteInt8((int8_t)LuaMessageValueType::TABLE);
                m_msgout.WriteVarInteger(2);
                WriteVarInteger(1);
                WriteVarInteger(mbid.nodeid());
                WriteVarInteger(2);
                WriteVarInteger(mbid.pid());
            }

            void WriteDouble(double v) {
                m_msgout.WriteInt8((int8_t)LuaMessageValueType::NUMBER);
                m_msgout.WriteDouble(v);
            }

            void WriteNil() {
                m_msgout.WriteInt8((int8_t)LuaMessageValueType::NIL);
            }

            void WriteArgumentCount(int argc) {
                m_msgout.WriteVarInteger(argc);
            }

            void WriteValue(const DataValue& v);

            void WriteTableBegin(int count);
            void WriteTableEnd();

        private:
            base::cluster::MessageOut& m_msgout;
        };
    }
}

#endif
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
