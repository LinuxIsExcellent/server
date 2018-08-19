#include "alliance.h"
#include "agent.h"
#include <base/data.h>
#include "mapMgr.h"
#include "mapProxy.h"
#include "unit/famouscity.h"
#include "unit/worldboss.h"
#include "unit/catapult.h"
#include <base/logger.h>
#include <model/tpl/templateloader.h>

namespace ms
{
    namespace map
    {
        using namespace std;
        
        // 获得名城主权
        void AllianceSimple::OnAllianceOwnCity(FamousCity* city)
        {
            if (city) {
                g_mapMgr->AllianceOwnCity(info.id, city);
            }
        }

        // 失去名城主权
        void AllianceSimple::OnAllianceLoseCity(FamousCity* city, int atkAllianceId)
        {
            if (city) {
                g_mapMgr->AllianceLoseCity(info.id, city, atkAllianceId);
                for (int64_t uid : members) {
                    if (Agent* agt = g_mapMgr->FindAgentByUID(uid)) {
                        agt->OnAllianceLoseCity(city->id());
                    }
                }
            }
        }

        // 获得名城占领权
        void AllianceSimple::OnAllianceOwnOccupyCity(FamousCity* city)
        {
            if (city) {
                g_mapMgr->AllianceOwnOccupyCity(info.id, city);
            }
        }

        //失去名城占领权
        void AllianceSimple::OnAllianceLoseOccupyCity(FamousCity* city, int atkAllianceId)
        {
            if (city) {
                g_mapMgr->AllianceLoseOccupyCity(info.id, city, atkAllianceId);
            }
        }


        void AllianceSimple::AllianceBuffOpen(const info::AllianceBuffInfo& buffInfo)
        {
            allianceBuffs.erase(buffInfo.buffId);
            allianceBuffs.emplace(buffInfo.buffId, buffInfo);
            for (int64_t uid : members) {
                if (Agent* agt = g_mapMgr->FindAgentByUID(uid)) {
                    agt->AllianceBuffOpen(buffInfo);
                }
            }

            if (buffInfo.type == model::AllianceBuffType::NEUTRAL_CASTLE_WALL_RECOVER) {
                auto cities = g_mapMgr->AllianceCity(info.id);
                if (cities) {
                    for (auto city : *cities) {
                        if (city) {
                            int cityDefenseMax = city->cityDefenseMax();
                            city->AddCityDefense(buffInfo.param1/10000.0 * cityDefenseMax);
                            city->NoticeUnitUpdate();
                        }
                    }
                }
            }
        }

        void AllianceSimple::AllianceBuffClosed(int buffId)
        {
            auto it = allianceBuffs.find(buffId);
            if (it !=  allianceBuffs.end()) {
                auto buff = it->second;
                for (int64_t uid : members) {
                    if (Agent* agt = g_mapMgr->FindAgentByUID(uid)) {
                        agt->AllianceBuffClosed(buff.type);
                    }
                }
            }
            allianceBuffs.erase(buffId);
        }

        void AllianceSimple::OnKillWorldBoss(const std::string& nickname, const WorldBoss& boss) const
        {
            // 发击杀boss联盟奖励邮件
            base::DataTable dropsDt;
            base::DataTable paramDt;
            if (const DropTpl* dropTpl = g_tploader->FindDrop(boss.tpl().allianceDropId)) {
                std::vector<DropItem> drops = dropTpl->DoDrop();
                SetDropsTable(dropsDt, drops);
            }
            paramDt.Set("params1", boss.tpl().name);
            for(int64_t uid : members) {
                g_mapMgr->proxy()->SendMail(uid, 0, model::MailType::SYSTEM, MailSubType::SYSTEM_WORLDBOSS_ALLIANCE_REWARD, dropsDt.Serialize(), paramDt.Serialize(), true);
            }
        }

