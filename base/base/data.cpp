#include "data.h"
#include "gateway/packet_base.h"
#include <iostream>
#include <unordered_map>
#include <vector>
#include <string.h>

namespace base
{
    using namespace std;

    DataValue::DataValue(const DataTable* v) : m_type(ValueType::TABLE)
    {
        m_val.tableVal = new DataTable(*v);
    }

    DataValue::DataValue(const DataTable& v) : DataValue(&v)
    {
    }

    DataValue::DataValue(DataTable && v) : m_type(ValueType::TABLE)
    {
        m_val.tableVal = new DataTable(std::move(v));
    }

    DataValue::DataValue(const DataValue& rhs)
    {
        m_type = rhs.m_type;
        m_val = rhs.m_val;
        if (m_type == ValueType::STRING) {
            m_val.stringVal = new std::string(rhs.ToString());
        } else if (m_type == ValueType::TABLE) {
            m_val.tableVal = new DataTable(rhs.ToTable());
        }
    }

    DataValue& DataValue::operator=(const DataValue& rhs)
    {
        Clear();
        m_type = rhs.m_type;
        m_val = rhs.m_val;
        if (m_type == ValueType::STRING) {
            m_val.stringVal = new std::string(rhs.ToString());
        } else if (m_type == ValueType::TABLE) {
            m_val.tableVal = new DataTable(rhs.ToTable());
        }
        return *this;
    }

    bool DataValue::operator==(const DataValue& rhs)
    {
        if (m_type != rhs.m_type) {
            return false;
        } else {
            if (m_type == ValueType::STRING) {
                return ToString() == rhs.ToString();
            } else {
                return memcmp(&m_val, &rhs.m_val, sizeof(m_val)) == 0;
            }
        }
    }

    void DataValue::Clear()
    {
        if (m_type == ValueType::STRING) {
            delete m_val.stringVal;
            m_val.stringVal = nullptr;
        } else if (m_type == ValueType::TABLE) {
            delete m_val.tableVal;
            m_val.tableVal = nullptr;
        }
        m_type = ValueType::NIL;
    }

    void DataValue::Display(bool singleLine /* = true */, int depth /* = 0 */) const
    {
        if (m_type != ValueType::TABLE) {
            for (int i = 0; i < depth; ++i) {
                cout << "    ";
            }
        }
        switch (m_type) {
            case ValueType::NIL:
                cout << "nil";
                break;
            case ValueType::BOOLEAN:
                cout << "boolean:" << boolalpha << ToBoolean();
                break;
            case ValueType::INTEGER:
                cout << "integer:" << ToInteger();
                break;
            case ValueType::DOUBLE:
                cout << "double:" << ToDouble();
                break;
            case ValueType::STRING:
                cout << "string:" << ToString();
                break;
            case ValueType::TABLE:
                DataTable* t = m_val.tableVal;
                t->DumpInfo(depth);
                break;
        }

        if (singleLine) {
            cout << endl;
        }
    }

    string DataValue::Serialize() const
    {
        string str;
        switch (m_type) {
            case ValueType::NIL:
                break;
            case ValueType::BOOLEAN:
                str = to_string(ToBoolean());
                break;
            case ValueType::INTEGER:
                str = to_string(ToInteger());
                break;
            case ValueType::DOUBLE:
                str = to_string(ToDouble());
                break;
            case ValueType::STRING:
                str = "\"" + ToString() + "\"";
                break;
            case ValueType::TABLE:
                const DataTable* t = m_val.tableVal;
                str = t->Serialize();
                break;
        }
        return str;
    }

    /// DataTableImpl
    struct DataTableImpl {
        std::vector<DataValue> vector;
        std::unordered_map<int64_t, DataValue> intKeyDict;
        std::unordered_map<std::string, DataValue> stringKeyDict;
    };

    /// DataTable
    DataTable::DataTable() : m_impl(new DataTableImpl())
    {
    }

    DataTable::~DataTable()
    {
        delete m_impl;
    }

    DataTable::DataTable(const DataTable& rhs) : m_impl(new DataTableImpl())
    {
        m_impl->vector = rhs.m_impl->vector;
        m_impl->intKeyDict = rhs.m_impl->intKeyDict;
        m_impl->stringKeyDict = rhs.m_impl->stringKeyDict;
    }

    DataTable::DataTable(DataTable && rhs) : m_impl(new DataTableImpl())
    {
        DataTableImpl* tmp = m_impl;
        m_impl = rhs.m_impl;
        rhs.m_impl = tmp;
    }

    DataTable& DataTable::operator=(const DataTable& rhs)
    {
        m_impl->vector = rhs.m_impl->vector;
        m_impl->intKeyDict = rhs.m_impl->intKeyDict;
        m_impl->stringKeyDict = rhs.m_impl->stringKeyDict;
        return *this;
    }

