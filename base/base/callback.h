#ifndef BASE_CALLBACK_H
#define BASE_CALLBACK_H

#include "object.h"
#include <functional>
#include "observer.h"

namespace base
{
    template<typename t>
    class CallbackObserver;

    // 带观察者的回调函数
    template<typename R, typename...ARGS>
    class CallbackObserver<R(ARGS...)>
    {
    public:
        CallbackObserver() : observer_(nullptr) {}
        CallbackObserver(const std::function<R(ARGS...)>& f, Observer* ob) : fun_(f), observer_(ob) {
            observer_->Retain();
        }
        CallbackObserver(const std::function<R(ARGS...)>& f, const AutoObserver& atob) : fun_(f), observer_(atob.GetObserver()) {
            observer_->Retain();
        }
        CallbackObserver(const CallbackObserver& rhs) {
            observer_ = rhs.observer_;
            fun_ = rhs.fun_;
            if (observer_ != nullptr) {
                observer_->Retain();
            }
        }
        CallbackObserver& operator = (const CallbackObserver& rhs) {
            if (observer_ != nullptr) {
                observer_->Release();
            }
            observer_ = rhs.observer_;
            fun_ = rhs.fun_;
            if (observer_ != nullptr) {
                observer_->Retain();
            }
            return *this;
        }
        virtual ~CallbackObserver() {
            if (observer_ != nullptr) {
                observer_->Release();
                observer_ = nullptr;
            }
        }
        inline bool IsExist() const {
            return observer_ != nullptr && observer_->IsExist();
        }

        inline R operator()(ARGS... args) const {
            if (IsExist()) {
                return fun_(args...);
            }
            return R();
        }

    private:
        std::function<R(ARGS...)> fun_;
        Observer* observer_;
    };

    class CallbackObserverInterface : public Object
    {
    public:
        CallbackObserverInterface(const AutoObserver& atob) : observer_(atob.GetObserver()) {
            observer_->Retain();
        }

        CallbackObserverInterface(const CallbackObserverInterface& rhs) {
            observer_ = rhs.observer_;
            if (observer_ != nullptr) {
                observer_->Retain();
            }
        }

        CallbackObserverInterface& operator = (const CallbackObserverInterface& rhs) {
            if (observer_ != nullptr) {
                observer_->Release();
            }
            observer_ = rhs.observer_;
            if (observer_ != nullptr) {
                observer_->Retain();
            }
            return *this;
        }

        bool IsExist() const {
            return observer_ != nullptr && observer_->IsExist();
        }

    protected:
        virtual ~CallbackObserverInterface() {
            SAFE_RELEASE(observer_);
        }

        Observer* observer_ = nullptr;
    };
}

#endif
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
