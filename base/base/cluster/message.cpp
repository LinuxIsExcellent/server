#include "message.h"
#include <iostream>

namespace base
{
    namespace cluster
    {
        using namespace std;

        void MessageIn::ResetFromNodeID(uint16_t node_id)
        {
            rewrite_node_id = node_id;
            mbid_from_ = MailboxID(rewrite_node_id, mbid_from_.pid());
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
