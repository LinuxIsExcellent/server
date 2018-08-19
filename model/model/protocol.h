#ifndef MODEL_PROTOCOL_H
#define MODEL_PROTOCOL_H

namespace model
{
    enum class CS {
        TEMPLATE_CHECK = 1,
        LOGIN = 5,
        PING_REPLY = 9,
        SET_OR_CHECK_NICKNAME = 20,
        SET_LANG_TYPE = 22,
        STAMINA_BUY = 24,
        GET_RANDOM_NAME = 32,
        SET_NAME = 34,
        FILL_RESOURCE = 36,
        ENERGY_BUY = 39,
        BAG_USE = 51,
        BAG_BUY = 53,
        BAG_SELL = 55,
        BAG_SYNTHESIZE = 57,
        BAG_MELT = 59,
        BUILDING_CREATE = 202,
        BUILDING_UPGRADE = 204,
        BUILDING_UPGRADE_CANCEL = 206,
        BUILDING_TRAIN = 208,
        BUILDING_HEAL = 222,
        BUILDING_HEAL_CANCEL = 224,
        BUILDING_TRAIN_CANCEL = 210,
        BUILDING_COLLECT = 214,
        BUILDING_COLLECT_BOOST = 216,
        BUILDING_DEMOLISH = 218,
        BUILDING_DEMOLISH_CANCEL = 220,
        BUILDING_OPEN_BUILDER2 = 226,
        BUILDING_OPEN_FOREST = 228,
        BUILDING_MOVE = 230,
        BUILDING_TECHNOLOGY_RESEARCH = 232,
        BUILDING_TECHNOLOGY_RESEARCH_CANCEL = 234,
        TAKE_TAX = 250,
        CDLIST_SPEED_UP = 302,
        SAVE_TEAM_INFO = 354,
        ARMY_FILL = 358,
        SET_CASTLE_DEFENDER = 360,
        DESTROY_TRAP = 362,
        SKILL_RESET = 402,
        SKILL_ADD_POINT = 404,
        SKILL_ACTIVE_USE = 407,
        MAIL_VIEW = 502,
        MAIL_DELETE = 504,
        MAIL_DRAW_ATTACHMENT = 506,
        MAIL_ALLIANCE_SEND = 508,
        MAIL_PRIVATE_SEND = 510,
        MAIL_ALLIANCE_SHARE = 512,
        MAIL_ALLIANCE_SHARE_VIEW = 513,
        QUEST_DRAW = 551,
        STORY_QUEST_DRAW = 556,
        DAILY_TASK_REWARD_DRAW = 562,
        DAILY_TASK_DRAW = 564,
        VIP_TAKE_BOX = 621,
        ACHIEVEMENT_DRAW = 651,
        SET_MEDAL = 653,
        MISC_SIGN = 701,
        MISC_DRAW_ONLINE = 704,
        MISC_TURNPLATE_DRAW = 710,
        MISC_TURNPLATE_RECORDS_FETCH = 713,
        MISC_DRAW_CODE = 720,
        PLAYER_SET = 724,
        ACTIVITY_LOGIN_NEXT_DAY_DRAW = 753,
        ACTIVITY_ACCUMULATE_LOGIN_DRAW = 757,
        ACTIVITY_LOGIN_EVERYDAY_DRAW = 761,
        ACTIVITY_TIME_MEAL_DRAW = 765,
        ACTIVITY_TARGET_DRAW = 769,
        ACTIVITY_EXCHANGE_EXCHANGE = 773,
        SEVEN_TARGET_DRAW = 851,
        PUSH_SETTING_SET = 901,
        PUSH_DEVICE_SYNC = 902,
        TOWER_FIGHT = 954,
        TOWER_RESET = 958,
        TOWER_RANKING_GET = 960,
        TOWER_MOPUP = 962,
        MAP_VIEW = 1000,
        MAP_SWITCH = 1005,
        MAP_JOIN = 1010,
        MAP_LEAVE = 1012,
        MAP_TELEPORT = 1014,
        MAP_MARCH = 1016,
        MAP_GO_HOME = 1018,
        MAP_RECALL = 1022,
        MAP_SPEED_UP = 1024,
        MAP_QUERY_PLAYERINFO = 1028,
        MAP_WALL_REPAIR = 1030,
        MAP_WALL_OUTFIRE = 1032,
        MAP_DECLARE_CANCEL = 1034,
        MAP_REPATRIATE = 1036,
        MAP_ALLIANCE_INVITE_LIST = 1038,
        MAP_ALLIANCE_INVITE_LIST_BY_NAME = 1040,
        MAP_GET_ALLIES_EXTRA_INFOS = 1042,
        MAP_ALLIANCE_UNIT_TELEPORT = 1044,
        MAP_NEUTRAL_CASTLE_REPATRIATE = 1046,
        MAP_GET_ALLIANCE_TERRITORY_LIST = 1048,
        MAP_GET_ALLIANCE_TERRITORY_INFO = 1050,
        MAP_KINGMAP_UNIT_POSITION = 1056,
        MAP_KINGDOM_INFO = 1058,
        MAP_NEUTRAL_CASTLE_CHANGE_NAME = 1060,
        MAP_GET_NEAREST_UNIT = 1062,
        MAP_GET_TROOP_EXTRA_INFO = 1064,
        MAP_GET_MARCH_DISTANCE = 1068,
        MAP_CHECK_UNIT_CAN_PLACE = 1070,
        MAP_GET_MYSTERIOUS_CITY_INFO = 1072,
        MAP_GET_GOBLIN_CAMP_INFO = 1074,
        MAP_RECALL_ALLIANCE_TROOP = 1078,
        BOOKMARK_ADD = 1081,
        BOOKMARK_EDIT = 1083,
        BOOKMARK_REMOVE = 1085,
        MAP_SEARCH = 1087,
        ALLIANCE_CHECK_NAME = 1210,
        ALLIANCE_CREATE = 1212,
        ALLIANCE_SEARCH = 1214,
        ALLIANCE_SEARCH_BY_ID = 1216,
        ALLIANCE_APPLY = 1217,
        ALLIANCE_APPLY_ACCEPT = 1219,
        ALLIANCE_INVITE = 1221,
        ALLIANCE_INVITE_ACCEPT = 1223,
        ALLIANCE_QUIT = 1225,
        ALLIANCE_KICK_MEMBER = 1227,
        ALLIANCE_BANNER_CHANGE = 1229,
        ALLIANCE_ANNOUNCEMENT_CHANGE = 1231,
        ALLIANCE_RANK_UP = 1233,
        ALLIANCE_RANK_DOWN = 1235,
        ALLIANCE_LEADER_TRANSFER = 1237,
        ALLIANCE_LEADER_REPLACE = 1239,
        ALLIANCE_SCIENCE_DONATE = 1241,
        ALLIANCE_HELP_REQUEST = 1243,
        ALLIANCE_HELP_ONE = 1245,
        ALLIANCE_HELP_ALL = 1246,
        ALLIANCE_USER_HIRE_HERO = 1252,
        ALLIANCE_USER_RECALL_HERO = 1254,
        ALLIANCE_USER_EMPLOY_HERO = 1256,
        ALLIANCE_COLLECT_HERO_HIRE_COST = 1258,
        ALLIANCE_BUFF_OPEN = 1263,
        ALLIANCE_BUFF_CLOSED = 1265,
        CHAT_SEND = 1400,
        CHAT_BLOCK = 1403,
        CHAT_UNBLOCK = 1405,
        CHAT_GM = 1410,
        RANGE_FETCH = 1450,
        NOVICE_SET = 1502,
        GET_SERVER_INFOS = 1552,
        MONTH_CARD_DRAW = 1564,
        HERO_DRAW = 2003,
        HERO_STAR_UPGRADE = 2009,
        HERO_SKILL_UPGRADE = 2011,
        HERO_DEBRIS_TURN = 2021,
        HERO_CHANGE_TECHNICAL_SKILL = 2023,
        HERO_QUICK_RECOVER = 2025,
        HERO_SOUL_UPGRADE = 2027,
        HERO_UPGRADE = 2029,
        SCENARIO_FIGHT = 2104,
        SCENARIO_MOPUP = 2106,
        SCENARIO_STAR_DRAW = 2108,
        SCENARIO_ANSWER = 2110,
        SCENARIO_RESET = 2111,
        STORE_BUY = 2205,
        STORE_REFRESH = 2207,
        ARENA_CHANGE_OPPONENT = 2309,
        ARENA_FIGHT = 2311,
        ARENA_SET_DEFENSE = 2313,
        ARENA_DREW_REWARD = 2315,
        ARENA_BATTLE_RECORD = 2319,
        ARENA_BATTLE_DETAILS = 2321,
        ARENA_CLEAR_BATTLE_CD = 2323,
        ARENA_RESET_BATTLE_COUNT = 2325,
        ARENA_REVENGE = 2327,
        BABEL_FIGHT = 2410,
        BABEL_MOPUP = 2412,
        BABEL_RESET = 2414,
        BRONZE_SPARROW_TOWER_START = 2509,
        BRONZE_SPARROW_TOWER_ANSWER = 2511,
        BRONZE_SPARROW_TOWER_RECIVE_REWARD = 2513,
        RIDING_ALONE_FIGHT = 2602,
        RIDING_ALONE_RESET = 2604,
        BATTLE_START = 3000,
        BATTLE_OVER = 3002,
        HERO_BATTLE_START = 3004,
        HERO_BATTLE_OVER = 3006,
        PALACE_WAR_SUCCESSIVE_KINGS = 1600,
        PALACE_WAR_RECORDS = 1602,
        KINGDOM_TITLE_LIST = 1604,
        KING_CHOOSED_BY_ALLIANCE_LEADER = 1606,
        KING_GIVE_TITLE = 1608,
        KING_CANCEL_TITLE = 1610,
        KING_GET_KINGDOM_SET_INFOS = 1612,
        KING_CHANGE_SERVER_NAME = 1614,
        KING_CHANGE_RESOURCE_NODE_RATE_ID = 1616,
        KING_GET_GIFT_GIVEN_LIST = 1618,
        KING_GIVE_GIFT = 1620,
        KING_GET_PLAYER_LIST_WHEN_GIVE = 1622,
        KING_GET_GIFT_LEFT_COUNT = 1627,
        GET_REPORT = 1629,
        GET_ALLIANCE_CITIES = 1631,
        TRANSPORT = 1638,
        STORY_ID_UPDATE = 1640,
    };