    DataTable& DataTable::operator=(DataTable && rhs)
    {
        DataTableImpl* tmp = m_impl;
        m_impl = rhs.m_impl;
        rhs.m_impl = tmp;
        return *this;
    }

    size_t DataTable::Count() const
    {
        return m_impl->vector.size() + m_impl->intKeyDict.size() + m_impl->stringKeyDict.size();
    }

    size_t DataTable::VectorCount() const
    {
        return m_impl->vector.size();
    }

    size_t DataTable::MapCount() const
    {
        return m_impl->intKeyDict.size() + m_impl->stringKeyDict.size();
    }

    DataValue* DataTable::Get(const char* key) const
    {
        auto it = m_impl->stringKeyDict.find(key);
        return it == m_impl->stringKeyDict.end() ? nullptr : &it->second;
    }

    DataValue* DataTable::Get(int64_t key) const
    {
        if (key > 0 && (key - 1 < (int64_t)m_impl->vector.size())) {
            return &m_impl->vector[key - 1];
        }
        auto it = m_impl->intKeyDict.find(key);
        return it == m_impl->intKeyDict.end() ? nullptr : &it->second;
    }

    void DataTable::CheckAndMoveVectorValue()
    {
        int64_t next = (int64_t)m_impl->vector.size() + 1;
        auto it = m_impl->intKeyDict.find(next);
        if (it != m_impl->intKeyDict.end()) {
            m_impl->vector.emplace_back(std::move(it->second));
            m_impl->intKeyDict.erase(it);
            CheckAndMoveVectorValue();
        }
    }

    void DataTable::Set(int64_t key, DataValue && v)
    {
        if (key - 1 == (int64_t)m_impl->vector.size()) {
            m_impl->vector.emplace_back(std::move(v));
            CheckAndMoveVectorValue();
            return;
        }

        auto it = m_impl->intKeyDict.find(key);
        if (it == m_impl->intKeyDict.end()) {
            m_impl->intKeyDict.emplace(key, std::move(v));
        } else {
            it->second = std::move(v);
        }
    }

    void DataTable::Set(const std::string& key, DataValue && v)
    {
        auto it = m_impl->stringKeyDict.find(key);
        if (it == m_impl->stringKeyDict.end()) {
            m_impl->stringKeyDict.emplace(key, std::move(v));
        } else {
            it->second = std::move(v);
        }
    }

    void DataTable::ForeachVector(const function<bool(int64_t, const DataValue&)>& fun) const
    {
        for (size_t i = 0; i < m_impl->vector.size(); ++i) {
            bool b = fun(i + 1, m_impl->vector[i]);
            if (b) {
                return;
            }
        }
    }

    void DataTable::ForeachMap(const function< bool(const DataValue&, const DataValue&) >& fun) const
    {
        for (auto it = m_impl->intKeyDict.begin(); it != m_impl->intKeyDict.end(); ++it) {
            bool b = fun(DataValue(it->first), it->second);
            if (b) {
                return;
            }
        }
        for (auto it = m_impl->stringKeyDict.begin(); it != m_impl->stringKeyDict.end(); ++it) {
            bool b = fun(DataValue(it->first), it->second);
            if (b) {
                return;
            }
        }
    }

    void DataTable::ForeachAll(const function< bool(const DataValue&, const DataValue&) >& fun) const
    {
        for (size_t i = 0; i < m_impl->vector.size(); ++i) {
            bool b = fun(DataValue((int64_t)(i + 1)), m_impl->vector[i]);
            if (b) {
                return;
            }
        }
        for (auto it = m_impl->intKeyDict.begin(); it != m_impl->intKeyDict.end(); ++it) {
            bool b = fun(DataValue(it->first), it->second);
            if (b) {
                return;
            }
        }
        for (auto it = m_impl->stringKeyDict.begin(); it != m_impl->stringKeyDict.end(); ++it) {
            bool b = fun(DataValue(it->first), it->second);
            if (b) {
                return;
            }
        }
    }

    void DataTable::DumpInfo(int depth /* = 0 */) const
    {
        cout << "datatable:" << this;
        cout << " vec.size=" << m_impl->vector.size();
        cout << " intKey.size=" << m_impl->intKeyDict.size();
        cout << " strKey.size=" << m_impl->stringKeyDict.size();
        cout << " count=" << Count() << endl;
        for (int i = 0; i < depth; ++i) {
            cout << "    ";
        }
        cout << "{" << endl;

        ForeachAll([this, depth](const DataValue & k, const DataValue & v) {
            k.Display(false, depth + 1);
            cout << " = ";
            if (v.IsTable()) {
                v.Display(false, depth + 1);
            } else {
                v.Display(false);
            }
            cout << endl;
            return false;
        });
        for (int i = 0; i < depth; ++i) {
            cout << "    ";
        }
        cout << "}";
    }

