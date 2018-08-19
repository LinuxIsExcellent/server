#ifndef BASE_DATA_H
#define BASE_DATA_H

#include "global.h"
#include <string>
#include <functional>
#include <cassert>

namespace base
{
    enum class ValueType
    {
        NIL,
        BOOLEAN,
        INTEGER,
        DOUBLE,
        STRING,
        TABLE,
    };

    namespace gateway
    {
        class PacketOutBase;
        class PacketInBase;
    }
    class DataTable;

    class DataValue
    {
    public:
        DataValue() : m_type(ValueType::NIL) {
        }
        explicit DataValue(std::nullptr_t) : m_type(ValueType::NIL) {
        }
        explicit DataValue(bool v) : m_type(ValueType::BOOLEAN) {
            m_val.booleanVal = v;
        }
        explicit DataValue(int v) : m_type(ValueType::INTEGER) {
            m_val.int64Val = v;
        }
        DataValue(int64_t v) : m_type(ValueType::INTEGER) {
            m_val.int64Val = v;
        }
        DataValue(double v) : m_type(ValueType::DOUBLE) {
            m_val.doubleVal = v;
        }
        DataValue(const char* v) : m_type(ValueType::STRING) {
            m_val.stringVal = new std::string(v);
        }
        DataValue(const std::string& v) : m_type(ValueType::STRING) {
            m_val.stringVal = new std::string(v);
        }
        DataValue(std::string && v) : m_type(ValueType::STRING) {
            m_val.stringVal = new std::string(std::move(v));
        }
        DataValue(const DataTable* v);
        DataValue(const DataTable& v);
        DataValue(DataTable && v);
        DataValue(const DataValue& rhs);
        DataValue(DataValue && rhs) {
            m_type = rhs.m_type;
            m_val = rhs.m_val;
            rhs.m_type = ValueType::NIL;
        }
        ~DataValue() {
            Clear();
        }

        DataValue& operator = (std::nullptr_t) {
            if (m_type != ValueType::NIL) {
                Clear();
                m_type = ValueType::NIL;
            }
            return *this;
        }

        DataValue& operator = (bool v) {
            if (m_type != ValueType::BOOLEAN) {
                Clear();
                m_type = ValueType::BOOLEAN;
            }
            m_val.booleanVal = v;
            return *this;
        }

        DataValue& operator = (int v) {
            if (m_type != ValueType::INTEGER) {
                Clear();
                m_type = ValueType::INTEGER;
            }
            m_val.int64Val = v;
            return *this;
        }

        DataValue& operator = (int64_t v) {
            if (m_type != ValueType::INTEGER) {
                Clear();
                m_type = ValueType::INTEGER;
            }
            m_val.int64Val = v;
            return *this;
        }

        DataValue& operator = (float v) {
            if (m_type != ValueType::DOUBLE) {
                Clear();
                m_type = ValueType::DOUBLE;
            }
            m_val.doubleVal = v;
            return *this;
        }

        DataValue& operator = (double v) {
            if (m_type != ValueType::DOUBLE) {
                Clear();
                m_type = ValueType::DOUBLE;
            }
            m_val.doubleVal = v;
            return *this;
        }

        DataValue& operator = (const char* v) {
            if (m_type != ValueType::STRING) {
                Clear();
                m_val.stringVal = new std::string(v);
                m_type = ValueType::STRING;
            } else {
                m_val.stringVal->assign(v);
            }
            return *this;
        }

        DataValue& operator = (const std::string& v) {
            if (m_type != ValueType::STRING) {
                Clear();
                m_val.stringVal = new std::string(v);
            } else {
                m_val.stringVal->assign(v);
            }
            return *this;
        }

        bool operator == (bool v) {
            return m_type == ValueType::BOOLEAN && ToBoolean() == v;
        }
        bool operator != (bool v) {
            return !(*this == v);
        }
        bool operator == (int32_t v) {
            return m_type == ValueType::INTEGER && ToInteger() == v;
        }
        bool operator != (int32_t v) {
            return !(*this == v);
        }
        bool operator == (int64_t v) {
            return m_type == ValueType::INTEGER && ToInteger() == v;
        }
        bool operator != (int64_t v) {
            return !(*this == v);
        }
        bool operator == (float v) {
            return m_type == ValueType::DOUBLE && ToDouble() == v;
        }
        bool operator != (float v) {
            return !(*this == v);
        }
        bool operator == (double v) {
            return m_type == ValueType::DOUBLE && ToDouble() == v;
        }
        bool operator != (double v) {
            return !(*this == v);
        }
        bool operator == (const std::string& v) {
            return m_type == ValueType::STRING && ToString() == v;
        }
        bool operator != (const std::string& v) {
            return !(*this == v);
        }
        bool operator == (const char* v) {
            return m_type == ValueType::STRING && ToString() == v;
        }
        bool operator != (const char* v) {
            return !(*this == v);
        }
        bool operator == (const DataValue& rhs);
        bool operator != (const DataTable& rhs) {
            return !(*this == rhs);
        }

