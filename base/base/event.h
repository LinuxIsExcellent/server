#ifndef BASE_EVENT_H
#define BASE_EVENT_H

#include "global.h"
#include "object.h"
#include "utils/intrusive_list.h"
#include <functional>
#include "observer.h"

namespace base
{
    // 事件链接器，Attach事件回调函数后，首先应持有EventLinker,否则事件无法生效
    // 减持EventLinker则自动移除事件绑定
    // DEPRECATE
    class EventLinker : public Object
    {
        virtual const char* GetObjectName() {
            return "base::EventLinker";
        }
    };

    template<typename T>
    class Event {};

    // 事件
    template<typename...Args>
    class Event<void(Args...)>
    {
    public:
        typedef std::function<void(Args...)> callback_t;
    private:
        struct Handler {
            INTRUSIVE_LIST(Handler)
            callback_t cb;
            EventLinker* linker;
            Observer* observer;

            Handler(const callback_t& _cb) : cb(_cb), linker(new EventLinker()), observer(nullptr) {}
            Handler(const callback_t& _cb, Observer* _observer) : cb(_cb), linker(nullptr), observer(_observer) {
                observer->Retain();
            }
            ~Handler() {
                SAFE_RELEASE(linker);
                SAFE_RELEASE(observer);
            }
        };
    public:
        ~Event() {
            Clear();
        }

        // DEPRECATE
        FIRE_DEPRECATED EventLinker* Attach(callback_t cb) WARN_IF_UNUSED {
            Handler* hd = new Handler(cb);
            handlers_.push_front(hd);
            return hd->linker;
        }

        // DEPRECATE
        FIRE_DEPRECATED EventLinker* AttachBack(callback_t cb) WARN_IF_UNUSED {
            Handler* hd = new Handler(cb);
            handlers_.push_back(hd);
            return hd->linker;
        }

        void Attach(callback_t cb, const AutoObserver& atob) {
            Handler* hd = new Handler(cb, atob.GetObserver());
            handlers_.push_back(hd);
        }

        void AttachFront(callback_t cb, const AutoObserver& atob) {
            Handler* hd = new Handler(cb, atob.GetObserver());
            handlers_.push_front(hd);
        }

        void Trigger(Args... args) {
            Handler* cur = handlers_.front();
            while (cur) {
                if ((cur->linker && cur->linker->reference_count() >= 2)
                        || (cur->observer && cur->observer->IsExist())) {
                    cur->cb(args...);
                    cur = cur->list_next();
                } else {
                    Handler* tmp = cur;
                    cur = handlers_.erase(cur);
                    delete tmp;
                }
            }
        }

        void Clear() {
            Handler* cur = handlers_.front();
            while (cur) {
                Handler* tmp = cur;
                cur = handlers_.erase(cur);
                delete tmp;
            }
        }

    private:
        utils::IntrusiveList<Handler> handlers_;
    };
}

#endif
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
