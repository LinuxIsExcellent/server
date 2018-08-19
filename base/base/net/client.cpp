#include "client.h"
#include "../net/utils.h"
#include "../logger.h"
#include "../memory/memorypool.h"
#include "../utils/utils_string.h"
#include "../thread/threadpool.h"
#include "../http/hostnameresolver.h"
#include <sys/socket.h>
#include <cerrno>
#include <unistd.h>

namespace base
{
    namespace net
    {
        using namespace std;

        Client::~Client()
        {
        }

        void Client::Connect(int clientfd)
        {
            getpeername(clientfd, &ipaddr_, &port_);
            make_fd_nonblocking(clientfd);
            make_socket_nodelay(clientfd);
            AddToDispatcher(clientfd, event::IO_WRITEABLE);
            OnConnectSuccess();
        }

        void Client::ConnectDomain(const char* domain, int port)
        {
            CallbackObserver<void(const http::DnsRecord&)> callback([this, port](const http::DnsRecord & dnsRecord) {
                string ip = dnsRecord.getIP();
                if (ip.empty()) {
                    OnConnectFail(-1, "can not resolve domain");
                } else {
                    ConnectIp(ip.c_str(), port);
                }
            }, m_autoObserver);
            std::shared_ptr<http::HostnameResolver> resolver(new http::HostnameResolver(domain, callback));
            thread::ThreadPool::getInstance()->queueUserWorkItem(resolver);
        }

        void Client::Connect(const char* host, int port)
        {
            if (is_ip_addr(host)) {
                ConnectIp(host, port);
            } else {
                ConnectDomain(host, port);
            }
        }

        void Client::ConnectIp(const char* ipaddr, int port)
        {
            ipaddr_ = ipaddr;
            port_ = port;
            int clientfd = ::socket(AF_INET, SOCK_STREAM, 0);
            errno_assert(clientfd != -1);
            sockaddr_in addr;
            convert_sockaddr(&addr, ipaddr, port);
            make_fd_nonblocking(clientfd);
            make_socket_nodelay(clientfd);
            AddToDispatcher(clientfd, event::IO_WRITEABLE);
            int rc = ::connect(clientfd, (sockaddr*)&addr, sizeof(struct sockaddr_in));
            if (rc == -1) {
                if (errno != EINPROGRESS) {
                    OnConnectFail(errno, strerror(errno));
                    Close();
                } else {
                    connect_pending_ = true;
                }
            } else {
                OnConnectSuccess();
            }
        }

        void Client::OnEventIOReadable()
        {
            // 不停的读，直到没有数据可读或是出现EAGAIN错误时
            while (connect() && !closed()) {
                if (recvp_.size() < RECVP_SIZE) {
                    mempool_.Aquire(recvp_, RECVP_SIZE - recvp_.size());
                }
                int cnt = Recvp2IOVec();
                if (cnt == 0) {
                    LOG_ERROR("allocate receive memory buffer fail, recvp_.size()=%u\n", recvp_.size());
                    Close();
                    break;
                }
                int rc = readv(fd(), recviov_, cnt);
                if (rc  > 0) {
                    size_t total = rc;
                    while (total > 0) {
                        // 将接收缓冲池的数据转移到接收队列中，等待处理
                        memory::RefMemoryChunk& ck = recvp_.front();
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
                    recvq_count_ += rc; // 待处理的字节数
                    OnReceive(recvq_count_); // 抛出事件
                } else {
                    if (rc == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            ; // 没有更多数据了
                        } else {
                            // 发生错误关闭
                            if (errno != ECONNRESET && errno != ETIMEDOUT) {
                                LOG_ERROR("client read error: errno=%d, %s\n", errno, strerror(errno));
                            }
                            Close();
                        }
                    } else {
                        Close(); // 正常关闭 rc == 0
                    }
                    break;
                }
            }
        }

        void Client::OnEventIOWriteable()
        {
            if (connect_pending_) {
                CheckIfConnectCompleted();
            }
            if (connect_) {
                ModifyIOEvent(event::IO_READABLE);
                InvokeSend();
            }
        }

        void Client::OnEventIOClose()
        {
            if (connect_) {
                connect_ = false;
                OnClose();
            }
        }

        void Client::CheckIfConnectCompleted()
        {
            connect_pending_ = false;
            int err = 0;
            socklen_t errlen = sizeof(int);
            if (getsockopt(fd(), SOL_SOCKET, SO_ERROR, &err, &errlen) == -1) {
                OnConnectFail(errno, strerror(errno));
                Close();
            } else {
                if (err) {
                    OnConnectFail(err, strerror(err));
                    Close();
                } else {
                    OnConnectSuccess();
                }
            }
        }

        size_t Client::CopyReceive(char* dst, size_t count)
        {
            size_t total = 0;
            std::deque<memory::RefMemoryChunk>::iterator it = recvq_.begin();
            while (it != recvq_.end() && count > 0) {
                uint32_t rd = (*it).Read(dst + total, count);
                (*it).Reset();
                count -= rd;
                total += rd;
                ++it;
            }
            return total;
        }

        size_t Client::PopReceive(vector< memory::RefMemoryChunk >& dst, size_t count)
        {
            size_t total = 0;
            while (!recvq_.empty() && count > 0) {
                memory::RefMemoryChunk& ck = recvq_.front();
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

        size_t Client::PopReceive(char* dst, size_t count)
        {
            size_t total = 0;
            while (!recvq_.empty() && count > 0) {
                memory::RefMemoryChunk& ck = recvq_.front();
                if (count >= ck.count()) {
                    ck.Read(dst, ck.count());
                    total += ck.count();
                    count -= ck.count();
                    recvq_.pop_front();
                } else {
                    ck.Read(dst, count);
                    ck.ForwardOffset(count);
                    total += count;
                    count = 0;
                }
            }
            recvq_count_ -= total;
            return total;
        }

        void Client::InvokeSend()
        {
            // 一直发送，直到发送队列为空或是出现EAGAIN错误
            bool sendevt = false;
            while (true) {
                int cnt = Sendq2IOVec();
                if (cnt == 0) {
                    break;
                }
                int rc = ::writev(fd(), sendiov_, cnt);
                if (rc > 0) {
                    sendevt = true;
                    size_t num = rc;
                    while (num > 0) {
                        // 从发送队列中弹出已成功送出的数据
                        memory::RefMemoryChunk& ck = sendq_.front();
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
                            ModifyIOEvent(event::IO_READABLE | event::IO_WRITEABLE);
                        } else {
                            Close();
                        }
                    }
                    break;
                }
            }
            if (sendevt && sendq_.empty()) {
                OnSendCompleted();
            }
        }

        void Client::Close()
        {
            if (fd() != -1) {
                shutdown(fd(), SHUT_RDWR);
            }
            base::event::EventIO::Close();
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