        DataValue& operator = (const DataValue& rhs);
        DataValue& operator = (DataValue && rhs) {
            Clear();
            m_type = rhs.m_type;
            m_val = rhs.m_val;
            rhs.m_type = ValueType::NIL;
            return *this;
        }

        ValueType type() const {
            return m_type;
        }

        bool IsNil() const {
            return m_type == ValueType::NIL;
        }
        bool IsInteger() const {
            return m_type == ValueType::INTEGER;
        }
        bool IsDouble() const {
            return m_type == ValueType::DOUBLE;
        }
        bool IsNumber() const {
            return IsInteger() || IsDouble();
        }
        bool IsString() const {
            return m_type == ValueType::STRING;
        }
        bool IsTable() const {
            return m_type == ValueType::TABLE;
        }
        bool IsBoolean() const {
            return m_type == ValueType::BOOLEAN;
        }

        bool ToBoolean() const {
            return m_val.booleanVal;
        }
        int64_t ToInteger() const {
            return m_val.int64Val;
        }
        double ToDouble() const {
            return m_val.doubleVal;
        }
        double ToNumber() const {
            return IsInteger() ? ToInteger() : ToDouble();
        }
        const std::string& ToString() const {
            assert(m_type == ValueType::STRING);
            return *m_val.stringVal;
        }
        const DataTable& ToTable() const {
            assert(m_type == ValueType::TABLE);
            return *m_val.tableVal;
        }
        DataTable& ToTable() {
            assert(m_type == ValueType::TABLE);
            return *m_val.tableVal;
        }

        void Clear();
        void Display(bool singleLine = true, int depth = 0) const;
        std::string Serialize() const;

    private:
        union Val {
            bool booleanVal;
            int64_t int64Val;
            double doubleVal;
            std::string* stringVal;
            DataTable* tableVal;
        };

        ValueType m_type;
        union Val m_val;
    };

    struct DataTableImpl;

    class DataTable
    {
    public:
        DataTable();
        ~DataTable();
        DataTable(const DataTable& rhs);
        DataTable(DataTable && rhs);

        DataTable& operator = (const DataTable& rhs);
        DataTable& operator = (DataTable && rhs);

        std::size_t Count() const;
        std::size_t VectorCount() const;
        std::size_t MapCount() const;

        DataValue* Get(int64_t key) const;
        DataValue* Get(const char* key) const;

        template<typename T>
        void Set(int64_t key, const T& t) {
            DataValue v(t);
            Set(key, std::move(v));
        }
        void Set(int64_t key, DataValue && v);

        template<typename T>
        void Set(const std::string& key, const T& t) {
            DataValue v(t);
            Set(key, std::move(v));
        }
        void Set(const std::string& key, DataValue && v);

        // qs=/12/attr
        DataValue* Query(const char* qs) const;

        void ForeachVector(const std::function<bool(int64_t, const DataValue&)>& fun) const;
        void ForeachMap(const std::function<bool(const DataValue&, const DataValue&)>& fun)const;
        void ForeachAll(const std::function<bool(const DataValue&, const DataValue&)>& fun) const;

        void DumpInfo(int depth = 0) const;
        std::string Serialize() const;

    private:
        void CheckAndMoveVectorValue();

        DataTableImpl* m_impl;
    };


    base::gateway::PacketOutBase& operator<< (base::gateway::PacketOutBase& pkt, const base::DataValue& value);
    base::gateway::PacketOutBase& operator<< (base::gateway::PacketOutBase& pkt, const base::DataTable& table);

    //base::gateway::PacketInBase& operator>> (base::gateway::PacketInBase& pkt, const base::DataTable& table) throw(const std::string&);
    base::gateway::PacketInBase& readPkt(base::gateway::PacketInBase& pkt, base::DataTable& table) throw(const std::string&);
}


#endif
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
