-- MySQL dump 10.13  Distrib 5.5.41, for debian-linux-gnu (x86_64)
--
-- Host: localhost    Database: sg_hub
-- ------------------------------------------------------
-- Server version	5.5.41-0ubuntu0.14.04.1

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
-- Table structure for table `s_activation`
--

DROP TABLE IF EXISTS `s_activation`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_activation` (
  `id` int(11) NOT NULL COMMENT '激活礼包 ID',
  `name` varchar(256) NOT NULL COMMENT '名称',
  `groupNo` varchar(64) NOT NULL COMMENT '分组号',
  `amount` int(11) NOT NULL COMMENT '激活码数量',
  `dropList` varchar(2048) NOT NULL DEFAULT '' COMMENT '掉落物品列表 格式：{[物品模板ID]=数量,[物品模板ID]=数量}',
  `expireTime` bigint(20) NOT NULL COMMENT '过期时间',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='激活礼包表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_activation_code`
--

DROP TABLE IF EXISTS `s_activation_code`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_activation_code` (
  `id` int(11) NOT NULL COMMENT '激活礼包 ID',
  `groupNo` varchar(64) NOT NULL COMMENT '分组号',
  `code` varchar(64) NOT NULL COMMENT '激活码',
  `used` tinyint(1) NOT NULL DEFAULT '0' COMMENT '是否已使用',
  `serverId` int(11) NOT NULL DEFAULT '0' COMMENT '使用此激活码的玩家所在 游戏区ID',
  `uid` bigint(20) NOT NULL DEFAULT '0' COMMENT '使用此激活码的玩家UID',
  PRIMARY KEY (`code`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='激活码表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_area`
--

DROP TABLE IF EXISTS `s_area`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_area` (
  `id` int(11) NOT NULL COMMENT '游戏区 ID',
  `merge_to_id` int(11) NOT NULL DEFAULT '0' COMMENT '合并到游戏区 ID（0表示还没合服）',
  `name` varchar(128) NOT NULL DEFAULT 'Kingdom_Map_PALACE' COMMENT '游戏区名称，默认为 Kingdom_Map_PALACE',
  `ost` bigint(20) NOT NULL DEFAULT '0' COMMENT '开服的时间戳',
  `pwt` bigint(20) NOT NULL DEFAULT '0' COMMENT '第一次打王城战的时间戳',
  `king` varchar(128) NOT NULL COMMENT '国王名称',
  `lang` int(11) NOT NULL DEFAULT '0' COMMENT '国王语言',
  `flag` int(11) NOT NULL DEFAULT '0' COMMENT '状态 0维护 1旧服 2新服',
  `res` varchar(128) NOT NULL DEFAULT '' COMMENT '资源站点',
  `cdn` varchar(128) NOT NULL DEFAULT '' COMMENT '资源cdn站点',
  `cond_test` tinyint(1) NOT NULL DEFAULT '0' COMMENT '过虑条件：需要测试模式',
  `cond_channel` varchar(1024) NOT NULL DEFAULT '' COMMENT '过虑条件：渠道，多个可用|号分隔',
  `cond_version` varchar(512) NOT NULL DEFAULT '' COMMENT '过虑条件：版本，多个可用|号分隔',
  `domain` varchar(128) NOT NULL DEFAULT '' COMMENT '域名',
  `wan_ip` varchar(128) NOT NULL DEFAULT '127.0.0.1' COMMENT '外网IP',
  `lan_ip` varchar(128) NOT NULL DEFAULT '127.0.0.1' COMMENT '内网IP',
  `ms_port` int(11) NOT NULL DEFAULT '9020' COMMENT '地图服务器 端口',
  `fs_port` int(11) NOT NULL DEFAULT '9100' COMMENT '前端服务器 端口（第1个）',
  `http_port` int(11) NOT NULL DEFAULT '9030' COMMENT 'http服务 端口',
  `http_key` varchar(128) NOT NULL DEFAULT 'BlackKnight' COMMENT 'http服务 密钥（建议定期变换）',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='区服列表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_area_last`
--

DROP TABLE IF EXISTS `s_area_last`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_area_last` (
  `openId` varchar(128) NOT NULL COMMENT 'tsdk 账号ID',
  `serverId` int(11) NOT NULL COMMENT '游戏区ID',
  PRIMARY KEY (`openId`),
  KEY `index_serverId` (`serverId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='最近登录区服表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_badword`
--

DROP TABLE IF EXISTS `s_badword`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_badword` (
  `word` varchar(32) NOT NULL COMMENT '脏词',
  PRIMARY KEY (`word`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='脏词库';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_bulletin`
--

