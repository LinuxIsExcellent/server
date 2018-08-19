#include "palaceWar.h"
#include <base/data.h>

namespace ms
{
    namespace map
    {
        namespace info
        {
            using namespace base;

            std::string SuccessiveKing::Serialize()
            {
                DataTable dt;
                dt.Set(1, headId);
                dt.Set(2, allianceNickname);
                dt.Set(3, nickname);
                dt.Set(4, n);
                return dt.Serialize();
            }

        }
    }
}