        void AllianceSimple::OnKillCityNpc(const std::string& nickname, const FamousCity& city) const
        {
            // 发击杀boss联盟奖励邮件
            base::DataTable dropsDt;
            base::DataTable paramDt;
            // cout << "city.tpl().allianceDropId" << city.tpl().allianceDropId << endl;
            if (const DropTpl* dropTpl = g_tploader->FindDrop(city.tpl().allianceDropId)) {
                std::vector<DropItem> drops = dropTpl->DoDrop();
                SetDropsTable(dropsDt, drops);
                paramDt.Set("params1", city.tpl().name);
                for(int64_t uid : members) {
                g_mapMgr->proxy()->SendMail(uid, 0, model::MailType::SYSTEM, MailSubType::SYSTEM_CITY_ALLIANCE_REWARD, dropsDt.Serialize(), paramDt.Serialize(), true);
                }
            }
        }

        void AllianceSimple::OnKillCatapultNpc(const std::string& nickname, const Catapult& catapult) const
        {
             // 发击杀boss联盟奖励邮件
            base::DataTable dropsDt;
            base::DataTable paramDt;
            // cout << "city.tpl().allianceDropId" << catapult.tpl().allianceDropId << endl;
            if (const DropTpl* dropTpl = g_tploader->FindDrop(catapult.tpl().allianceDropId)) {
                std::vector<DropItem> drops = dropTpl->DoDrop();
                SetDropsTable(dropsDt, drops);
                paramDt.Set("params1", catapult.tpl().name);
                for(int64_t uid : members) {
                    g_mapMgr->proxy()->SendMail(uid, 0, model::MailType::SYSTEM, MailSubType::SYSTEM_CATAPULT_ALLIANCE_REWARD, dropsDt.Serialize(), paramDt.Serialize(), true);
                }
            }
        }

        AllianceSimple*  Alliance::FindAllianceSimple(int64_t aid) {
            auto it = m_alliances.find(aid);
            return it != m_alliances.end() ? &it->second : nullptr;
        }