DROP TABLE IF EXISTS `s_bulletin`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_bulletin` (
  `id` int(11) NOT NULL DEFAULT '0' COMMENT '公告ID',
  `priority` int(11) DEFAULT '0' COMMENT '优先级，数字越大，表示优先级越高',
  `expireTime` bigint(20) DEFAULT NULL COMMENT '过期时间',
  `title_CN` varchar(256) DEFAULT '' COMMENT '标题 中文简体',
  `title_TW` varchar(256) DEFAULT '' COMMENT '标题 中文繁体',
  `title_EN` varchar(256) DEFAULT '' COMMENT '标题 英语',
  `title_FR` varchar(256) DEFAULT '' COMMENT '标题 法语',
  `title_DE` varchar(256) DEFAULT '' COMMENT '标题 德语',
  `title_RU` varchar(256) DEFAULT '' COMMENT '标题 俄语',
  `title_KR` varchar(256) DEFAULT '' COMMENT '标题 韩语',
  `title_TH` varchar(256) DEFAULT '' COMMENT '标题 泰语',
  `title_JP` varchar(256) DEFAULT '' COMMENT '标题 日语',
  `title_PT` varchar(256) DEFAULT '' COMMENT '标题 葡萄牙语',
  `title_ES` varchar(256) DEFAULT '' COMMENT '标题 西班牙语',
  `title_TR` varchar(256) DEFAULT '' COMMENT '标题 土耳其语',
  `title_ID` varchar(256) DEFAULT '' COMMENT '标题 印尼语',
  `title_IT` varchar(256) DEFAULT '' COMMENT '标题 意大利语',
  `title_PL` varchar(256) DEFAULT '' COMMENT '标题 波兰语',
  `title_NL` varchar(256) DEFAULT '' COMMENT '标题 荷兰语',
  `title_AR` varchar(256) DEFAULT '' COMMENT '标题 阿拉伯语',
  `title_RO` varchar(256) DEFAULT '' COMMENT '标题 罗马尼亚语',
  `title_FA` varchar(256) DEFAULT '' COMMENT '标题 波斯语',
  `content_CN` text COMMENT '内容 中文简体',
  `content_TW` text COMMENT '内容 中文繁体',
  `content_EN` text COMMENT '内容 英语',
  `content_FR` text COMMENT '内容 法语',
  `content_DE` text COMMENT '内容 德语',
  `content_RU` text COMMENT '内容 俄语',
  `content_KR` text COMMENT '内容 韩语',
  `content_TH` text COMMENT '内容 泰语',
  `content_JP` text COMMENT '内容 日语',
  `content_PT` text COMMENT '内容 葡萄牙语',
  `content_ES` text COMMENT '内容 西班牙语',
  `content_TR` text COMMENT '内容 土耳其语',
  `content_ID` text COMMENT '内容 印尼语',
  `content_IT` text COMMENT '内容 意大利语',
  `content_PL` text COMMENT '内容 波兰语',
  `content_NL` text COMMENT '内容 荷兰语',
  `content_AR` text COMMENT '内容 阿拉伯语',
  `content_RO` text COMMENT '内容 罗马尼亚语',
  `content_FA` text COMMENT '内容 波斯语',
  `sort` int(10) DEFAULT NULL,
  `publishTime` bigint(20) DEFAULT NULL COMMENT '发布时间',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='公告表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_charge`
--

DROP TABLE IF EXISTS `s_charge`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_charge` (
  `id` varchar(128) NOT NULL COMMENT '充值ID',
  `type` int(11) NOT NULL DEFAULT '1' COMMENT '充值类型，1金币充值 2礼包充值 3月卡充值',
  `name` varchar(64) NOT NULL DEFAULT '' COMMENT '名称',
  `remark` varchar(64) NOT NULL DEFAULT '' COMMENT '备注',
  `icon` varchar(128) NOT NULL DEFAULT '' COMMENT '图标',
  `iconAtlas` varchar(128) NOT NULL DEFAULT '' COMMENT '图标合集',
  `inset` varchar(64) NOT NULL DEFAULT '' COMMENT '插画',
  `uiType` int(11) NOT NULL DEFAULT '0' COMMENT '界面类型',
  `priority` int(11) NOT NULL DEFAULT '0' COMMENT '优先级，数字越大，表示优先级越高',
  `moneyToGold` int(11) NOT NULL COMMENT '充值金额换算金币（数值确定后不能修改）',
  `gold` int(11) NOT NULL COMMENT '充值获得金币（月卡每天获得的金币也是它）',
  `discount` float NOT NULL COMMENT '折扣（用于展示）',
  `giftId` int(11) NOT NULL COMMENT '联盟礼物ID（0表示没有）',
  `giftCount` int(11) NOT NULL COMMENT '联盟礼物数量',
  `isHot` tinyint(1) NOT NULL DEFAULT '0' COMMENT '是否热门',
  `openTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '开始时间',
  `closeTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '结束时间',
  `minCastleLevel` int(11) NOT NULL DEFAULT '0' COMMENT '城堡等级下限,0表示不限制',
  `maxCastleLevel` int(11) NOT NULL DEFAULT '0' COMMENT '城堡等级上限,0表示不限制',
  `minChargeGold` int(11) NOT NULL DEFAULT '0' COMMENT '累充金币下限,0表示不限制',
  `maxChargeGold` int(11) NOT NULL DEFAULT '0' COMMENT '累充金币上限,0表示不限制',
  `limitDay` int(11) NOT NULL COMMENT '限购天数（0表示永久限购）',
  `limitCount` int(11) NOT NULL COMMENT '限购次数（0表示无限购）',
  `extraGold` int(11) NOT NULL COMMENT 'type=1金币充值，限购额外获得金币',
  `extraItems` varchar(2048) NOT NULL DEFAULT '{}' COMMENT 'type=2礼包充值或3月卡充值，限购额外获得的物品或月卡每天获得物品，格式：{[物品ID]=数量,[物品ID]=数量}',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='充值配置表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_mail`
