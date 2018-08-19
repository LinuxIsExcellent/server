#include <iostream>
#include <base/framework.h>
#include <base/utils/utils_string.h>
#include "moduletestclient.h"
#include "clientSession.h"
#include <thread>
#include <readline/readline.h>
#include <readline/history.h>
#include <vector>

using namespace std;


void thread_readline()
{

    rl_bind_key('\t', rl_possible_completions);

    add_history("welcome");

    while (true) {
        char* m = readline("> ");
        if (!m) {
            break;
        }
        string cmd = m;
        free(m);
        printf("input command : %s\n", cmd.c_str());
        if (cmd == "exit") {
            break;
        } else {
            vector<string> params = base::utils::string_split(cmd, ' ');
            for (uint16_t i = 0; i < params.size(); ++i) {
                printf("i %u, params %s\n", i, params[i].c_str());
            }
            if (params.size() <= 0) {
                printf("bad cmd.....\n");
                continue;
            }
            if (params[0] == "login") {
                //add_history(cmd.c_str());
                if (params.size() == 2) {
                    printf("login ........ %s\n", params[1].c_str());
                    g_session->SendLogin(params[1]);
                }
            } else if (params[0] == "logout") {
                add_history(cmd.c_str());
            } else if (params[0] == "build_create") {
                add_history(cmd.c_str());
                if (params.size() == 4) {
                    uint16_t grid = 0u;
                    uint32_t tplid = 0u;
                    bool isnow = false;
                    sscanf(params[1].c_str(), "%hu", &grid);
                    sscanf(params[2].c_str(), "%u", &tplid);
                    if (params[3] == "true") {
                        isnow = true;
                    }
                    printf("build_create, grid %u, tplid %u, is new %s", grid, tplid, (isnow ? "true" : "false"));
                    g_session->BuildingCreate(grid, tplid, isnow);
                }
            } else if (params[0] == "build_upgrade") {
                if (params.size() == 3) {

                    uint16_t grid = 0u;
                    bool isnow = false;
                    sscanf(params[1].c_str(), "%hu", &grid);
                    if (params[2] == "true") {
                        isnow = true;
                    }
                    printf("build_create, grid %u, is new %s", grid, (isnow ? "true" : "false"));
                    g_session->BuildingUpgrade(grid, isnow);
                }
            } else if (params[0] == "build_upgrade_cancle") {
                if (params.size() == 2) {
                    uint16_t grid = 0u;
                    sscanf(params[1].c_str(), "%hu", &grid);
                    g_session->BuildingUpgradeCancel(grid);
                }               
            } else if (params[0] == "build_demolish") {
                if (params.size() == 2) {
                    uint16_t grid = 0u;
                    sscanf(params[1].c_str(), "%hu", &grid);
                    g_session->BuildingDemolish(grid);
                }
            } else if (params[0] == "build_demolish_cancle") {
                if (params.size() == 2) {
                    uint16_t grid = 0u;
                    sscanf(params[1].c_str(), "%hu", &grid);
                    g_session->BuildingDemolishCancel(grid);
                }            
            } else if (params[0] == "build_train") {
                if (params.size() == 6) {
                    uint16_t grid = 0u;
                    uint16_t queueIndex = 0u;
                    uint32_t armyId = 0u;
                    uint32_t count = 0u;
                    bool trainNow = false;
                    sscanf(params[1].c_str(), "%hu", &grid);
                    sscanf(params[2].c_str(), "%hu", &queueIndex);
                    sscanf(params[3].c_str(), "%hu", &armyId);
                    sscanf(params[4].c_str(), "%hu", &count);
                    if (params[5] == "true") {
                        trainNow = true;
                    }
                     g_session->BuildingTrain(grid, queueIndex, armyId, count, trainNow);
                }
            } else if (params[0] == "build_train_cancle") {
                if (params.size() == 3) {
                    uint16_t grid = 0u;
                    uint16_t queueIndex = 0u;
                    sscanf(params[1].c_str(), "%hu", &grid);
                    sscanf(params[2].c_str(), "%hu", &queueIndex);
                    g_session->BuildingTrainCancle(grid, queueIndex);
                }
            } else if (params[0] == "build_collect") {
                if (params.size() == 3) {
                    uint16_t grid = 0u;
                    uint16_t index = 0u;
                    sscanf(params[1].c_str(), "%hu", &grid);
                    sscanf(params[2].c_str(), "%hu", &index);
                    g_session->BuildingCollect(grid, index);
                }
                
            } else if (params[0] == "build_technology") {
                if (params.size() == 4) {
                    uint16_t grid = 0u;
                    int tplId = 0u;
                    bool trainNow = false;
                    sscanf(params[1].c_str(), "%hu", &grid);
                    sscanf(params[2].c_str(), "%d", &tplId);
                    if (params[3] == "true") {
                        trainNow = true;
                    }
                     g_session->BuildTechnology(grid, tplId, trainNow);
                }
            } else if (params[0] == "build_technology_cancle") {
                if (params.size() == 3) {
                    uint16_t grid = 0u;
                    int tplId = 0u;
                    sscanf(params[1].c_str(), "%hu", &grid);
                    sscanf(params[2].c_str(), "%d", &tplId);
                     g_session->BuildTechnologyCancle(grid, tplId);
                }
            } else if (params[0] == "quest_draw") {
                if (params.size() == 2) {
                    uint32_t questId = 0;
                    sscanf(params[1].c_str(), "%u", &questId);
                    printf("quest_draw, questid %u\n", questId);
                    g_session->QuestDraw(questId);
                }
            } else if (params[0] == "mail_view") {
                if (params.size() == 3) {
                    int mailType = 0;
                    int64_t mailId = 0;
                    sscanf(params[1].c_str(), "%d", &mailType);
                    sscanf(params[2].c_str(), "%ld", &mailId);
                    //printf("mail_view, mailid %u\n", mailId);
                    g_session->MailView(mailType, mailId);
                }
            } else if (params[0] == "mail_draw") {
                if (params.size() == 4) {
                    int mailType = 0;
                    int64_t mailId = 0;
                    int64_t parentId = 0;
                    sscanf(params[1].c_str(), "%d", &mailType);
                    sscanf(params[2].c_str(), "%ld", &mailId);
                    sscanf(params[3].c_str(), "%ld", &parentId);
                    //printf("mail_draw, mailid %u, parentId %u.\n", mailId, parentId);
                    g_session->MailDraw(mailType, mailId, parentId);
                }
            } else if (params[0] == "mail_delete") {
                if (params.size() == 3) {
                    int mailType = 0;
                    int64_t mailId = 0;
                    sscanf(params[1].c_str(), "%d", &mailType);
                    sscanf(params[2].c_str(), "%ld", &mailId);
                    //printf("mail_delete, mailid %u\n", mailId);
                    g_session->MailDelete(mailType, mailId);
                }
            } else if (params[0] == "mail_private") {
                if (params.size() == 3) {
                    int64_t toUid = 0u;
                    sscanf(params[1].c_str(), "%ld", &toUid);
                    printf("mail_private, to uid %ld, content %s\n", toUid, params[2].c_str());
                    g_session->MailSendPrivate(toUid, params[2]);
                }
            } else if (params[0] == "mail_alliance") {
                if (params.size() == 2) {
                    printf("mail_alliance, content %s\n",  params[1].c_str());
                    g_session->MailSendAlliance(params[1]);
                }
            } else if (params[0] == "mail_share") {
                if (params.size() == 4) {
                    int mailType = 0;
                    int64_t mailId = 0;
                    int64_t parentId = 0;
                    sscanf(params[1].c_str(), "%d", &mailType);
                    sscanf(params[2].c_str(), "%ld", &mailId);
                    sscanf(params[3].c_str(), "%ld", &parentId);
                    //printf("mail_share, mailId %d, parentId %d\n",  mailId, parentId);
                    g_session->MailShare(mailType, mailId, parentId);
                }
            } else if (params[0] == "mail_share_view") {
                if (params.size() == 3) {
                    int64_t uid = 0;
                    int mailId = 0;
                    sscanf(params[1].c_str(), "%ld", &uid);
                    sscanf(params[2].c_str(), "%d", &mailId);

                    printf("mail_share_view, uid %ld, mailId %d,\n", uid, mailId);
                    g_session->MailShareView(uid, mailId);
                }
            } else if (params[0] == "map_join") {
                if (params.size() == 4) {
                    uint32_t k = 0u;
                    uint32_t x = 0u;
                    uint32_t y = 0u;
                    sscanf(params[1].c_str(), "%u", &k);
                    sscanf(params[2].c_str(), "%u", &x);
                    sscanf(params[3].c_str(), "%u", &y);
                    printf("map_join, k %u, x %u, y %u\n", k, x, y);
                    g_session->MapJoin(k, x, y);
                }
            } else if (params[0] == "map_leave") {
                if (params.size() == 1) {
                    printf("map_leave ... \n");
                    g_session->MapLeave();
                }
            } else if (params[0] == "map_view") {
                if (params.size() == 3) {
                    //uint32_t k = 0u;
                    uint32_t x = 0u;
                    uint32_t y = 0u;
                    //sscanf(params[1].c_str(), "%u", &k);
                    sscanf(params[1].c_str(), "%u", &x);
                    sscanf(params[2].c_str(), "%u", &y);
                    printf("map_view, x %u, y %u\n", x, y);
                    g_session->MapView(x, y);
                }
            } else if (params[0] == "map_remarch") {
                if (params.size() == 5) {
                    uint32_t troopId = 0u;
                    uint32_t t = 0u;
                    uint32_t x = 0u;
                    uint32_t y = 0u;
                    sscanf(params[1].c_str(), "%u", &troopId);
                    sscanf(params[2].c_str(), "%u", &t);
                    sscanf(params[3].c_str(), "%u", &x);
                    sscanf(params[4].c_str(), "%u", &y);
                    printf("map_remarch, id %d, t %u, x %u, y %u\n",troopId, t, x, y);
                    g_session->MapReMarch(troopId, t, x, y);
                }
            }else if (params[0] == "map_march") {
                if (params.size() == 4) {
                    uint32_t t = 0u;
                    uint32_t x = 0u;
                    uint32_t y = 0u;
                    sscanf(params[1].c_str(), "%u", &t);
                    sscanf(params[2].c_str(), "%u", &x);
                    sscanf(params[3].c_str(), "%u", &y);
                    printf("map_march, t %u, x %u, y %u\n", t, x, y);
                    g_session->MapMarch(t, x, y);
                }
            } else if (params[0] == "chat_k") {
                if (params.size() == 2) {
                    printf("chat_kingdom, content %s \n", params[1].c_str());
                    g_session->ChatKingdom(params[1]);
                }
            } else if (params[0] == "chat_a") {
                if (params.size() == 2) {
                    printf("chat_alliance, content %s \n", params[1].c_str());
                    g_session->ChatAlliance(params[1]);
                }
            } else if (params[0] == "chat_block") {
                if (params.size() == 2) {
                    printf("chat_block, aid %s \n", params[1].c_str());
                    int64_t aid = 0;
                    sscanf(params[1].c_str(), "%ld", &aid);
                    g_session->ChatBlock(aid);
                }
            } else if (params[0] == "chat_unblock") {
                if (params.size() == 2) {
                    printf("chat_unblock, aid %s \n", params[1].c_str());
                    int64_t aid = 0;
                    sscanf(params[1].c_str(), "%ld", &aid);
                    g_session->ChatUnblock(aid);
                }
            } else if (params[0] == "range_fetch") {
                if (params.size() == 2) {
                    printf("range_fetch, type %s\n", params[1].c_str());
                    int type = 0;
                    sscanf(params[1].c_str(), "%d", &type);
                    g_session->RangeFetch(type);
                }
            } else if (params[0] == "misc_sign") {
                g_session->Sign();
            } else if (params[0] == "misc_drawonline") {
                g_session->DrawOnline();
            } else if(params[0] == "turnplate") {
                g_session->Turnplate();
            } else if(params[0] == "turnplate_records") {
                g_session->TurnplateRecords();
            } else if (params[0] == "skill_add") {
                if (params.size() == 3) {
                    int64_t tplId = 0;
                    bool isFull = false;
                    sscanf(params[1].c_str(), "%ld", &tplId);
                    if (params[2] == "true") {
                        isFull = true;
                    }
                    g_session->SkillAdd(tplId, isFull);
                    
                }
            } else if (params[0] == "hero_draw") {
                if (params.size() == 4) {
                    int drawType = 0;
                    bool isTen = false;
                    int64_t bid = 0;
                    sscanf(params[1].c_str(), "%d", &drawType);
                    if (params[2] == "true") {
                        isTen = true;
                    }
                    sscanf(params[1].c_str(), "%ld", &bid);
                    g_session->HeroDraw(drawType, isTen, bid);

                }
            } else if (params[0] == "team_save") {
                if (params.size() == 5) {
                    int team = 0;
                    int troopType = 0;
                    int x = 0;
                    int y = 0;
                    sscanf(params[1].c_str(), "%d", &team);
                    sscanf(params[2].c_str(), "%d", &troopType);
                    sscanf(params[3].c_str(), "%d", &x);
                    sscanf(params[4].c_str(), "%d", &y);
                    g_session->TeamSave(team, troopType, x, y);
                }
            } else if (params[0] == "team_remove") {
                if (params.size() == 2) {
                    int team = 0;
                    sscanf(params[1].c_str(), "%d", &team);
                    g_session->TeamRemove(team);
                }
            } else if (params[0] == "bag_synthe") {
                if (params.size() == 2) {
                    int64_t bid = 0u;
                    sscanf(params[1].c_str(), "%ld", &bid);
                    g_session->BagSynthe(bid);
                }
            } else if (params[0] == "hero_puton") {
                if (params.size() == 5) {
                    int type = 0;
                    int64_t bid = 0u;
                    int heroId = 0;
                    int slot = 0;
                    sscanf(params[1].c_str(), "%d", &type);
                    sscanf(params[2].c_str(), "%ld", &bid);
                    sscanf(params[3].c_str(), "%d", &heroId);
                    sscanf(params[4].c_str(), "%d", &slot);
                    
                    g_session->HeroPutOn(type, bid, heroId, slot);
                }
            } else if (params[0] == "hero_takeoff") {
                if (params.size() == 4) {
                    int type = 0;
                    int heroId = 0;
                    int slot = 0;
                    sscanf(params[1].c_str(), "%d", &type);
                    sscanf(params[2].c_str(), "%d", &heroId);
                    sscanf(params[3].c_str(), "%d", &slot);
                    
                    g_session->HeroTakeOff(type, heroId, slot);
                }
            } else if (params[0] == "hero_star_upgrage") {
                if (params.size() == 3) {
                    int heroId = 0;
                    int64_t bid = 0u;
                    sscanf(params[1].c_str(), "%d", &heroId);
                    sscanf(params[2].c_str(), "%ld", &bid);
                    g_session->HeroStarUpgrage(heroId, bid);
                }
                
            } else if (params[0] == "hero_skill_upgrage") {
                if (params.size() == 4) {
                    int heroId = 0;
                    int slot = 0;
                    int64_t bid = 0u;
                    sscanf(params[1].c_str(), "%d", &heroId);
                    sscanf(params[2].c_str(), "%d", &slot);
                    sscanf(params[3].c_str(), "%ld", &bid);
                    g_session->HeroSkillUpgrage(heroId, slot, bid); 
                }
                
            } else if (params[0] == "hero_equip_upgrage") {
                if (params.size() == 4) {
                    int heroId = 0;
                    int slot = 0;
                    int64_t bid = 0u;
                    sscanf(params[1].c_str(), "%d", &heroId);
                    sscanf(params[2].c_str(), "%d", &slot);
                    sscanf(params[3].c_str(), "%ld", &bid);
                    g_session->HeroEquipUpgrage(heroId, slot, bid); 
                }
                
            } else if(params[0] == "hero_equip_succinct") {
                if (params.size() == 4) {
                    int heroId = 0;
                    int slot = 0;
                    int locate = 0u;
                    sscanf(params[1].c_str(), "%d", &heroId);
                    sscanf(params[2].c_str(), "%d", &slot);
                    sscanf(params[3].c_str(), "%d", &locate);
                    g_session->HeroEquipSuccinct(heroId, slot, locate);
                }
                
            } else if (params[0] == "hero_equip_inlay_on") {
                if (params.size() == 5) {
                    int heroId = 0;
                    int slot = 0;
                    int locate = 0u;
                    int64_t bid = 0u;
                    sscanf(params[1].c_str(), "%d", &heroId);
                    sscanf(params[2].c_str(), "%d", &slot);
                    sscanf(params[3].c_str(), "%d", &locate);
                    sscanf(params[4].c_str(), "%ld", &bid);
                    g_session->HeroEquipInlayOn(heroId, slot, locate, bid);
                }
                
            } else if (params[0] == "hero_equip_inlay_off") {
                if (params.size() == 4) {
                    int heroId = 0;
                    int slot = 0;
                    int locate = 0u;
                    sscanf(params[1].c_str(), "%d", &heroId);
                    sscanf(params[2].c_str(), "%d", &slot);
                    sscanf(params[3].c_str(), "%d", &locate);
                    g_session->HeroEquipInlayOff(heroId, slot, locate);
                }
            } else if(params[0] == "bag_sell") {
                if (params.size() == 3) {
                    int64_t bid = 0u;
                    int count = 0;
                    sscanf(params[1].c_str(), "%ld", &bid);
                    sscanf(params[2].c_str(), "%d", &count);
                    
                    g_session->BagSell(bid, count);
                }
            } else if(params[0] == "battle_start") {
                if (params.size() == 2) {
                    int teamId = 0;
                    sscanf(params[1].c_str(), "%d", &teamId);
                    
                    g_session->BattleStart(teamId);
                }
            } else if(params[0] == "battle_over") {
                if (params.size() == 2) {
                    int battleId = 0;
                    sscanf(params[1].c_str(), "%d", &battleId);
                    
                    g_session->BattleOver(battleId);
                }
            } else if(params[0] == "scenario_mopup") {
                if (params.size() == 4) {
                    int chapterId = 0;
                    int sectionId = 0;
                    int count = 0;
                    sscanf(params[1].c_str(), "%d", &chapterId);
                    sscanf(params[2].c_str(), "%d", &sectionId);
                    sscanf(params[3].c_str(), "%d", &count);
                    g_session->ScenarioMmopup(chapterId, sectionId, count);
                }
            } else if(params[0] == "scenario_drew") {
                if (params.size() == 3) {
                    int chapterId = 0;
                    int target = 0;
                    sscanf(params[1].c_str(), "%d", &chapterId);
                    sscanf(params[2].c_str(), "%d", &target);
                    g_session->ScenarioDrew(chapterId, target);
                }
            } else if(params[0] == "scenario_fight") {
                if (params.size() == 3) {
                    int chapterId = 0;
                    int sectionId = 0;
                    sscanf(params[1].c_str(), "%d", &chapterId);
                    sscanf(params[2].c_str(), "%d", &sectionId);
                    g_session->ScenarioFight(chapterId, sectionId);
                }
            } else if(params[0] == "scenario_answer") {
                if (params.size() == 4) {
                    int chapterId = 0;
                    int sectionId = 0;
                    int count = 0;
                    sscanf(params[1].c_str(), "%d", &chapterId);
                    sscanf(params[2].c_str(), "%d", &sectionId);
                    sscanf(params[3].c_str(), "%d", &count);
                    g_session->ScenarioAnswer(chapterId, sectionId, count);
                }
            } else if (params[0] == "taketax") {
                if (params.size() == 2) {
                    int gold = 0;
                    sscanf(params[1].c_str(), "%d", &gold);
                    bool isGoldTax = gold > 0;
                    g_session->TakeTax(isGoldTax);
                }
            } else if(params[0] == "hbattle_start") {
                if (params.size() == 1) {
                    g_session->HeroBattleStart();
                }
            } else if(params[0] == "hbattle_over") {
                if (params.size() == 2) {
                    int battleId = 0;
                    sscanf(params[1].c_str(), "%d", &battleId);

                    g_session->HeroBattleOver(battleId);
                }
            } else if(params[0] == "hire_hero") {
                if (params.size() == 2) {
                    int heroId = 0;
                    sscanf(params[1].c_str(), "%d", &heroId);
                    g_session->AillanceHireHero(heroId);
                }
            } else if(params[0] == "recall_hero") {
                if (params.size() == 2) {
                    int heroId = 0;
                    sscanf(params[1].c_str(), "%d", &heroId);
                    g_session->AillanceRecallHero(heroId);
                }
            } else if(params[0] == "employ_hero") {
                if (params.size() == 3) {
                    int64_t ownerUid = 0;
                    int heroId = 0;
                    sscanf(params[1].c_str(), "%ld", &ownerUid);
                    sscanf(params[2].c_str(), "%d", &heroId);
                    g_session->AillanceEmployHero(ownerUid, heroId);
                }
            } else if (params[0] == "map_search") {
                if (params.size() == )

            } 
        }
    }

    exit(0);
}

int main(int argc, char* argv[])
{
    base::ServerRule rule("fs", 1);
    if (!framework.AutoSetup(rule, "/opt/sg/server/resource")) {
        return -1;
    }
    std::string ip = "127.0.0.1";
//     std::string ip = "192.168.1.55";
    int port = 8100;
    if (argc >= 2) {
        ip = argv[1];
    }
    if (argc >= 3) {
        port = atoi(argv[2]);
    }
    framework.RegisterModule(ModuleTestClient::Create(ip, port));

    thread t(thread_readline);

    return framework.Run();
}