        void Alliance::HandleMessage(const std::string& method, const base::DataTable& dt)
        {
            // std::cout << "---------Alliance::HandleMessage method = " <<  method <<  std::endl;
            if (method == "alliance_sync_all") {
                CleanAlliances();
                base::DataTable& alsTab = dt.Get(1)->ToTable();
                uint16_t count = alsTab.Count();
                for (uint16_t i = 1; i <= count; ++i) {
                    base::DataTable& at = alsTab.Get(i)->ToTable();
                    AllianceSimple simple;
                    simple.info.id = at.Get("id")->ToNumber();
                    simple.info.name = at.Get("name")->ToString();
                    simple.info.nickname = at.Get("nickname")->ToString();
                    simple.info.level = at.Get("level")->ToNumber();
                    simple.info.bannerId = at.Get("bannerId")->ToNumber();
                    simple.info.toatlActive = at.Get("toatlActive")->ToNumber();
                    simple.info.leaderId = at.Get("leaderId")->ToNumber();
                    simple.info.leaderName = at.Get("leaderName")->ToString();
                    simple.info.leaderHeadId = at.Get("leaderHeadId")->ToNumber();
                    base::DataTable& mt = at.Get("memberList")->ToTable();
                    base::DataTable& st = at.Get("scienceList")->ToTable();

                    mt.ForeachVector([&](int k, const base::DataValue & v) {
                        const base::DataTable& m = v.ToTable();
                        int64_t uid = m.Get("uid")->ToNumber();
                        //printf("Alliance::HandleMessage ... sync_alliances aid %d, index %d, uid %ld\n", simple.info.id, k, uid);
                        simple.members.push_back(uid);
                        return false;
                    });

                    st.ForeachVector([&](int k, const base::DataValue & v) {
                        const base::DataTable& m = v.ToTable();
                        int groupId = m.Get("groupId")->ToNumber();
                        int tplId = m.Get("tplId")->ToNumber();
                        int level = m.Get("level")->ToNumber();
                        // printf("Alliance::HandleMessage ... alliance_sync_all id %d groupId %d, tplId %d, level %ld\n", simple.info.id, groupId, tplId, level);
                        info::AllianceScienceInfo sicences;
                        sicences.groupId = groupId;
                        sicences.tplId = tplId;
                        sicences.level = level;
                        simple.sciences.push_back(sicences);
                        return false;
                    });

                    //printf("Alliance::HandleMessage ... sync_alliances index %d, aid %ld, name %s, level %d, bannerId %ld, toatlActive %d, leaderId %ld, leaderName %s, towerMax %d, castleMax %d, member size %d\n", i, simple.info.id, simple.info.name.c_str(), simple.info.level, simple.info.bannerId, simple.info.toatlActive, simple.info.leaderId, simple.info.leaderName.c_str(), simple.info.towerMax, simple.info.neutralCastleMax, int(simple.members.size()));
                    UpdateAlliance(simple, true);
                }

            } else if (method == "alliance_create") {
                base::DataTable& at = dt.Get(1)->ToTable();
                AllianceSimple simple;
                simple.info.id = at.Get("id")->ToNumber();
                simple.info.name = at.Get("name")->ToString();
                simple.info.nickname = at.Get("nickname")->ToString();
                simple.info.level = at.Get("level")->ToNumber();
                simple.info.bannerId = at.Get("bannerId")->ToNumber();
                simple.info.toatlActive = at.Get("toatlActive")->ToNumber();
                simple.info.leaderId = at.Get("leaderId")->ToNumber();
                simple.info.leaderName = at.Get("leaderName")->ToString();
                simple.info.leaderHeadId = at.Get("leaderHeadId")->ToNumber();
                base::DataTable& mt = at.Get("memberList")->ToTable();
                base::DataTable& st = at.Get("scienceList")->ToTable();

                mt.ForeachVector([&](int k, const base::DataValue & v) {
                    const base::DataTable& m = v.ToTable();
                    int64_t uid = m.Get("uid")->ToNumber();
                    //printf("Alliance::HandleMessage ... create_alliance aid %d, index %d, uid %ld\n", simple.info.id, k, uid);
                    simple.members.push_back(uid);
                    return false;
                });

                st.ForeachVector([&](int k, const base::DataValue & v) {
                    const base::DataTable& m = v.ToTable();
                    int groupId = m.Get("groupId")->ToNumber();
                    int tplId = m.Get("tplId")->ToNumber();
                    int level = m.Get("level")->ToNumber();
                    // printf("Alliance::HandleMessage ... alliance_create id %d groupId %d, tplId %d, level %ld\n", simple.info.id, groupId, tplId, level);
                    info::AllianceScienceInfo sicences;
                    sicences.groupId = groupId;
                    sicences.tplId = tplId;
                    sicences.level = level;
                    simple.sciences.push_back(sicences);
                    return false;
                });


                //printf("Alliance::HandleMessage ... sync_alliances aid %ld, name %s, level %d, bannerId %ld, toatlActive %d, leaderId %ld, leaderName %s, towerMax %d, castleMax %d, member size %d\n", simple.info.id, simple.info.name.c_str(), simple.info.level, simple.info.bannerId, simple.info.toatlActive, simple.info.leaderId, simple.info.leaderName.c_str(), simple.info.towerMax, simple.info.neutralCastleMax, int(simple.members.size()));
                UpdateAlliance(simple);

            } else if (method == "alliance_update") {
                AllianceSimpleInfo info;
                base::DataTable& at = dt.Get(1)->ToTable();
                info.id = at.Get("id")->ToNumber();
                info.name = at.Get("name")->ToString();
                info.nickname = at.Get("nickname")->ToString();
                info.level = at.Get("level")->ToNumber();
                info.bannerId = at.Get("bannerId")->ToNumber();
                info.toatlActive = at.Get("toatlActive")->ToNumber();
                info.leaderId = at.Get("leaderId")->ToNumber();
                info.leaderName = at.Get("leaderName")->ToString();
                info.leaderHeadId = at.Get("leaderHeadId")->ToNumber();
                //printf("Alliance::HandleMessage ... sync_alliances aid %ld, name %s, level %d, bannerId %ld, toatlActive %d, leaderId %ld, leaderName %s, towerMax %d, castleMax %d, member size %d\n", info.id, info.name.c_str(), info.level, info.bannerId, info.toatlActive, info.leaderId, info.leaderName.c_str(), info.towerMax, info.neutralCastleMax);
                UpdateAllianceInfo(info);

            } else if (method == "alliance_disband") {
                int64_t aid = dt.Get(1)->ToNumber();
                Disband(aid);
            } else if (method == "alliance_add_member") {
                int64_t aid = dt.Get(1)->ToNumber();
                int64_t uid = dt.Get(2)->ToNumber();
                AddMember(aid, uid);
            } else if (method == "alliance_remove_member") {
                int64_t aid = dt.Get(1)->ToNumber();
                int64_t uid = dt.Get(2)->ToNumber();
		        int64_t allianceCdTimestamp = dt.Get(3)->ToNumber();
                RemoveMember(aid, uid);
		        setAllianceCdTimestamp(uid, allianceCdTimestamp);
            } else if (method == "alliance_record_invited") {
                int64_t uid = dt.Get(1)->ToNumber();
                RecordInvited(uid);
            } else if (method == "alliance_buff_open") {
                int64_t aid = dt.Get(1)->ToNumber();
                base::DataTable& buffTable = dt.Get(2)->ToTable();
                info::AllianceBuffInfo info;
                info.buffId = buffTable.Get("buffId")->ToInteger();
                info.type = static_cast<model::AllianceBuffType>(buffTable.Get("type")->ToInteger());
                info.endTimestamp = buffTable.Get("endTimestamp")->ToInteger();
                info.param1 = buffTable.Get("param1")->ToInteger();
                info.attr = buffTable.Get("attr")->ToTable();
                AllianceBuffOpen(aid, info);
            } else if (method == "alliance_buff_closed") {
                int64_t aid = dt.Get(1)->ToNumber();
                int64_t buffId = dt.Get(2)->ToNumber();
                AllianceBuffClosed(aid, buffId);
            } else if (method == "alliance_science_update") {
                int64_t aid = dt.Get(1)->ToNumber();
                base::DataTable& st = dt.Get(2)->ToTable();
                AllianceSimple* allianceSimple = FindAllianceSimple(aid);
                if (allianceSimple)
                {
                    allianceSimple->sciences.clear();
                    uint16_t count = st.Count();
                    for (uint16_t i = 1; i <= count; ++i) {
                        base::DataTable& at = st.Get(i)->ToTable();
                        int groupId = at.Get("groupId")->ToNumber();
                        int tplId = at.Get("tplId")->ToNumber();
                        int level = at.Get("level")->ToNumber();
                        // printf("Alliance::HandleMessage ... alliance_science_update id %d groupId %d, tplId %d, level %ld\n", allianceSimple->info.id, groupId, tplId, level);
                        info::AllianceScienceInfo sicences;
                        sicences.groupId = groupId;
                        sicences.tplId = tplId;
                        sicences.level = level;
                        allianceSimple->sciences.push_back(sicences);
                    }
                }
            }
        }