--

DROP TABLE IF EXISTS `s_mail`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_mail` (
  `id` int(11) NOT NULL AUTO_INCREMENT COMMENT '个人邮件 ID',
  `serverId` int(11) NOT NULL COMMENT '游戏区 ID',
  `uid` bigint(20) NOT NULL COMMENT '玩家UID（UID永远不变）',
  `nickname` varchar(256) NOT NULL DEFAULT '' COMMENT '玩家昵称（可能会变）',
  `title` varchar(512) NOT NULL DEFAULT '' COMMENT '邮箱标题',
  `content` varchar(4096) NOT NULL DEFAULT '' COMMENT '邮箱内容',
  `dropList` varchar(1024) NOT NULL DEFAULT '' COMMENT '掉落物品列表　格式： {[物品ID]=数量,[物品ID]=数量}',
  `addUser` varchar(128) NOT NULL DEFAULT '' COMMENT '添加邮件者',
  `addTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '添加时间',
  `verifyUser` varchar(128) NOT NULL DEFAULT '' COMMENT '审核邮件者',
  `verifyTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '审核时间',
  `state` int(11) NOT NULL COMMENT '状态 1等待审核 2已通过  3已拒绝',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=12 DEFAULT CHARSET=utf8 COMMENT='个人邮件表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_palacewar`
--

DROP TABLE IF EXISTS `s_palacewar`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_palacewar` (
  `id` int(11) NOT NULL COMMENT '地图服务器ID',
  `kingUid` bigint(20) NOT NULL COMMENT '国王uid',
  `titles` varchar(4096) NOT NULL COMMENT '称号',
  `gifts` varchar(4096) NOT NULL COMMENT '礼包',
  `canChangeServerName` tinyint(1) NOT NULL DEFAULT '0' COMMENT '能否改服务器名字',
  `resourceNodeRateId` int(11) NOT NULL DEFAULT '0' COMMENT '刷新资源率ID',
  `lastChangeResIdTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '上次修改资源率ID的时间戳',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='王城争霸';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_recharge`
--

DROP TABLE IF EXISTS `s_recharge`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_recharge` (
  `id` int(11) NOT NULL AUTO_INCREMENT COMMENT '充值补单 ID',
  `serverId` int(11) NOT NULL COMMENT '游戏区 ID',
  `uid` bigint(20) NOT NULL COMMENT '玩家UID（UID永远不变）',
  `nickname` varchar(256) NOT NULL DEFAULT '' COMMENT '玩家昵称（可能会变）',
  `orderId` varchar(128) NOT NULL DEFAULT '' COMMENT '订单号',
  `chargeId` varchar(128) NOT NULL DEFAULT '' COMMENT '充值ID',
  `payType` varchar(128) NOT NULL DEFAULT 'recharge' COMMENT '充值类型，默认请填recharge，可以根据这个类型来统计补单的记录',
  `addUser` varchar(128) NOT NULL DEFAULT '' COMMENT '添加补单者',
  `addTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '添加时间',
  `verifyUser` varchar(128) NOT NULL DEFAULT '' COMMENT '审核补单者',
  `verifyTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '审核时间',
  `state` int(11) NOT NULL COMMENT '状态 1等待审核 2已通过  3已拒绝',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`),
  KEY `index_serverId` (`serverId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='充值补单表';
/*!40101 SET character_set_client = @saved_cs_client */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2018-04-16 21:28:10
