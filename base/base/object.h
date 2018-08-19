#ifndef BASE_OBJECT_H
#define BASE_OBJECT_H

#include "global.h"
#include<set>
#include<unordered_map>

namespace base
{
    class Object;

    class ObjectTracker
    {
    public:
        static ObjectTracker* Create();
        static void Destroy();

        std::string SqlCountsToJson();

    private:
        void AddObject(Object* ptr);
        void RemoveObject(Object* ptr);

        std::set<Object*> m_objects;
        std::unordered_map<std::string, int64_t> m_objectCounts;

        friend class Object;
    };

    // 引用计数的对象
    class Object
    {
    public:
        Object();

        // 对象的引用次数
        uint32_t reference_count() const {
            return reference_count_;
        }

        // 是否单一引用
        bool IsSingleReference() const {
            return reference_count_ == 1;
        }
        // 释放一次引用
        void Release();
        // 持有一次引用
        void Retain();
        // 自动释放(在下次循环中调用Release)
        void AutoRelease();

        virtual const char* GetObjectName() {
            return "base::Object";
        }

    protected:
        virtual ~Object();

    private:
        uint32_t reference_count_;
    };

    class ObjectScope
    {
    public:
        ObjectScope(Object* o, bool retain = false) : o_(o) {
            if (retain) {
                o_->Retain();
            }
        }
        ~ObjectScope() {
            o_->Release();
        }
        Object* get() {
            return o_;
        }

        DISABLE_COPY(ObjectScope);
    private:
        Object* o_;
    };

    template<typename T>
    class SharedObject
    {
    public:
        SharedObject() : o_(nullptr) {}
        ~SharedObject() {
            if (o_ != nullptr) {
                o_->Release();
            }
        }

        T* Get() const {
            return o_;
        }

        SharedObject(const SharedObject& rhs) {
            o_ = rhs.o_;
            if (o_ != nullptr) {
                o_->Retain();
            }
        }

        SharedObject<T>& operator=(const SharedObject& rhs) {
            if (o_ != nullptr) {
                o_->Release();
            }
            o_ = rhs.o_;
            if (o_ != nullptr) {
                o_->Retain();
            }
            return *this;
        }

        SharedObject<T>& operator = (T* o) {
            Ref(o);
            return *this;
        }

        /**
         * 对一个Object生成引用
         */
        void Ref(T* o) {
            if (o_ != nullptr) {
                o_->Release();
            }
            o_ = o;
            if (o_ != nullptr) {
                o_->Retain();
            }
        }

        /**
         * 持有一个Object
         */
        void Hold(T* o) {
            if (o_ != nullptr) {
                o_->Release();
            }
            o_ = o;
        }

        operator bool() const {
            return o_ != nullptr;
        }

        T* operator->() const {
            return o_;
        }

        T& operator*() const {
            return *o_;
        }

    private:
        T* o_;
    };

    extern ObjectTracker* gObjectTracker;
}

#define SAFE_RETAIN(obj) do { if (obj != nullptr) { obj->Retain(); } } while (0)
#define SAFE_RELEASE(obj) do { if (obj != nullptr) { obj->Release(); obj = nullptr; } } while(0)

#endif // OBJECT_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
