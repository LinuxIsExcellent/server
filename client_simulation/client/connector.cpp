#include "platform.h"
#include "connector.h"
#include "packet.h"
#include "memorypool.h"
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#ifdef HAVE_WINDOWS
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <err.h>
#include <unistd.h>
#include <iostream>
#endif

namespace client
{
    using namespace std;

#ifdef HAVE_WINDOWS
    struct __init_was_env {
        __init_was_env() {
            WSAData wsa_data;
            int rc = WSAStartup(MAKEWORD(2, 2), &wsa_data);
            assert(rc == 0);
            assert(LOBYTE(wsa_data.wVersion) == 2 && HIBYTE(wsa_data.wVersion) == 2);
        }

        ~__init_was_env() {
            WSACleanup();
        }
    };

#endif

    // 设置fd为非阻塞模式
    static bool make_fd_nonblocking(fd_t fd)
    {
#ifdef HAVE_WINDOWS
        u_long nonblock = 1;
        int rc = ioctlsocket(fd, FIONBIO, &nonblock);
        return rc != SOCKET_ERROR;
#else
        int opts = fcntl(fd, F_GETFL);
        if (opts == -1) {
            return false;
        }
        opts = opts | O_NONBLOCK;
        int rc = 0;
        rc = fcntl(fd, F_SETFL, opts);
        if (rc == -1) {
            return false;
        }
        return true;
#endif
    }

    Connector::Connector(ConnectorEventHandler* delegate)
        : handler_(delegate), mempool_(NULL), fd_(invalid_fd),
        connect_(false), connect_pending_(false), recvq_count_(0)
    {
#ifdef HAVE_WINDOWS
        static __init_was_env __init_was_env_v;
#endif
        mempool_ = new MemoryPool(64, 256);
    }

    Connector::~Connector()
    {
        if (fd_ != invalid_fd) {
#ifdef HAVE_WINDOWS
            closesocket(fd_);
#else
            close(fd_);
#endif
            fd_ = invalid_fd;
        }
        sendq_.clear();
        recvq_.clear();
        recvp_.clear();
        SAFE_DELETE(mempool_);
    }

