#ifndef BASE_OBSERVER_H
#define BASE_OBSERVER_H

#include "object.h"

namespace base
{
    class AutoObserver;
    class Observer : public Object
    {
    public:
        static Observer* Create() {
            Observer* ret = new Observer;
            ret->AutoRelease();
            return ret;
        }

        virtual ~Observer() {}

        bool IsExist() const {
            return exist_;
        }

        void SetNotExist() {
            exist_ = false;
        }

        virtual const char* GetObjectName() {
            return "base::Observer";
        }

    private:
        Observer() : exist_(true) {}
        bool exist_;

        friend class AutoObserver;
    };

    class AutoObserver
    {
    public:
        AutoObserver() : observer_(new Observer) {}
        ~AutoObserver() {
            observer_->SetNotExist();
            observer_->Release();
        }

        Observer* GetObserver() const {
            return observer_;
        }

        void SetNotExist() {
            observer_->SetNotExist();
        }

        // 重新创建，使以前的连接失败
        void ReCreate() {
            observer_->SetNotExist();
            observer_->Release();
            observer_ = new Observer;
        }

        AutoObserver(const AutoObserver& rhs) = delete;
        AutoObserver& operator = (const AutoObserver& rhs) = delete;

    private:
        Observer* observer_;
    };

    class InterfaceWithObserver : public Object
    {
    public:
        InterfaceWithObserver(const AutoObserver& atob) : m_observer(atob.GetObserver()) {
            m_observer->Retain();
        }
        InterfaceWithObserver(Observer* ob) : m_observer(ob) {
            m_observer->Retain();
        }
        virtual ~InterfaceWithObserver() {
            m_observer->Release();
        }

        // disable copy
        InterfaceWithObserver(const InterfaceWithObserver& rhs) = delete;
        InterfaceWithObserver& operator=(const InterfaceWithObserver& rhs) = delete;

        bool IsExist() const {
            return m_observer->IsExist();
        }

        virtual const char* GetObjectName() {
            return "base::InterfaceWithObserver";
        }

    private:
        Observer* m_observer;
    };
}

#endif // OBSERVER_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