        void Alliance::CleanAlliances()
        {
            m_players.clear();
            m_alliances.clear();
        }

        void Alliance::UpdateAlliance(AllianceSimple alliance, bool isCSUp)
        {
            bool needBroadTroops = false;
            auto it = m_alliances.find(alliance.info.id);
            if (it != m_alliances.end()) {
                m_alliances.erase(it);
            }
            m_alliances.emplace(alliance.info.id, alliance);
            // update member
            for (size_t i = 0; i < alliance.members.size(); ++i) {
                m_players.emplace(alliance.members[i], alliance.info);
            }
            // update to map agent
            if (!isCSUp) {
                needBroadTroops = true;
            }
            auto it_a = m_alliances.find(alliance.info.id);
            AllianceSimple& a = it_a->second;
            for (size_t i = 0; i < a.members.size(); ++i) {
                if (Agent* agent = g_mapMgr->FindAgentByUID(a.members[i])) {
                    agent->OnAllianceInfoUpdate(&a, needBroadTroops);
                }
                // test
                //printf("Alliance::UpdateAlliance ... dump info id %ld, name %s, nickname %s, leaderId %ld, leaderName %s, leaderHid %ld, bannerId %d, castleMax %d\n",
                //        alliance.info.id, alliance.info.name.c_str(), alliance.info.nickname.c_str(), alliance.info.leaderId, alliance.info.leaderName.c_str(), alliance.info.leaderHeadId, alliance.info.bannerId, alliance.info.castleMax);
            }
//             if (isCSUp) {
//                 //绑定萌城
//                 g_mapMgr->UnitsForeach([&](Unit * unit) {
//                     if (NeutralCastle* nc = unit->ToNeutralCastle()) {
//                         if (nc->ownerId() == a.info.id) {
//                             a.neutralCastles.emplace(nc->id(), nc);
//                             nc->NoticeCastleUpdateNcProperty();
//                         }
//                     }
//                 });
//             }
        }

