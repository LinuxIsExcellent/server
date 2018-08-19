#include "luamessage.h"
#include "../logger.h"

namespace base
{
    namespace lua
    {
        using namespace std;

        const char* LuaMessageBadTypeException::Type2String(LuaMessageValueType type)
        {
            switch (type) {
                case LuaMessageValueType::NIL:
                    return "NIL";
                case LuaMessageValueType::BOOLEAN:
                    return "BOOLEAN";
                case LuaMessageValueType::LIGHTUSERDAT:
                    return "LIGHTUSERDAT";
                case LuaMessageValueType::NUMBER:
                    return "NUMBER";
                case LuaMessageValueType::STRING:
                    return "STRING";
                case LuaMessageValueType::TABLE:
                    return "TABLE";
                case LuaMessageValueType::FUNCTION:
                    return "FUNCTION";
                case LuaMessageValueType::USERDATA:
                    return "USERDATA";
                case LuaMessageValueType::THREAD:
                    return "THREAD";
                case LuaMessageValueType::EX_INTEGER:
                    return "EX_INTEGER";
                default:
                    return "???";
            }
        }

        cluster::MailboxID LuaMessageReader::ReadMailboxId()
        {
            LuaMessageValueType type = (LuaMessageValueType)m_msgin.ReadInt8();
            if (type == LuaMessageValueType::TABLE) {
                int len = m_msgin.ReadVarInteger<int>();
                if (len == 2) {
                    DataValue k1 = ReadValue();
                    DataValue v1 = ReadValue();
                    DataValue k2 = ReadValue();
                    DataValue v2 = ReadValue();
                    if (k1.IsInteger() && v1.IsInteger() && k2.IsInteger() && v2.IsInteger()) {
                        return base::cluster::MailboxID(v1.ToInteger(), v2.ToInteger());
                    }
                }
            }
            throw LuaMessageBadTypeException(LuaMessageValueType::TABLE, type);
        }

        DataValue LuaMessageReader::ReadValue()
        {
            LuaMessageValueType type = (LuaMessageValueType)m_msgin.ReadInt8();
            switch (type) {
                case LuaMessageValueType::BOOLEAN: {
                    bool v = m_msgin.ReadBoolean();
                    return DataValue(v);
                }
                break;
                case LuaMessageValueType::NIL: {
                    return DataValue();
                }
                break;
                case LuaMessageValueType::EX_INTEGER: {
                    int64_t v = m_msgin.ReadVarInteger<int64_t>();
                    return DataValue(v);
                }
                break;
                case LuaMessageValueType::NUMBER: {
                    double v = m_msgin.ReadDouble();
                    return DataValue(v);
                }
                break;
                case LuaMessageValueType::STRING: {
                    std::string v = m_msgin.ReadString();
                    return DataValue(std::move(v));
                }
                break;
                case LuaMessageValueType::TABLE: {
                    DataTable table;
                    int len = m_msgin.ReadVarInteger<int>();
                    for (int i = 0; i < len; ++i) {
                        DataValue key = ReadValue();
                        DataValue val = ReadValue();
                        if (key.IsInteger()) {
                            table.Set(key.ToInteger(), val);
                        } else if (key.IsString()) {
                            table.Set(key.ToString(), val);
                        }
                    }
                    return table;
                }
                break;
                default:
                    LOG_WARN2("cat not read type of %d value", (int)type);
                    break;
            }
            return DataValue();
        }

        DataTable LuaMessageReader::ReadAllArguments()
        {
            int argc = m_msgin.ReadVarInteger<int>();
            DataTable table;
            for (int i = 0; i < argc; ++i) {
                table.Set(i + 1, ReadValue());
            }
            return table;
        }

        void LuaMessageWriter::WriteValue(const DataValue& v)
        {
            switch (v.type()) {
                case base::ValueType::BOOLEAN: {
                    WriteBoolean(v.ToBoolean());
                }
                break;
                case base::ValueType::DOUBLE: {
                    WriteDouble(v.ToDouble());
                }
                break;
                case base::ValueType::INTEGER: {
                    WriteVarInteger(v.ToInteger());
                }
                break;
                case base::ValueType::NIL: {
                    WriteNil();
                }
                break;
                case base::ValueType::STRING: {
                    WriteString(v.ToString());
                }
                break;
                case base::ValueType::TABLE: {
                    m_msgout.WriteInt8((int8_t)LuaMessageValueType::TABLE);
                    const DataTable& table = v.ToTable();
                    m_msgout.WriteVarInteger(table.Count());
                    table.ForeachVector([this](int k, const DataValue & v) {
                        WriteVarInteger(k);
                        WriteValue(v);
                        return false;
                    });
                    table.ForeachMap([this](const DataValue & k, const DataValue & v) {
                        WriteValue(k);
                        WriteValue(v);
                        return false;
                    });
                }
                break;
            }
        }

        void LuaMessageWriter::WriteTableBegin(int count)
        {
            m_msgout.WriteInt8((int8_t)LuaMessageValueType::TABLE);
            m_msgout.WriteVarInteger(count);
        }

        void LuaMessageWriter::WriteTableEnd()
        {
            // TODO close tag check?
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
