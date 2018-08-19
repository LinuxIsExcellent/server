#ifndef MAP_QO_COMMANDCHECKUSER_H
#define MAP_QO_COMMANDCHECKUSER_H
#include <base/command/commandcallback.h>

namespace ms
{
    namespace map
    {
        namespace qo
        {
            class CommandCheckUsernameIsExist : public base::command::Command
            {
            public:
                CommandCheckUsernameIsExist(const std::string& username, const std::function<void(bool isExist, int64_t uid)>& cb)
                    : m_username(username), m_cb(cb) {}
                virtual ~CommandCheckUsernameIsExist() {}

            private:
                virtual void OnCommandExecute() override;

            private:
                std::string m_username;
                std::function<void(bool, int64_t)> m_cb;
            };

            class CommandCheckNicknameIsExist : public base::command::Command
            {
            public:
                CommandCheckNicknameIsExist(const std::string& nickname, const std::function<void(bool isExist)>& cb)
                    : m_nickname(nickname), m_cb(cb) {}
                virtual ~CommandCheckNicknameIsExist() {}

            private:
                virtual void OnCommandExecute() override;

            private:
                std::string m_nickname;
                std::function<void(bool)> m_cb;
            };

        }
    }
}

#endif // COMMANDCHECKUSER_H