    string DataTable::Serialize() const
    {
        string str;
        str += "{";
        ForeachAll([&](const DataValue & k, const DataValue & v) {
            str += "[";
            str += k.Serialize();
            str += "]";
            str += "=";
            str += v.Serialize();
            str += ",";
            return false;
        });
        str += "}";
        return str;
    }

    DataValue* DataTable::Query(const char* qs) const
    {
        return Get(qs);
    }

    gateway::PacketOutBase& operator<<(gateway::PacketOutBase& pkt, const DataValue& value)
    {
        ValueType type = value.type();
        switch (type) {
            case ValueType::BOOLEAN: {
                bool v = value.ToBoolean();
                pkt << (int)type;
                pkt << v;
            }
            break;
            case ValueType::INTEGER: {
                int64_t v = value.ToInteger();
                pkt << (int)type;
                pkt << v;
            }
            break;
            case ValueType::DOUBLE: {
                double v = value.ToDouble();
                pkt << (int)type;
                pkt << v;
            }
            break;
            case ValueType::STRING: {
                const string& v = value.ToString();
                pkt << (int)type;
                pkt << v;
            }
            break;
            case ValueType::TABLE: {
                const DataTable& v = value.ToTable();
                pkt << (int)type;
                pkt << v;
            }
            break;
            default:
                break;
        }
        return pkt;
    }

    gateway::PacketOutBase& operator<<(gateway::PacketOutBase& pkt, const DataTable& table)
    {
        pkt << table.Count();
        table.ForeachAll([&](const DataValue & k, const DataValue & v) {
            pkt << k;
            pkt << v;
            return false;
        });
        return pkt;
    }

    gateway::PacketInBase& readPkt(gateway::PacketInBase& pkt, DataTable& table) throw(const string&)
    {
        int argc = pkt.ReadVarInteger<int>();
        // cout << "readToTable argc = " << argc << endl;
        for (int i = 0; i < argc; ++i) {
            int64_t k_i = 0;
            string k_s;
            base::ValueType valueType_k = static_cast<base::ValueType>(pkt.ReadVarInteger<int>());
            // cout << "type_k = " << (int)valueType_k << endl;
            switch (valueType_k) {
                case base::ValueType::INTEGER: {
                    k_i = pkt.ReadVarInteger<int64_t>();
                    //cout << "k_i = " << k_i << endl;
                }
                break;
                case base::ValueType::STRING: {
                    k_s = pkt.ReadString();
                    //cout << "k_s = " << k_s << endl;
                }
                break;
                default:
                    throw "key type just can be INTEGER OR STRING, type = " + to_string((int)valueType_k);
            }
            base::ValueType valueType_v = static_cast<base::ValueType>(pkt.ReadVarInteger<int>());
            // cout << "type_v = " << (int)valueType_v << endl;
            switch (valueType_v) {
                case base::ValueType::BOOLEAN: {
                    bool v = pkt.ReadBoolean();
                    //cout << "v = " << v << endl;
                    if (valueType_k == base::ValueType::INTEGER) {
                        table.Set(k_i, v);
                    } else {
                        table.Set(k_s, v);
                    }
                }
                break;
                case base::ValueType::INTEGER: {
                    int64_t v = pkt.ReadVarInteger<int64_t>();
                    //cout << "v = " << v << endl;
                    if (valueType_k == base::ValueType::INTEGER) {
                        table.Set(k_i, v);
                    } else {
                        table.Set(k_s, v);
                    }
                }
                break;
                case base::ValueType::STRING: {
                    string v = pkt.ReadString();
                    //cout << "v = " << v << endl;
                    if (valueType_k == base::ValueType::INTEGER) {
                        table.Set(k_i, v);
                    } else {
                        table.Set(k_s, v);
                    }
                }
                break;
                case base::ValueType::DOUBLE: {
                    double v = pkt.ReadDouble();
                    //cout << "v = " << v << endl;
                    if (valueType_k == base::ValueType::INTEGER) {
                        table.Set(k_i, v);
                    } else {
                        table.Set(k_s, v);
                    }
                }
                break;
                case base::ValueType::TABLE: {
                    base::DataTable v;
                    try {
                        readPkt(pkt, v);
                    } catch (const string& err) {
                        throw err;
                    }
                    if (valueType_k == base::ValueType::INTEGER) {
                        table.Set(k_i, v);
                    } else {
                        table.Set(k_s, v);
                    }
                }
                break;
                default:
                    throw "value type is WRONG, type = " + to_string((int)valueType_v);
            }
        }
        return pkt;
    }

}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