        void Alliance::UpdateAllianceInfo(const AllianceSimpleInfo& info)
        {
            auto it = m_alliances.find(info.id);
            if (it == m_alliances.end()) {
                return;
            }
            bool needBroadTroops = false;
            AllianceSimple& a = it->second;
            a.info.name = info.name;
            string oldNickname = a.info.nickname;
            if (oldNickname != info.nickname) {
                needBroadTroops = true;
            }
            a.info.nickname = info.nickname;
            a.info.leaderId = info.leaderId;
            a.info.leaderName = info.leaderName;
            a.info.leaderHeadId = info.leaderHeadId;
            a.info.bannerId = info.bannerId;
            // update to map agent
            for (size_t i = 0; i < a.members.size(); ++i) {
                if (Agent* agent = g_mapMgr->FindAgentByUID(a.members[i])) {
                    agent->OnAllianceInfoUpdate(&a, needBroadTroops);
                }
            }
            // test
            //printf("Alliance::UpdateAllianceInfo ... dump info id %ld, name %s, nickname %s, leaderId %ld, leaderName %s, leaderHid %ld, bannerId %d, castleMax %d\n",
            //            info.id, info.name.c_str(), info.nickname.c_str(), info.leaderId, info.leaderName.c_str(), info.leaderHeadId, info.bannerId, info.castleMax);
        }

        void Alliance::Disband(int64_t aid)
        {
            /*
            auto it = m_alliances.find(aid);
            if (it == m_alliances.end()) {
                return;
            }
            AllianceSimple& a = it->second;
            // update to map agent
            for (size_t i = 0; i < a.members.size(); ++i) {
                Agent* agent = g_mapMgr->FindAgentByUID(a.members[i]);
                if (agent) {
                    agent->OnAllianceInfoReset(&a);
                }
            }
            // notice all neutral castle
            for (auto itNc = a.neutralCastles.begin(); itNc != a.neutralCastles.end(); ++itNc) {
                NeutralCastle* nc = itNc->second;
                nc->OnAllianceDisband();
            }

            for (uint16_t i = 0; i < a.members.size(); ++i) {
                auto it_p = m_players.find(a.members[i]);
                if (it_p != m_players.end()) {
                    m_players.erase(it_p);
                }
            }
            m_alliances.erase(it);
            */
        }