    int Connector::Connect(const char* ip, int port)
    {
        fd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (fd_ == -1) {
            // RM: when app no permission to internet, create socket will fail
            return 5;
        }
        sockaddr_in addr;
        memset(&addr, 0, sizeof(sockaddr_in));
        addr.sin_family = AF_INET;
#ifdef HAVE_WINDOWS
        int r = 0;
        addr.sin_addr.s_addr = inet_addr(ip);
#else
        int r = inet_aton(ip, &(addr.sin_addr));
        if (r == 0) {
            return 2;
        }
#endif

        addr.sin_port = htons(port);

        // make socket as nonblocking
        if (!make_fd_nonblocking(fd_)) {
            return 1;
        }

#ifdef HAVE_WINDOWS
        r = connect(fd_, (sockaddr*)&addr, sizeof(addr));
        if (r == SOCKET_ERROR) {
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                handler_->OnConnectFail(this, strerror(errno));
                return 3;
#else
        r = connect(fd_, (sockaddr*)&addr, sizeof(addr));
        if (r == -1) {
            if (errno != EINPROGRESS) {
                handler_->OnConnectFail(this, strerror(errno));
                return 3;
#endif
            } else {
                connect_pending_ = true;
                DDD("connect. pending");
                return 0;
            }
        } else {
            OnConnect();
            DDD("connect. direct finish");
        }
        return 0;
    }

    void Connector::Close()
    {
        if (fd_ != invalid_fd) {
#ifdef HAVE_WINDOWS
            shutdown(fd_, SD_BOTH);
            closesocket(fd_);
#else
            shutdown(fd_, SHUT_RDWR);
            close(fd_);
#endif
            fd_ = invalid_fd;
        }

    }

    void Connector::OnConnect()
    {
        if (!connect_) {
            connect_ = true;
            handler_->OnConnect(this);
        }
    }

    void Connector::OnClose()
    {
        if (connect_) {
            connect_ = false;
            handler_->OnClose(this);
        }
    }

    size_t Connector::CopyReceive(char* dst, size_t count)
    {
        size_t total = 0;
        std::deque<RefMemoryChunk>::iterator it = recvq_.begin();
        while (it != recvq_.end() && count > 0) {
            uint32_t rd = (*it).Read(dst + total, count);
            (*it).Reset();
            count -= rd;
            total += rd;
            ++it;
        }
        return total;
    }

    size_t Connector::PopReceive(vector< RefMemoryChunk >& dst, size_t count)
    {
        size_t total = 0;
        while (!recvq_.empty() && count > 0) {
            RefMemoryChunk& ck = recvq_.front();
            if (count >= ck.count()) {
                dst.push_back(ck);
                total += ck.count();
                count -= ck.count();
                recvq_.pop_front();
            } else {
                dst.push_back(ck);
                dst.back().ShrinkCount(ck.count() - count);
                ck.ForwardOffset(count);
                total += count;
                count = 0;
            }
        }
        recvq_count_ -= total;
        return total;
    }

    void Connector::OnReceive(uint32_t count)
    {
        // 解析数据包
        while (count >= PacketIn::HEAD_SIZE) {
            CopyReceive(pkthead_.data, sizeof(pkthead_));
            if (pkthead_.head.identifier == PacketIn::IDENTIFIER
                && pkthead_.head.length >= PacketIn::HEAD_SIZE) {
                    if (count >= pkthead_.head.length) {
                        std::vector<RefMemoryChunk> data;
                        PopReceive(data, pkthead_.head.length);
                        PacketIn pktin(data);
                        pktin.ReadHead();
                        handler_->OnReceivePacket(this, pktin);
                        count -= pkthead_.head.length;
                    } else {
                        // 不足以够成一个完整的数据包
                        break;
                    }
            } else {
                char buff[PacketIn::HEAD_SIZE];
                CopyReceive(buff, PacketIn::HEAD_SIZE);
                /*
                std::string text = net::dump_raw_data(buff, PacketIn::HEAD_SIZE);
                LOG_ERROR("detected broken packet from %s:%d: count=%u, identifier=%u, length=%u, data=%s\n",
                ipaddr().c_str(), port(), count, pkthead_.head.identifier, pkthead_.head.length, text.c_str());
                */
                // TODO
                Close();
                break;
            }
        }
    }

    void Connector::Send(packet_data_t data)
    {
        // 加入到发送队列
        for (packet_data_t::iterator it = data.begin(); it != data.end(); ++it) {
            sendq_.push_back(*it);
        }
        InvokeSend();
    }

    void Connector::Send(PacketOut& pktout)
    {
/*
            pktout.WriteHead();
            std::vector<RefMemoryChunk> data = pktout.FetchData();
            if (websocket_frame_parser_ != nullptr) {
                // write websocket head
                size_t size = pktout.size();
                base::gateway::PacketOutBase header(30, mempool());
                uint8_t opCode = (uint8_t)WebSocketFrameType::Binary;
                opCode |= 0x1 << 7;
                header.WriteUInt8(opCode);
                if (size <= 125) {
                    header.WriteUInt8(size);
                } else if (size <= 65535) {
                    header.WriteUInt8(126);
                    size = myntohs((uint16_t)size);
                    header.WriteUInt16(size);
                } else {
                    header.WriteUInt8(127);
                    header.WriteUInt64(size);
                }
                std::vector<memory::RefMemoryChunk> data0 = header.FetchData();
                PushSend(data0);
            }
            PushSend(data); 
            */
            
        pktout.WriteHead();
        packet_data_t data = pktout.FetchData();
        // 加入到发送队列
        for (packet_data_t::iterator it = data.begin(); it != data.end(); ++it) {
            sendq_.push_back(*it);
        }
        InvokeSend();
    }

    void Connector::PerformIO()
    {
        if (!connect_ && connect_pending_) {
            CheckIsConnectCompleted();
        }
        if (connect_) {
            InvokeSend();
            InvokeReceive();
        }
    }

    void Connector::InvokeReceive()
    {
#ifdef HAVE_WINDOWS
        while (connect_) {
            if (recvp_.size() < RECVP_SIZE) {
                mempool_->Aquire(recvp_, RECVP_SIZE - recvp_.size());
            }
            RefMemoryChunk& chunk = recvp_.front();
            int rc = recv(fd_, chunk.data(), chunk.count(), 0);
            if (rc == SOCKET_ERROR) {
                int err = WSAGetLastError();
                if (err == WSAEWOULDBLOCK) {
                    break;
                } else {
                    OnClose();
                    break;
                }
            } else if (rc == 0) {
                OnClose();
                break;
            } else {
                if (rc == chunk.count()) {
                    recvq_.push_back(chunk);
                    recvp_.pop_front();
                } else {
                    recvq_.push_back(chunk);
                    recvq_.back().ShrinkCount(chunk.count() - rc);
                    chunk.ForwardOffset(rc);
                }
                recvq_count_ += rc;
                OnReceive(recvq_count_);
            }
        }
#else
        while (connect_) {
            if (recvp_.size() < RECVP_SIZE) {
                mempool_->Aquire(recvp_, RECVP_SIZE - recvp_.size());
            }
            int cnt = Recvp2IOVec();
            if (cnt == 0) {
                OnClose();
                break;
            }
            int rc = readv(fd_, recviov_, cnt);
            if (rc > 0) {
                size_t total = rc;
                while (total > 0) {
                    RefMemoryChunk& ck = recvp_.front();
                    if (total >= ck.count()) {
                        recvq_.push_back(ck);
                        total -= ck.count();
                        recvp_.pop_front();
                    } else {
                        recvq_.push_back(ck);
                        recvq_.back().ShrinkCount(ck.count() - total);
                        ck.ForwardOffset(total);
                        total = 0;
                    }
                }
                recvq_count_ += rc;
                OnReceive(recvq_count_);
            } else {
                if (rc == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        ;
                    } else {
                        OnClose();
                    }
                } else if (rc == 0) {
                    OnClose();
                }
                break;
            }
        }
#endif
    }