    enum class SC {
        TEMPLATE_UPDATE = 2,
        LOGIN_RESPONSE = 6,
        LOGOUT = 7,
        PING = 8,
        PING_RESULT = 10,
        READY = 11,
        SET_OR_CHECK_NICKNAME_RESPONSE = 21,
        SET_LANG_TYPE_RESPONSE = 23,
        STAMINA_BUY_RESPONSE = 25,
        NOTICE_MESSAGE = 26,
        USER_UPGRADE_DATA = 29,
        USER_UPDATE = 30,
        ATTR_PLUS_UPDATE = 31,
        GET_RANDOM_NAME_RESPONSE = 33,
        SET_NAME_RESPONSE = 35,
        FILL_RESOURCE_RESPONSE = 37,
        IS_SET_NAME = 38,
        ENERGY_BUY_RESPONSE = 40,
        HERO_ATTR_PLUS_UPDATE = 41,
        BAG_UPDATE = 50,
        BAG_USE_RESPONSE = 52,
        BAG_BUY_RESPONSE = 54,
        BAG_SELL_RESPONSE = 56,
        BAG_SYNTHESIZE_RESPONSE = 58,
        BAG_MELT_RESPONSE = 60,
        BUILDING_UPDATE = 201,
        BUILDING_CREATE_RESPONSE = 203,
        BUILDING_UPGRADE_RESPONSE = 205,
        BUILDING_UPGRADE_CANCEL_RESPONSE = 207,
        BUILDING_TRAIN_RESPONSE = 209,
        BUILDING_HEAL_RESPONSE = 223,
        BUILDING_HEAL_CANCEL_RESPONSE = 225,
        BUILDING_TRAIN_CANCEL_RESPONSE = 211,
        BUILDING_QUEUES_UPDATE = 212,
        BUILDING_COLLECT_RESPONSE = 215,
        BUILDING_COLLECT_BOOST_RESPONSE = 217,
        BUILDING_DEMOLISH_RESPONSE = 219,
        BUILDING_DEMOLISH_CANCEL_RESPONSE = 221,
        BUILDING_OPEN_BUILDER2_RESPONSE = 227,
        BUILDING_OPEN_FOREST_RESPONSE = 229,
        BUILDING_MOVE_RESPONSE = 231,
        BUILDING_TECHNOLOGY_RESEARCH_RESPONSE = 233,
        BUILDING_TECHNOLOGY_RESEARCH_CANCEL_RESPONSE = 235,
        TECHNOLOGY_TREE_UPDATE = 236,
        TAKE_TAX_RESPONSE = 251,
        TAX_UPDATE = 252,
        CDLIST_UPDATE = 301,
        CDLIST_SPEED_UP_RESPONSE = 303,
        ARMY_INFOS_UPDATE = 351,
        TEAM_INFOS_UPDATE = 353,
        SAVE_TEAM_INFO_RESPONSE = 355,
        ARMY_FILL_RESPONSE = 359,
        SET_CASTLE_DEFENDER_RESPONSE = 361,
        DESTROY_TRAP_RESPONSE = 363,
        SKILL_UPDATE = 401,
        SKILL_RESET_RESPONSE = 403,
        SKILL_ADD_POINT_RESPONSE = 405,
        SKILL_ACTIVE_UPDATE = 406,
        SKILL_ACTIVE_USE_RESPONSE = 408,
        TECHNOLOGY_INFOS_UPDATE = 451,
        MAIL_UPDATE = 501,
        MAIL_VIEW_RESPONSE = 503,
        MAIL_DELETE_RESPONSE = 505,
        MAIL_DRAW_ATTACHMENT_RESPONSE = 507,
        MAIL_ALLIANCE_SEND_RESPONSE = 509,
        MAIL_PRIVATE_SEND_RESPONSE = 511,
        MAIL_ALLIANCE_SHARE_VIEW_REPONSE = 514,
        QUEST_UPDATE = 550,
        QUEST_DRAW_RESPONSE = 552,
        STORY_QUEST_UPDATE = 555,
        STORY_QUEST_DRAW_RESPONSE = 557,
        DAILY_TASK_UPDATE = 560,
        DAILY_TASK_REWARD_UPDATE = 561,
        DAILY_TASK_REWARD_DRAW_RESPONSE = 563,
        DAILY_TASK_DRAW_RESPONSE = 565,
        BUFF_LIST_UPDATE = 601,
        BUFF_SKILL_UPDATE = 602,
        VIP_UPDATE = 620,
        VIP_TAKE_BOX_RESPONSE = 622,
        ACHIEVEMENT_UPDATE = 650,
        ACHIEVEMENT_DRAW_RESPONSE = 652,
        SET_MEDAL_RESPONSE = 654,
        MISC_SIGN_UPDATE = 700,
        MISC_SIGN_RESPONSE = 702,
        MISC_ONLINE_UPDATE = 703,
        MISC_DRAW_ONLINE_RESPONSE = 705,
        MISC_TURNPLATE_DATA_UPDATE = 709,
        MISC_TURNPLATE_DRAW_RESPONSE = 711,
        MISC_TURNPLATE_REWARD_UPDATE = 712,
        MISC_TURNPLATE_RECORDS_FETCH_RESPONSE = 714,
        MISC_DRAW_CODE_RESPONSE = 721,
        PLAYER_SET_UPDATE = 723,
        PLAYER_SET_RESPONSE = 725,
        ACTIVITY_RECORD_UPDATE = 750,
        ACTIVITY_LOGIN_NEXT_DAY_INFO_UPDATE = 752,
        ACTIVITY_LOGIN_NEXT_DAY_DRAW_RESPONSE = 754,
        ACTIVITY_ACCUMULATE_LOGIN_INFO_UPDATE = 756,
        ACTIVITY_ACCUMULATE_LOGIN_DRAW_RESPONSE = 758,
        ACTIVITY_LOGIN_EVERYDAY_INFO_UPDATE = 760,
        ACTIVITY_LOGIN_EVERYDAY_DRAW_RESPONSE = 762,
        ACTIVITY_TIME_MEAL_INFO_UPDATE = 764,
        ACTIVITY_TIME_MEAL_DRAW_RESPONSE = 766,
        ACTIVITY_TARGET_INFO_UPDATE = 768,
        ACTIVITY_TARGET_DRAW_RESPONSE = 770,
        ACTIVITY_EXCHANGE_INFO_UPDATE = 772,
        ACTIVITY_EXCHANGE_EXCHANGE_RESPONSE = 774,
        GMS_SEND_MARQUEE = 800,
        GMS_NOTICE_UPDATE = 801,
        SEVEN_TARGET_UPDATE = 850,
        PUSH_SETTING_UPDATE = 900,
        TOWER_PUPPET_INFO_UPDATE = 951,
        TOWER_INFO_UPDATE = 953,
        TOWER_FIGHT_RESPONSE = 955,
        TOWER_BATTLE_RESULT = 957,
        TOWER_RESET_RESPONSE = 959,
        TOWER_RANKING_GET_RESPONSE = 961,
        TOWER_MOPUP_RESPONSE = 963,
        MAP_UNIT_UPDATE = 1001,
        MAP_TROOP_UPDATE = 1002,
        MAP_PERSONAL_INFO_UPDATE = 1003,
        MAP_TELEPORT_RESPONSE = 1015,
        MAP_MARCH_RESPONSE = 1017,
        MAP_UNIT_BATTLE = 1027,
        MAP_QUERY_PLAYERINFO_RESPONSE = 1029,
        MAP_ALLIANCE_INVITE_LIST_RESPONSE = 1039,
        MAP_GET_ALLIES_EXTRA_INFOS_RESPONSE = 1043,
        MAP_ALLIANCE_UNIT_TELEPORT_RESPONSE = 1045,
        MAP_GET_ALLIANCE_TERRITORY_LIST_RESPONSE = 1049,
        MAP_GET_ALLIANCE_TERRITORY_INFO_RESPONSE = 1051,
        MAP_MONSTER_SIEGE = 1053,
        MAP_MONSTER_SIEGE_INFO_UPDATE = 1055,
        MAP_KINGMAP_UNIT_POSITION_RESPONSE = 1057,
        MAP_KINGDOM_INFO_RESPONSE = 1059,
        MAP_NEUTRAL_CASLTE_CHANGE_NAME_RESPONSE = 1061,
        MAP_GET_NEAREST_UNIT_RESPONSE = 1063,
        MAP_GET_TROOP_EXTRA_INFO_RESPONSE = 1065,
        MAP_NEUTRAL_CASTLE_PROPERTY_UPDATE = 1067,
        MAP_GET_MARCH_DISTANCE_RESPONSE = 1069,
        MAP_CHECK_UNIT_CAN_PLACE_RESPONSE = 1071,
        MAP_GET_MYSTERIOUS_CITY_INFO_RESPONSE = 1073,
        MAP_GET_GOBLIN_CAMP_INFO_RESPONSE = 1075,
        PATROL_INFO_UPDATE = 1076,
        MAP_SEND_REINFORCEMENTS = 1077,
        MAP_RECALL_ALLIANCE_TROOP_RESPONSE = 1079,
        BOOKMARK_UPDATE = 1080,
        BOOKMARK_ADD_RESPONSE = 1082,
        BOOKMARK_EDIT_RESPONSE = 1084,
        BOOKMARK_REMOVE_RESPONSE = 1086,
        MAP_SEARCH_RESPONSE = 1088,
        ALLIANCE_INFO_UPDATE = 1200,
        ALLIANCE_MEMBER_INFO_UPDATE = 1201,
        ALLIANCE_APPLY_INFO_UPDATE = 1202,
        ALLIANCE_INVITED_INFO_UPDATE = 1203,
        ALLIANCE_SCIENCE_UPDATE = 1204,
        ALLIANCE_SCORE_UPDATE = 1205,
        ALLIANCE_PERSONAL_UPDATE = 1206,
        ALLIANCE_DONATE_ITEMS_UPDATE = 1207,
        ALLIANCE_HELP_UPDATE = 1208,
        ALLIANCE_REQUEST_HELP = 1209,
        ALLIANCE_CHECK_NAME_RESPONSE = 1211,
        ALLIANCE_CREATE_RESPONSE = 1213,
        ALLIANCE_SEARCH_RESPONSE = 1215,
        ALLIANCE_APPLY_RESPONSE = 1218,
        ALLIANCE_APPLY_ACCEPT_RESPONSE = 1220,
        ALLIANCE_INVITE_RESPONSE = 1222,
        ALLIANCE_INVITE_ACCEPT_RESPONSE = 1224,
        ALLIANCE_QUIT_RESPONSE = 1226,
        ALLIANCE_KICK_MEMBER_RESPONSE = 1228,
        ALLIANCE_BANNER_CHANGE_RESPONSE = 1230,
        ALLIANCE_ANNOUNCEMENT_CHANGE_RESPONSE = 1232,
        ALLIANCE_RANK_UP_RESPONSE = 1234,
        ALLIANCE_RANK_DOWN_RESPONSE = 1236,
        ALLIANCE_LEADER_TRANSFER_RESPONSE = 1238,
        ALLIANCE_LEADER_REPLACE_RESPONSE = 1240,
        ALLIANCE_SCIENCE_DONATE_RESPONSE = 1242,
        ALLIANCE_HELP_REQUEST_RESPONSE = 1244,
        ALLIANCE_HIRE_HERO_UPDATE = 1250,
        ALLIANCE_USER_EMPLOY_HERO_UPDATE = 1251,
        ALLIANCE_USER_HIRE_HERO_RESPONSE = 1253,
        ALLIANCE_USER_RECALL_HERO_RESPONSE = 1255,
        ALLIANCE_USER_EMPLOY_HERO_RESPONSE = 1257,
        ALLIANCE_COLLECT_HERO_HIRE_COST_RESPONSE = 1259,
        ALLIANCE_RECORD_UPDATE = 1260,
        ALLIANCE_HERO_LEASE_RECORD_UPDATE = 1261,
        ALLIANCE_BUFF_UPDATE = 1262,
        ALLIANCE_BUFF_OPEN_RESPONSE = 1264,
        ALLIANCE_BUFF_CLOSED_RESPONSE = 1266,
        ALLIANCE_HERO_HIRE_COLLECT_SILVER_UPDATE = 1270,
        ALLIANCE_HERO_HIRE_TIME_SIVLER_UPDATE = 1271,
        CHAT_RECV = 1401,
        CHAT_BLOCK_UPDATE = 1402,
        CHAT_BLOCK_RESPONSE = 1404,
        CHAT_UNBLOCK_RESPONSE = 1406,
        CHAT_GM_RESPONSE = 1411,
        RANGE_FETCH_RESPONSE = 1451,
        NOVICE_UPDATE = 1501,
        TIME_SYNC = 1550,
        AGENT_INIT_COMPLETE = 1551,
        GET_SERVER_INFOS_RESPONSE = 1553,
        CHARGE_OPTIONS = 1560,
        CHARGE_BUY_UPDATE = 1561,
        CHARGE_RESPONSE = 1562,
        MONTH_CARD_UPDATE = 1563,
        MONTH_CARD_DRAW_RESPONSE = 1565,
        HERO_UPDATE = 2000,
        HERO_SKILL_UPDATE = 2001,
        HERO_DRAW_DATA_UPDATE = 2002,
        HERO_DRAW_RESPONSE = 2004,
        HERO_STAR_UPGRADE_RESPONSE = 2010,
        HERO_SKILL_UPGRADE_RESPONSE = 2012,
        HERO_DEBRIS_TURN_RESPONSE = 2022,
        HERO_CHANGE_TECHNICAL_SKILL_RESPONSE = 2024,
        HERO_QUICK_RECOVER_RESPONSE = 2026,
        HERO_SOUL_UPGRADE_RESPONSE = 2028,
        HERO_UPGRADE_RESPONSE = 2030,
        HERO_SOUL_UPDATE = 2031,
        SCENARIO_CHAPTER_UPDATE = 2101,
        SCENARIO_STAR_REWARD_UPDATE = 2102,
        SCENARIO_BATTLE_RESULT = 2103,
        SCENARIO_FIGHT_RESPONSE = 2105,
        SCENARIO_MOPUP_RESPONSE = 2107,
        SCENARIO_STAR_DRAW_RESPONSE = 2109,
        SCENARIO_RESET_RESPONSE = 2112,
        STORE_INFO_UPDATE = 2201,
        STORE_BUY_RESPONSE = 2206,
        ARENA_USER_DATA_UPDATE = 2301,
        ARENA_DEFENSE_UPDATE = 2302,
        ARENA_BATTLE_RESULT = 2303,
        ARENA_CHANGE_OPPONENT_RESPONSE = 2310,
        ARENA_FIGHT_RESPONSE = 2312,
        ARENA_SET_DEFENSE_RESPONSE = 2314,
        ARENA_DREW_REWARD_RESPONSE = 2316,
        ARENA_BATTLE_RECORD_RESPONSE = 2320,
        ARENA_BATTLE_DETAILS_RESPONSE = 2322,
        ARENA_CLEAR_BATTLE_CD_RESPONSE = 2324,
        ARENA_RESET_BATTLE_COUNT_RESPONSE = 2326,
        ARENA_REVENGE_RESPONSE = 2328,
        BABEL_DATA_UPDATE = 2401,
        BABEL_BATTLE_RESULT = 2402,
        BABEL_FIGHT_RESPONSE = 2411,
        BABEL_MOPUP_RESPONSE = 2413,
        BABEL_RESET_RESPONSE = 2415,
        BRONZE_SPARROW_TOWER_UPDATE = 2501,
        BRONZE_SPARROW_TOWER_START_RESPONSE = 2510,
        BRONZE_SPARROW_TOWER_ANSWER_RESPONSE = 2512,
        BRONZE_SPARROW_TOWER_RECIVE_REWARD_RESPONSE = 2514,
        BRONZE_SPARROW_TOWER_RESET = 2515,
        RIDING_ALONE_UPDATE = 2601,
        RIDING_ALONE_FIGHT_RESPONSE = 2603,
        RIDING_ALONE_RESET_RESPONSE = 2605,
        BATTLE_START_RT = 3001,
        BATTLE_OVER_RT = 3003,
        HERO_BATTLE_START_RT = 3005,
        HERO_BATTLE_OVER_RT = 3007,
        PALACE_WAR_SUCCESSIVE_KINGS_RESPONSE = 1601,
        PALACE_WAR_RECORDS_RESPONSE = 1603,
        KINGDOM_TITLE_LIST_RESPONSE = 1605,
        KING_CHOOSED_BY_ALLIANCE_LEADER_RESPONSE = 1607,
        KING_GIVE_TITLE_RESPONSE = 1609,
        KING_CANCEL_TITLE_RESPONSE = 1611,
        KING_GET_KINGDOM_SET_INFOS_RESPONSE = 1613,
        KING_CHANGE_SERVER_NAME_RESOPNSE = 1615,
        KING_CHANGE_RESOURCE_NODE_RATE_ID_RESPONSE = 1617,
        KING_GET_GIFT_GIVEN_LIST_RESPONSE = 1619,
        KING_GIVE_GIFT_RESPONSE = 1621,
        KING_GET_PLAYER_LIST_WHEN_GIVE_RESPONSE = 1623,
        TITLE_UPDATE = 1625,
        KING_GET_GIFT_LEFT_COUNT_RESPONSE = 1628,
        GET_REPORT_RESPONSE = 1630,
        GET_ALLIANCE_CITIES_RESPONSE = 1632,
        SCENARIO_TEAM_INFOS_UPDATE = 1633,
        CAN_MARCH_MONSTER_LEVEL_UPDATE = 1635,
        MARKET_TRANSPORT_RECORD_UPDATE = 1637,
        TRANSPORT_RESPONSE = 1639,
        STORY_ID_UPDATE_RESPONSE = 1641,
    };

}

#endif