        void Alliance::AddMember(int64_t aid, int64_t uid)
        {
            auto it = m_alliances.find(aid);
            if (it == m_alliances.end()) {
                return;
            }
            AllianceSimple& a = it->second;
            auto it_p = m_players.find(uid);
            if (it_p != m_players.end()) {
                return;
            }
            a.members.push_back(uid);

            m_players.emplace(uid, a.info);
            // update to map agent
            if (Agent* agent = g_mapMgr->FindAgentByUID(uid)) {
                agent->OnAllianceInfoUpdate(&a, true);
                agent->ClearInvited();
            }
        }

        void Alliance::RemoveMember(int64_t aid, int64_t uid)
        {
            auto it = m_alliances.find(aid);
            if (it == m_alliances.end()) {
                return;
            }

            AllianceSimple& a = it->second;

            for (auto it_m = a.members.begin(); it_m != a.members.end();) {
                if (*it_m == uid) {
                    a.members.erase(it_m);
                    break;
                }
                it_m++;
            }
            auto it_p = m_players.find(uid);
            if (it_p != m_players.end()) {
                m_players.erase(it_p);
            }
            if (Agent* agent = g_mapMgr->FindAgentByUID(uid)) {
                agent->OnAllianceInfoReset(&a);
            }
        }
        
        void Alliance::RecordInvited(int64_t uid)
        {
            if (Agent* agent = g_mapMgr->FindAgentByUID(uid)) {
                agent->AddInvited(uid);
            }
        }

        void Alliance::OnBattleUpdate(BattleInfo* info, bool sync)
        {
            if (info->red.allianceId != 0 && !info->isRedDelete) {
                auto it = m_battles.find(info->red.allianceId);
                if (it != m_battles.end()) {
                    std::list<BattleInfo*>& list = it->second;
                    if (list.size() >= 25) {
                        auto it_b = list.begin();
                        (*it_b)->isRedDelete = true;
                        (*it_b)->isDirty = true;
                        list.erase(it_b);
                    }
                    list.push_back(info);
                } else {
                    std::list<BattleInfo*> list;
                    list.push_back(info);
                    m_battles.emplace(info->red.allianceId, list);
                }
                // update to client
                if (sync) {
                    Agent* agent = g_mapMgr->FindAgentByUID(info->red.uid);
                    if (agent) {
                        agent->OnBattleHistoryUpdate(info);
                    }
                }
            }
            if (info->blue.allianceId != 0 && !info->isBlueDelete) {
                auto it = m_battles.find(info->blue.allianceId);
                if (it != m_battles.end()) {
                    std::list<BattleInfo*>& list = it->second;
                    if (list.size() >= 25) {
                        auto it_b = list.begin();
                        (*it_b)->isBlueDelete = true;
                        (*it_b)->isDirty = true;
                        list.erase(it_b);
                    }
                    list.push_back(info);
                } else {
                    std::list<BattleInfo*> list;
                    list.push_back(info);
                    m_battles.emplace(info->blue.allianceId, list);
                }
                // update to client
                if (sync) {
                    Agent* agent = g_mapMgr->FindAgentByUID(info->blue.uid);
                    if (agent) {
                        agent->OnBattleHistoryUpdate(info);
                    }
                }
            }
        }

        void Alliance::AllianceBuffOpen(int64_t aid, const info::AllianceBuffInfo& info)
        {
            auto as = FindAllianceSimple(aid);
            if (as) {
                as->AllianceBuffOpen(info);
            }
        }

        void Alliance::AllianceBuffClosed(int64_t aid, int buffId)
        {
            auto as = FindAllianceSimple(aid);
            if (as) {
                as->AllianceBuffClosed(buffId);
            }
        }
        
	void Alliance::setAllianceCdTimestamp(int64_t uid, int64_t allianceCdTimestamp)
        {
            if (Agent* agent = g_mapMgr->FindAgentByUID(uid)) {
                agent->setAllianceCdTimestamp(allianceCdTimestamp);
            }
        }

    }
}
