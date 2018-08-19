-- MySQL dump 10.13  Distrib 5.5.57, for debian-linux-gnu (x86_64)
--
-- Host: localhost    Database: sg_log
-- ------------------------------------------------------
-- Server version	5.5.57-0ubuntu0.14.04.1

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `s_activity_target_reward`
--

DROP TABLE IF EXISTS `s_activity_target_reward`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_activity_target_reward` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `activity_id` bigint(20) NOT NULL COMMENT '活动ID',
  `activity_tplid` int(11) NOT NULL COMMENT '活动模板ID',
  `target` int(11) NOT NULL COMMENT '目标',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='活动目标奖励日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_alliance_explore`
--

DROP TABLE IF EXISTS `s_alliance_explore`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_alliance_explore` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `aid` bigint(20) NOT NULL COMMENT 'AID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `explore_id` int(11) NOT NULL COMMENT '探险ID',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='联盟探险日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_alliance_science_donate`
--

DROP TABLE IF EXISTS `s_alliance_science_donate`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_alliance_science_donate` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `aid` bigint(20) NOT NULL COMMENT 'AID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `science_id` int(11) NOT NULL COMMENT '科技ID',
  `group_id` int(11) NOT NULL COMMENT '科技组ID',
  `science_level` int(11) NOT NULL COMMENT '科技等级',
  `item_id` int(11) NOT NULL COMMENT '贡献物品ID',
  `count` int(11) NOT NULL COMMENT '数量',
  `exp` int(11) NOT NULL COMMENT '获得经验',
  `active` int(11) NOT NULL COMMENT '获得活跃度',
  `score` int(11) NOT NULL COMMENT '获得积分',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=446 DEFAULT CHARSET=utf8 COMMENT='联盟捐献日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_alliance_score`
--

DROP TABLE IF EXISTS `s_alliance_score`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_alliance_score` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `score` int(11) NOT NULL COMMENT '积分',
  `log_type` smallint(6) NOT NULL DEFAULT '1' COMMENT 'LogItemType 1获得 2消耗',
  `opt_type` smallint(6) NOT NULL COMMENT '获得/删除类型',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE,
  KEY `index_log_opt_type` (`log_type`,`opt_type`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=446 DEFAULT CHARSET=utf8 COMMENT='联盟积分日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_alliance_store`
--

DROP TABLE IF EXISTS `s_alliance_store`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_alliance_store` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `aid` bigint(20) NOT NULL COMMENT 'AID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `honor` int(11) NOT NULL COMMENT '消耗荣誉',
  `item_id` int(11) NOT NULL COMMENT '物品ID',
  `item_count` int(11) NOT NULL COMMENT '物品数量',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='联盟商店日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_arena_win_point`
--

DROP TABLE IF EXISTS `s_arena_win_point`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_arena_win_point` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `score` int(11) NOT NULL COMMENT '数量',
  `log_type` smallint(6) NOT NULL DEFAULT '1' COMMENT 'LogItemType 1获得 2消耗',
  `opt_type` smallint(6) NOT NULL COMMENT '获得/删除类型',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE,
  KEY `index_log_opt_type` (`log_type`,`opt_type`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='擂台胜点日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_army`
--

DROP TABLE IF EXISTS `s_army`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_army` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `armyType` int(11) NOT NULL COMMENT '兵种类型',
  `level` int(11) NOT NULL COMMENT '兵种等级',
  `count` int(11) NOT NULL COMMENT '数量',
  `log_type` smallint(6) NOT NULL DEFAULT '1' COMMENT 'LogItemType 1获得 2消耗',
  `opt_type` smallint(6) NOT NULL COMMENT '获得/删除类型',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE,
  KEY `index_log_opt_type` (`log_type`,`opt_type`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=31616 DEFAULT CHARSET=utf8 COMMENT='兵种日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_babel_score`
--

