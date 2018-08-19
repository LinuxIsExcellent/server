#include "websocketheadparser.h"
#include "../utils/utils_string.h"
#include "../utils/crypto.h"
#include <iostream>

namespace base
{
    namespace gateway
    {
        using namespace std;

        WebSocketHeadParser::WebSocketHeadParser()
        {

        }

        WebSocketHeadParser::~WebSocketHeadParser()
        {

        }

        string WebSocketHeadParser::CalculateSecKey()
        {
            auto it = m_headers.find("Sec-WebSocket-Key");
            if (it == m_headers.end()) {
                return "";
            }
            string key(it->second);
            key.append("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
            string output;
            base::utils::sha1(key, output);
            return base::utils::base64_encode(output);
        }

        void WebSocketHeadParser::ParseData(const std::vector< base::memory::RefMemoryChunk >& data)
        {
            for (const base::memory::RefMemoryChunk & ck : data) {
                m_data.push_back(ck);
            }
            UpdateSize();

            if (finish() || error()) {
                return;
            }

            char c;
            bool stop = false;
            while (GetByte(&c, 1) && !stop) {
                switch (m_state) {
                    case State::DATA: {
                        if (c == '\r') {
                            m_state = State::LB_R1;
                        } else {
                            m_tmpLine.push_back(c);
                        }
                    }
                    break;
                    case State::LB_R1: {
                        if (c == '\n') {
                            m_state = State::LB_N1;
                        } else {
                            m_state = State::ERROR;
                        }
                    }
                    break;
                    case State::LB_N1: {
                        if (c == '\r') {
                            m_state = State::LB_R2;
                        } else {
                            vector<string> arr = base::utils::string_split(m_tmpLine, ':');
                            if (arr.size() == 2u) {
                                m_headers.emplace(move(arr[0]), base::utils::string_lr_trim(arr[1].c_str()));
                            }
                            m_tmpLine.clear();

                            m_state = State::DATA;
                            m_tmpLine.push_back(c);
                        }
                    }
                    break;
                    case State::LB_R2: {
                        if (c == '\n') {
                            m_state = State::COMPLETED;
                            stop = true;
                        } else {
                            m_state = State::ERROR;
                        }
                    }
                    break;
                    case State::COMPLETED: {
                        assert(false);
                    }
                    break;
                    case State::ERROR: {
                        stop = true;
                    }
                    break;
                }
            }

            if (m_state == State::COMPLETED) {
                /*
                for (auto it = m_headers.begin(); it != m_headers.end(); ++it) {
                    cout << it->first << "," << it->second << endl;
                }
                */
            } else if (m_state == State::ERROR) {
                cout << "Parse WebSocket Fail" << endl;
            }
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
