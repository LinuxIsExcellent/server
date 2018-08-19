#include "aio.h"
#include <aio.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <signal.h>
#include <string.h>

namespace ts
{
    using namespace std;

    static struct aiocb o_aiocb;
    static char o_buff[Aio::BUFF_SIZE];

    void Aio::Setup(AioHandler* handler)
    {
        memset(&o_aiocb, 0x00, sizeof(aiocb));
        memset(o_buff, '\0', BUFF_SIZE);

        // Setup the AIO request
        bzero((char*)&o_aiocb, sizeof(struct aiocb));
        o_aiocb.aio_fildes = 0;
        o_aiocb.aio_buf = o_buff;
        o_aiocb.aio_nbytes = BUFF_SIZE;
        o_aiocb.aio_offset = 0;

        handler_ = handler;

        /* SIGNAL
         * struct sigaction sig_act;

        // Setup the signal handle
        sigemptyset(&sig_act.sa_mask);
        sig_act.sa_flags = SA_SIGINFO | SA_RESTART;
        sig_act.sa_sigaction = [](int signo, siginfo_t * info, void * contest) {
            struct aiocb* req;

            // Ensure it's our signal
            if (info->si_signo == SIGUSR1) {
                req = (struct aiocb*)info->si_value.sival_ptr;
                // Did the request complete
                if (aio_error(req) == 0) {
                    cout << "aio_buf = " << (char*)req->aio_buf << endl;
                    if (aio_read(&o_aiocb) == -1) {
                        perror("aio_read");
                    }
                    memset(o_buff, '\0', BUFF_SIZE);
                }
            }
        };*/

        /* SIGNAL
                // Link the AIO request with the Signal Handle
                o_aiocb.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
                o_aiocb.aio_sigevent.sigev_signo = SIGUSR1;
                o_aiocb.aio_sigevent.sigev_value.sival_ptr = &o_aiocb;

                // Map the Signal to the Signal Handle
                sigaction(SIGUSR1, &sig_act, nullptr);
        */


        // Link the AIO request with a thread callback
        o_aiocb.aio_sigevent.sigev_notify = SIGEV_THREAD;
        o_aiocb.aio_sigevent.sigev_notify_function = [](sigval_t sigval) {
            struct aiocb* req;
            req = (struct aiocb*)sigval.sival_ptr;

            // Did the request complete
            if (aio_error(req) == 0) {
                char* temp = (char*)req->aio_buf;
                while (*temp != '\n') {
                    ++temp;
                }
                *temp = '\0';
                Aio::instance().handler()->AioHandle((char*)req->aio_buf);
                if (aio_read(&o_aiocb) == -1) {
                    perror("aio_read");
                }
                memset(o_buff, '\0', BUFF_SIZE);
            }
        };
        o_aiocb.aio_sigevent.sigev_notify_attributes = nullptr;
        o_aiocb.aio_sigevent.sigev_value.sival_ptr = &o_aiocb;

        // aio_read
        if (aio_read(&o_aiocb) == -1) {
            perror("aio_read");
        }
    }
}