DROP TABLE IF EXISTS `s_babel_score`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_babel_score` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `score` int(11) NOT NULL COMMENT '积分',
  `log_type` smallint(6) NOT NULL DEFAULT '1' COMMENT 'LogItemType 1获得 2消耗',
  `opt_type` smallint(6) NOT NULL COMMENT '获得/删除类型',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE,
  KEY `index_log_opt_type` (`log_type`,`opt_type`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=3 DEFAULT CHARSET=utf8 COMMENT='千重楼积分日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_bronze_score`
--

DROP TABLE IF EXISTS `s_bronze_score`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_bronze_score` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `score` int(11) NOT NULL COMMENT '积分',
  `log_type` smallint(6) NOT NULL DEFAULT '1' COMMENT 'LogItemType 1获得 2消耗',
  `opt_type` smallint(6) NOT NULL COMMENT '获得/删除类型',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE,
  KEY `index_log_opt_type` (`log_type`,`opt_type`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=3 DEFAULT CHARSET=utf8 COMMENT='铜雀台积分日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_building_upgrade`
--

DROP TABLE IF EXISTS `s_building_upgrade`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_building_upgrade` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `building_type` int(11) NOT NULL COMMENT '建筑类型',
  `from_level` int(11) NOT NULL COMMENT '原来等级',
  `to_level` int(11) NOT NULL COMMENT '升到等级',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=3543 DEFAULT CHARSET=utf8 COMMENT='建筑升级日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_dice`
--

DROP TABLE IF EXISTS `s_dice`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_dice` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `is_free` tinyint(1) NOT NULL COMMENT '是否免费',
  `design_type` int(11) NOT NULL COMMENT '骰子类型 1四图案相同 2三图案相同 3两图案相同 4四图案都不同',
  `drops` varchar(256) NOT NULL COMMENT '获得掉落 lua_table',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='摇骰子日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_food`
--

DROP TABLE IF EXISTS `s_food`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_food` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `count` int(11) NOT NULL COMMENT '数量',
  `log_type` smallint(6) NOT NULL DEFAULT '1' COMMENT 'LogItemType 1获得 2消耗',
  `opt_type` smallint(6) NOT NULL COMMENT '获得/删除类型',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE,
  KEY `index_log_opt_type` (`log_type`,`opt_type`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=64805 DEFAULT CHARSET=utf8 COMMENT='粮食日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_gold`
--

DROP TABLE IF EXISTS `s_gold`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_gold` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `count` int(11) NOT NULL COMMENT '数量',
  `param1` int(11) NOT NULL DEFAULT '0' COMMENT '参数1 购买并使用时表示物品ID',
  `log_type` smallint(6) NOT NULL DEFAULT '1' COMMENT 'LogItemType 1获得 2消耗',
  `opt_type` smallint(6) NOT NULL COMMENT '获得/删除类型',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE,
  KEY `index_log_opt_type` (`log_type`,`opt_type`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=33694 DEFAULT CHARSET=utf8 COMMENT='金币日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_hero`
--

DROP TABLE IF EXISTS `s_hero`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_hero` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `hero_id` int(11) NOT NULL COMMENT '武将ID',
  `hero_level` int(11) NOT NULL COMMENT '武将等级',
  `hero_star` int(11) NOT NULL COMMENT '武将星级',
  `log_type` smallint(6) NOT NULL DEFAULT '1' COMMENT 'LogItemType 1获得 2消耗',
  `opt_type` smallint(6) NOT NULL COMMENT '获得/删除类型',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE,
  KEY `index_log_opt_type` (`log_type`,`opt_type`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=12999 DEFAULT CHARSET=utf8 COMMENT='武将日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_hero_upgrade`
--

