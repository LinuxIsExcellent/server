#ifndef AIO_H
#define AIO_H
#include <stdint.h>
#include <string>

namespace ts
{
    class Aio
    {
    public:
        static const uint32_t BUFF_SIZE = 65535;
        struct AioHandler {
            virtual ~AioHandler() {}
            virtual void AioHandle(const std::string& str) = 0;
        };
        
    public:
        static Aio& instance() {
            static Aio aio;
            return aio;
        }
        AioHandler* handler() {
            return handler_;
        }

    public:
        void Setup(AioHandler* handler);

    private:
        Aio() {}
        virtual ~Aio() {}
        Aio(const Aio& aio) {}
        void operator=(const Aio& aio) {}
        
        AioHandler* handler_ = nullptr;
    };
}
#endif // AIO_H
