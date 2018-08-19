#!/bin/bash
mysql -uroot -proot << EOF;
use sg_map;
truncate s_agent;  
truncate s_battle_history;  
truncate s_monster_siege;  
truncate s_msg_queue;
truncate s_palace_war_record; 
truncate s_troop;            
truncate s_unit;


use sg_game;
truncate s_activity_ranking; 
truncate s_alliance; 
truncate s_alliance_gift; 
truncate s_alliance_help; 
truncate s_alliance_hero_lease; 
truncate s_alliance_invite;
truncate s_alliance_member; 
truncate s_arena; 
truncate s_arena_record;
truncate s_bag; 
truncate s_charge; 
truncate s_dict; 
truncate s_hero; 
truncate s_incr; 
truncate s_mail; 
truncate s_mail_batch; 
truncate s_marquee; 
truncate s_notice; 
truncate s_quest; 
truncate s_seven_target; 
truncate s_stat;
truncate s_user;
truncate s_report;
truncate s_transport_record;
EOF