DROP TABLE IF EXISTS `s_hero_upgrade`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_hero_upgrade` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `heroId` int(11) NOT NULL COMMENT '武将ID',
  `from_level` int(11) NOT NULL COMMENT '原来等级',
  `to_level` int(11) NOT NULL COMMENT '升到等级',
  `from_star` int(11) NOT NULL COMMENT '原来星级',
  `to_star` int(11) NOT NULL COMMENT '升到星级',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_heroid` (`heroId`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=2225 DEFAULT CHARSET=utf8 COMMENT='武将等级星级升级日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_iron`
--

DROP TABLE IF EXISTS `s_iron`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_iron` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `count` int(11) NOT NULL COMMENT '数量',
  `log_type` smallint(6) NOT NULL DEFAULT '1' COMMENT 'LogItemType 1获得 2消耗',
  `opt_type` smallint(6) NOT NULL COMMENT '获得/删除类型',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE,
  KEY `index_log_opt_type` (`log_type`,`opt_type`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=13043 DEFAULT CHARSET=utf8 COMMENT='铁矿日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_item`
--

DROP TABLE IF EXISTS `s_item`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_item` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `tplid` int(11) NOT NULL COMMENT '物品模板ID',
  `count` int(11) NOT NULL COMMENT '数量',
  `log_type` smallint(6) NOT NULL DEFAULT '1' COMMENT 'LogItemType 1获得 2消耗',
  `opt_type` smallint(6) NOT NULL COMMENT '获得/删除类型',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE,
  KEY `index_log_opt_type` (`log_type`,`opt_type`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=24027 DEFAULT CHARSET=utf8 COMMENT='物品日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_login`
--

DROP TABLE IF EXISTS `s_login`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_login` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `exp` int(11) NOT NULL COMMENT '获得经验',
  `login_level` int(11) NOT NULL COMMENT '登录时等级',
  `logout_level` int(11) NOT NULL COMMENT '退出时等级',
  `login_time` bigint(20) NOT NULL COMMENT '登录时间',
  `logout_time` bigint(20) NOT NULL COMMENT '退出时间',
  `isReconnect` tinyint(1) NOT NULL DEFAULT '0' COMMENT '是否重连',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_login_time` (`login_time`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=14301 DEFAULT CHARSET=utf8 COMMENT='登录日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_march`
--

DROP TABLE IF EXISTS `s_march`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_march` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `type` int(11) NOT NULL COMMENT '出征类型 1攻城 3采集 4打怪 5打营地 6侦查',
  `is_win` tinyint(1) NOT NULL COMMENT '是否胜利',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=93940 DEFAULT CHARSET=utf8 COMMENT='出征日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_monster_siege`
--

DROP TABLE IF EXISTS `s_monster_siege`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_monster_siege` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `is_get_back` tinyint(1) NOT NULL COMMENT '是否资源抢回',
  `level` int(11) NOT NULL COMMENT '怪物等级',
  `food` int(11) NOT NULL COMMENT '粮食',
  `wood` int(11) NOT NULL COMMENT '木材',
  `iron` int(11) NOT NULL COMMENT '铁',
  `stone` int(11) NOT NULL COMMENT '石料',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='怪物攻城日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_online`
--

DROP TABLE IF EXISTS `s_online`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_online` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `count` int(11) NOT NULL COMMENT '在线人数',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=34639 DEFAULT CHARSET=utf8 COMMENT='在线人数日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_online_reward`
--

DROP TABLE IF EXISTS `s_online_reward`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_online_reward` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `seconds` int(11) NOT NULL COMMENT '等待秒数',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='在线奖励日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_role_upgrade`
--

DROP TABLE IF EXISTS `s_role_upgrade`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_role_upgrade` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `from_level` int(11) NOT NULL COMMENT '原来等级',
  `to_level` int(11) NOT NULL COMMENT '升到等级',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=65015 DEFAULT CHARSET=utf8 COMMENT='角色升级日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_sign`
--

DROP TABLE IF EXISTS `s_sign`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_sign` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `days` int(11) NOT NULL COMMENT '签到天数',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='签到日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_silver`
--

DROP TABLE IF EXISTS `s_silver`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_silver` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `count` int(11) NOT NULL COMMENT '数量',
  `log_type` smallint(6) NOT NULL DEFAULT '1' COMMENT 'LogItemType 1获得 2消耗',
  `opt_type` smallint(6) NOT NULL COMMENT '获得/删除类型',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE,
  KEY `index_log_opt_type` (`log_type`,`opt_type`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=2538 DEFAULT CHARSET=utf8 COMMENT='银两日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_skill_point`
--

DROP TABLE IF EXISTS `s_skill_point`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_skill_point` (
  `id` bigint(20) NOT NULL COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `count` int(11) NOT NULL COMMENT '数量',
  `log_type` smallint(6) NOT NULL DEFAULT '1' COMMENT 'LogItemType 1获得 2消耗',
  `opt_type` smallint(6) NOT NULL COMMENT '获得/删除类型',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_stamina`
--

DROP TABLE IF EXISTS `s_stamina`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_stamina` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `count` int(11) NOT NULL COMMENT '数量',
  `log_type` smallint(6) NOT NULL DEFAULT '1' COMMENT 'LogItemType 1获得 2消耗',
  `opt_type` smallint(6) NOT NULL COMMENT '获得/删除类型',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE,
  KEY `index_log_opt_type` (`log_type`,`opt_type`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=66456 DEFAULT CHARSET=utf8 COMMENT='体力日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_stone`
--

DROP TABLE IF EXISTS `s_stone`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_stone` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `count` int(11) NOT NULL COMMENT '数量',
  `log_type` smallint(6) NOT NULL DEFAULT '1' COMMENT 'LogItemType 1获得 2消耗',
  `opt_type` smallint(6) NOT NULL COMMENT '获得/删除类型',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE,
  KEY `index_log_opt_type` (`log_type`,`opt_type`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=12732 DEFAULT CHARSET=utf8 COMMENT='石矿日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_store`
--

DROP TABLE IF EXISTS `s_store`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_store` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `store_type` smallint(6) NOT NULL COMMENT '商店类型StoreType',
  `coin_type` smallint(6) NOT NULL COMMENT '货币类型StoreCoinType',
  `consume` int(11) NOT NULL COMMENT '消耗',
  `item_id` int(11) NOT NULL COMMENT '物品ID',
  `item_count` int(11) NOT NULL COMMENT '物品数量',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=57 DEFAULT CHARSET=utf8 COMMENT='商店日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_technology`
--

DROP TABLE IF EXISTS `s_technology`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_technology` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `technology_id` int(11) NOT NULL COMMENT '科技ID',
  `technology_level` int(11) NOT NULL COMMENT '科技等级',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=386 DEFAULT CHARSET=utf8 COMMENT='科技日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_tower`
--

DROP TABLE IF EXISTS `s_tower`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_tower` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `layer` int(11) NOT NULL COMMENT '层数',
  `is_win` tinyint(1) NOT NULL COMMENT '是否成功',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='爬塔日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_turnplate`
--

DROP TABLE IF EXISTS `s_turnplate`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_turnplate` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `is_free` tinyint(1) NOT NULL COMMENT '是否免费',
  `type` int(11) NOT NULL COMMENT '转盘奖品类型 1倍数 2资源 3道具',
  `mul` int(11) NOT NULL COMMENT '倍数',
  `drops` varchar(256) NOT NULL COMMENT '获得掉落 lua_table',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='转盘日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_vip_upgrade`
--

DROP TABLE IF EXISTS `s_vip_upgrade`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_vip_upgrade` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `from_level` int(11) NOT NULL COMMENT '原来等级',
  `to_level` int(11) NOT NULL COMMENT '升到等级',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=49 DEFAULT CHARSET=utf8 COMMENT='VIP升级日志表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_wood`
--

DROP TABLE IF EXISTS `s_wood`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_wood` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `count` int(11) NOT NULL COMMENT '数量',
  `log_type` smallint(6) NOT NULL DEFAULT '1' COMMENT 'LogItemType 1获得 2消耗',
  `opt_type` smallint(6) NOT NULL COMMENT '获得/删除类型',
  `log_time` bigint(20) NOT NULL COMMENT '日志时间',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_log_time` (`log_time`) USING BTREE,
  KEY `index_log_opt_type` (`log_type`,`opt_type`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=54871 DEFAULT CHARSET=utf8 COMMENT='木材日志表';
/*!40101 SET character_set_client = @saved_cs_client */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2018-06-04 15:56:00
