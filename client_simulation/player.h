#ifndef PLAYER_H
#define PLAYER_H

#include "configure.h"
#include <Theron/Theron.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "./dal/frontserver.h"

using namespace std;

class PingPong : public Theron::Actor
{
public:

    struct StartMessage
    {
        inline StartMessage(dal::FrontServer* fs, const Theron::Address& caller, const Theron::Address& partner) :
            fs_(fs),
            mCaller(caller),
            mPartner(partner) {
        }
        dal::FrontServer *fs_;
        Theron::Address mCaller;
        Theron::Address mPartner;
    };

    inline PingPong(Theron::Framework &framework) : Theron::Actor(framework)
    {
        RegisterHandler(this, &PingPong::Start);
    }

private:

    inline void Start(const StartMessage &message, const Theron::Address /*from*/)
    {
        mCaller = message.mCaller;
        mPartner = message.mPartner;
        fs_ = message.fs_;
        DeregisterHandler(this, &PingPong::Start);
        RegisterHandler(this, &PingPong::Receive);
    }

    inline void Receive(const int &message, const Theron::Address /*from*/)
    {
        //if (message > 0)
        {
	    printf("=== Actor address = 0X%p , count = %d \n", this, message);
	    sleep(1);

            TailSend(message - 1, mPartner);
            return;
        }
    }
    dal::FrontServer *fs_;
    Theron::Address mCaller;
    Theron::Address mPartner;
};

class Player  : public Theron::Actor
{
public:
    ~Player();
    Player(const UserActions& useractions, Theron::Framework& framework);
    void Run(const std::string &message, const Theron::Address from);
    void Stop(const std::string &message, const Theron::Address from);
    
private:
    uint32_t Login();
    uint32_t DoOneStep(const std::string& actionname, const std::string& step, const std::string& param);
private:
    dal::FrontServer *fs_;
    std::string username_;
    UserActions useractions_;
};

#endif // PLAYER_H
