-- MySQL dump 10.13  Distrib 5.5.57, for debian-linux-gnu (x86_64)
--
-- Host: localhost    Database: sg_map
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
-- Table structure for table `s_agent`
--

DROP TABLE IF EXISTS `s_agent`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_agent` (
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `data` text NOT NULL COMMENT '其它数据(json)',
  PRIMARY KEY (`uid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='地图中玩家数据表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_battle_history`
--

DROP TABLE IF EXISTS `s_battle_history`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_battle_history` (
  `id` int(11) NOT NULL COMMENT 'ID',
  `redUid` bigint(20) NOT NULL DEFAULT '0' COMMENT '红队玩家ID',
  `redNickname` varchar(128) NOT NULL DEFAULT '' COMMENT '红队玩家昵称',
  `redAllianceId` bigint(20) NOT NULL DEFAULT '0' COMMENT '红队联盟ID',
  `redAllianceNickname` varchar(128) NOT NULL DEFAULT '' COMMENT '红队联盟昵称',
  `blueUid` bigint(20) NOT NULL DEFAULT '0' COMMENT '蓝队玩家ID',
  `blueNickname` varchar(128) NOT NULL DEFAULT '' COMMENT '蓝队玩家昵称',
  `blueAllianceId` bigint(20) NOT NULL DEFAULT '0' COMMENT '蓝队联盟ID',
  `blueAllianceNickname` varchar(128) NOT NULL DEFAULT '' COMMENT '蓝队联盟昵称',
  `isRedDelete` tinyint(1) NOT NULL COMMENT '红队已经删除',
  `isBlueDelete` tinyint(1) NOT NULL COMMENT '蓝队已经删除',
  `winTeam` int(11) NOT NULL DEFAULT '0' COMMENT '胜利的队伍',
  `battleType` int(11) NOT NULL DEFAULT '0' COMMENT '战斗类型',
  `battleTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '战斗时间',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='战斗历史记录';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_monster_siege`
--

DROP TABLE IF EXISTS `s_monster_siege`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_monster_siege` (
  `uid` bigint(20) NOT NULL COMMENT '玩家ID',
  `level` int(11) NOT NULL COMMENT '怪物等级',
  `food` int(11) NOT NULL COMMENT '粮食',
  `wood` int(11) NOT NULL COMMENT '木材',
  `iron` int(11) NOT NULL COMMENT '铁',
  `mithril` int(11) NOT NULL COMMENT '秘银',
  `timestamp` bigint(20) NOT NULL COMMENT '被攻击的时间戳',
  PRIMARY KEY (`uid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='怪物攻城';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_msg_queue`
--

DROP TABLE IF EXISTS `s_msg_queue`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_msg_queue` (
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `mid` int(11) NOT NULL COMMENT 'MID',
  `type` smallint(6) NOT NULL COMMENT '消息类型(MessageQueueType)',
  `data` mediumtext NOT NULL COMMENT '消息数据(json)',
  `create_time` bigint(20) NOT NULL DEFAULT '0' COMMENT '创建时间',
  UNIQUE KEY `index_uid_mid` (`uid`,`mid`) USING BTREE,
  KEY `index_uid` (`uid`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='消息队列表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_palace_war_record`
--

DROP TABLE IF EXISTS `s_palace_war_record`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_palace_war_record` (
  `id` int(11) NOT NULL AUTO_INCREMENT COMMENT 'ID',
  `type` int(11) NOT NULL COMMENT '记录类型',
  `data` varchar(512) NOT NULL COMMENT '其它数据(lua_table)',
  `timestamp` bigint(20) NOT NULL COMMENT '记录时间戳',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='王城争霸记录';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_troop`
--

DROP TABLE IF EXISTS `s_troop`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_troop` (
  `id` int(11) NOT NULL COMMENT 'ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `data` text NOT NULL COMMENT '其它数据(json)',
  `is_delete` tinyint(1) NOT NULL DEFAULT '0' COMMENT '是否已删除',
  `create_time` bigint(20) NOT NULL DEFAULT '0' COMMENT '创建时间',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='地图中玩家数据表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_unit`
--

DROP TABLE IF EXISTS `s_unit`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_unit` (
  `id` int(11) NOT NULL COMMENT 'ID',
  `tplid` int(11) NOT NULL COMMENT 'uit模板ID',
  `x` int(11) NOT NULL COMMENT '坐标x',
  `y` int(11) NOT NULL COMMENT '坐标y',
  `uid` bigint(20) NOT NULL DEFAULT '0' COMMENT 'UID（是城堡才需要保存UID，方便查询与合服处理）',
  `data` text NOT NULL COMMENT '其它数据(json)',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='地图中unit（怪物除外）数据表';
/*!40101 SET character_set_client = @saved_cs_client */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2018-06-04 15:55:58