    void Connector::InvokeSend()
    {
#ifdef HAVE_WINDOWS
        while (!sendq_.empty()) {
            RefMemoryChunk& chunk = sendq_.front();
            int rc = send(fd_, chunk.data(), chunk.count(), 0);
            if (rc != SOCKET_ERROR) {
                chunk.ForwardOffset(rc);
                if (chunk.count() == 0) {
                    sendq_.pop_front();
                }
            } else {
                if (WSAGetLastError() == WSAEWOULDBLOCK) {
                    // too busy
                    break;
                } else {
                    OnClose();
                }
            }
        }
#else
        while (true) {
            int cnt = Sendq2IOVec();
            if (cnt == 0) {
                break;
            }
            int rc = writev(fd_, sendiov_, cnt);
            if (rc > 0) {
                size_t num = rc;
                while (num > 0) {
                    RefMemoryChunk& ck = sendq_.front();
                    if (num >= ck.count()) {
                        num -= ck.count();
                        sendq_.pop_front();
                    } else {
                        ck.ForwardOffset(num);
                        num = 0;
                    }
                }
            } else {
                if (rc == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        break;
                    } else {
                        OnClose();
                    }
                }
                break;
            }
        }
#endif
    }

    void Connector::CheckIsConnectCompleted()
    {
        fd_set writefds;
        FD_ZERO(&writefds);
        FD_SET(fd_, &writefds);
        timeval timeout = {0, 0};
        int r = select(fd_ + 1, NULL, &writefds, NULL, &timeout);
        if (r == -1) {
            connect_pending_ = false;
            handler_->OnConnectFail(this, "select fail");
        } else if (r > 0) {
            if (FD_ISSET(fd_, &writefds)) {
                connect_pending_ = false;
                int err = 0;
                socklen_t err_len = sizeof(err);
                if (getsockopt(fd_, SOL_SOCKET, SO_ERROR, (char*)&err, &err_len) == -1) {
                    handler_->OnConnectFail(this, strerror(errno));
                } else {
                    if (err) {
                        handler_->OnConnectFail(this, strerror(err));
                    } else {
                        OnConnect();
                    }
                }
            }
        }
    }
}
