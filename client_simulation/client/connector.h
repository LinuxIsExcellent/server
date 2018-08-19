#ifndef CONNECTOR_H
#define CONNECTOR_H

#include "platform.h"
#include "memorychunk.h"
#include "packet.h"
#include <deque>
#include <vector>

#ifdef HAVE_WINDOWS
#include <Windows.h>
#else
#include <sys/uio.h>
#endif

namespace client
{
    class MemoryPool;

#ifdef HAVE_WINDOWS
    typedef UINT_PTR fd_t;
    enum {invalid_fd = (fd_t)(~0)};
#else
    typedef int fd_t;
    enum {
        invalid_fd = -1
    };
#endif

    class Connector;
    struct ConnectorEventHandler {
        virtual ~ConnectorEventHandler() {}
        virtual void OnConnect(Connector* sender) = 0;
        virtual void OnConnectFail(Connector* sender, const char* reason) = 0;
        virtual void OnClose(Connector* sender) = 0;
        virtual void OnReceivePacket(Connector* sender, PacketIn& pktin) = 0;
    };

    class Connector
    {
    public:
        //Connector(ConnectorEventHandler& delegate);
      Connector(ConnectorEventHandler* delegate);
        virtual ~Connector();

        bool closed() const {
            return !connect_ && !connect_pending_;
        }
        bool connected() const {
	    return connect_;
	}
        MemoryPool& mempool() {
            return *mempool_;
        }

        // 连接指定ip与端口
        int Connect(const char* ip, int port);
        // 关闭连接
        void Close();
        void Send(packet_data_t data);
        // 发送协议包
        void Send(PacketOut& pktout);
        // 执行IO
        void PerformIO();
        // 从收到的数据中复制指定长度数据
        // dst 目标地址
        // count 字节数
        // 返回实际拷贝的字符数
        std::size_t CopyReceive(char* dst, std::size_t count);

        // 从收到的数据中弹出指定长度的数据, 同时从接收缓冲池中清除已弹出的数据
        // dst 存放弹出的数据
        // count 字节数
        // 返回实际弹出的字节数
        std::size_t PopReceive(std::vector<RefMemoryChunk>& dst, std::size_t count);

    private:
        void InvokeReceive();
        void InvokeSend();
        void OnConnect();
        void OnClose();
        void CheckIsConnectCompleted();
        void OnReceive(uint32_t count);

        ConnectorEventHandler* handler_;
	
        MemoryPool* mempool_;
        fd_t fd_;
        bool connect_;
        bool connect_pending_;
        std::deque<RefMemoryChunk> sendq_;
        std::deque<RefMemoryChunk> recvq_;
        std::size_t recvq_count_;
        static const uint32_t RECVP_SIZE = 10;
        std::deque<RefMemoryChunk> recvp_;
        PacketIn::PacketHead pkthead_;

#ifdef HAVE_WINDOWS
#else
        static const int32_t SENDIOV_SIZE = 50;
        // 发送相关
        iovec sendiov_[SENDIOV_SIZE];
        int Sendq2IOVec() {
            int cnt = 0;
            std::deque<RefMemoryChunk>::iterator it = sendq_.begin();
            while (it != sendq_.end() && cnt < SENDIOV_SIZE) {
                sendiov_[cnt].iov_base = (*it).data();
                sendiov_[cnt].iov_len = (*it).count();
                ++cnt;
                ++it;
            }
            return cnt;
        }

        // 接收相关
        static const int32_t RECVIOV_SIZE = 30;
        iovec recviov_[RECVIOV_SIZE];
        int Recvp2IOVec() {
            int cnt = 0;
            std::deque<RefMemoryChunk>::iterator it = recvp_.begin();
            while (it != recvp_.end() && cnt < RECVIOV_SIZE) {
                recviov_[cnt].iov_base = (*it).data();
                recviov_[cnt].iov_len = (*it).count();
                ++cnt;
                ++it;
            }
            return cnt;
        }
#endif
    };
}
#endif // CONNECTOR_H
