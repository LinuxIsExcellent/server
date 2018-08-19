#include "fuben.h"
#include "frontserver.h"
#include "clientimpl.h"
#include "character.h"
#include "misc.h"
#include <sstream>

namespace dal
{
    using namespace std;

    FuBen::FuBen(ClientImpl *impl, FrontServer *fs) : g_fs(fs)
    {
        HANDLE_MAP(model::SC::SCENARIO_CHAPTER_UPDATE, FuBen::HandleScenarioChapterUpdate);
    }

    FuBen::~FuBen()
    {
    }

    void FuBen::HandleScenarioChapterUpdate(client::PacketIn& pktin)
    {

    }

}