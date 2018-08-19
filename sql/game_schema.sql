-- MySQL dump 10.13  Distrib 5.5.57, for debian-linux-gnu (x86_64)
--
-- Host: localhost    Database: sg_game
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
-- Table structure for table `s_activity`
--

DROP TABLE IF EXISTS `s_activity`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_activity` (
  `id` bigint(20) NOT NULL COMMENT '活动ID',
  `remark` varchar(128) NOT NULL DEFAULT '' COMMENT '备注名称',
  `priority` int(11) NOT NULL DEFAULT '0' COMMENT '优先级，数字越大，表示优先级越高',
  `type` int(11) NOT NULL COMMENT '活动类型',
  `tplId` int(11) NOT NULL COMMENT '活动展示的模板ID',
  `checkOnStart` tinyint(1) NOT NULL DEFAULT '0' COMMENT '是否开始时才检查城堡等级（如果否，则开始后也会检查）',
  `minLordLevel` int(11) NOT NULL DEFAULT '1' COMMENT '需要最小城主等级',
  `maxLordLevel` int(11) NOT NULL DEFAULT '30' COMMENT '需要最大城主等级',
  `preOpenSeconds` bigint(20) NOT NULL COMMENT '开启之前秒数',
  `timeType` smallint(6) NOT NULL COMMENT '1:不限时 2:开服 3:限时',
  `openTime` bigint(20) NOT NULL COMMENT '开启时间',
  `closeTime` bigint(20) NOT NULL COMMENT 'timeType=2表示持续秒数 timeType=3表示关闭时间',
  `afterCloseSeconds` bigint(20) NOT NULL COMMENT '关闭之后秒数',
  `active` tinyint(1) NOT NULL DEFAULT '0' COMMENT '是否已激活',
  `display` tinyint(1) NOT NULL DEFAULT '1' COMMENT '是否展示出来',
  `autoReward` tinyint(1) NOT NULL DEFAULT '1' COMMENT '是否自动发奖励（只对个人普通/个人积分活动）',
  `bestItemList` varchar(512) NOT NULL DEFAULT '' COMMENT '需要特别显示的奖励物品ID列表（如果有奖励），以,号分隔',
  `config` text NOT NULL COMMENT '综合配置（json []括号里的数据可配置多个）',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='活动表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_activity_ranking`
--

DROP TABLE IF EXISTS `s_activity_ranking`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_activity_ranking` (
  `id` bigint(20) NOT NULL COMMENT '活动ID',
  `type` int(11) NOT NULL COMMENT '活动类型',
  `tplId` int(11) NOT NULL COMMENT '活动模板ID',
  `openTime` bigint(20) NOT NULL COMMENT '开启时间',
  `closeTime` bigint(20) NOT NULL COMMENT '关闭时间',
  `rewarded` tinyint(1) NOT NULL DEFAULT '0' COMMENT '是否已派奖励',
  `rankingList` mediumtext NOT NULL COMMENT '排名列表　格式： {[排名]={uid=1,headId=1,nickname="kiki",progress=1}',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='活动排名表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_alliance`
--

DROP TABLE IF EXISTS `s_alliance`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_alliance` (
  `id` bigint(20) NOT NULL COMMENT 'ID',
  `name` varchar(256) NOT NULL COMMENT '联盟名称',
  `nickname` varchar(256) NOT NULL COMMENT '联盟简称',
  `level` int(11) NOT NULL DEFAULT '1' COMMENT '联盟等级',
  `exp` int(11) NOT NULL DEFAULT '0' COMMENT '联盟经验',
  `bannerId` int(11) NOT NULL COMMENT '联盟旗帜ID',
  `power` int(11) NOT NULL COMMENT '联盟战力',
  `leaderId` bigint(20) NOT NULL COMMENT '盟主ID',
  `leaderName` varchar(256) NOT NULL COMMENT '盟主昵称',
  `leaderHeadId` bigint(20) NOT NULL COMMENT '盟主头像',
  `alliesCount` int(11) NOT NULL DEFAULT '0' COMMENT '成员数',
  `towerCount` int(11) NOT NULL DEFAULT '0' COMMENT '防御塔数',
  `castleCount` int(11) NOT NULL DEFAULT '0' COMMENT '城池数',
  `toatlActive` int(11) NOT NULL DEFAULT '0' COMMENT '联盟总活跃',
  `announcement` varchar(1024) NOT NULL DEFAULT '' COMMENT '公告',
  `openRecruitment` tinyint(1) NOT NULL COMMENT '公开招募',
  `applyList` text NOT NULL COMMENT '申请者列表',
  `inviteList` text NOT NULL COMMENT '被邀请者列表',
  `scienceList` text NOT NULL COMMENT '联盟科技列表',
  `allianceBuffList` text NOT NULL COMMENT '联盟增益buff列表',
  `allianceLoseBuffList` text NOT NULL COMMENT '失去Buff的名城ID列表',
  `createTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '创建时间',
  `disbandTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '解散时间',
  `changeBannerCdTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '换联盟旗帜CD时间',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='联盟表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_alliance_gift`
--

DROP TABLE IF EXISTS `s_alliance_gift`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_alliance_gift` (
  `gid` bigint(20) NOT NULL COMMENT 'GID',
  `aid` bigint(20) NOT NULL COMMENT '联盟ID',
  `uid` bigint(20) NOT NULL COMMENT '购买礼包的玩家ID',
  `nickname` varchar(128) NOT NULL COMMENT '购买礼包的玩家昵称',
  `giftId` int(11) NOT NULL COMMENT '联盟礼物ID',
  `giftCount` int(11) NOT NULL COMMENT '联盟礼物数量',
  `payTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '购买礼包时间',
  PRIMARY KEY (`gid`),
  KEY `index_aid` (`aid`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='联盟礼物表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_alliance_help`
--

DROP TABLE IF EXISTS `s_alliance_help`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_alliance_help` (
  `id` int(11) NOT NULL COMMENT '帮助ID',
  `aid` bigint(20) NOT NULL COMMENT '联盟ID',
  `uid` bigint(20) NOT NULL COMMENT '申请帮助的玩家UID',
  `buildingId` int(11) NOT NULL COMMENT '申请帮助的建筑ID',
  `times` int(11) NOT NULL COMMENT '被帮助次数',
  `accept` int(11) NOT NULL COMMENT '玩家已处理帮助次数',
  `max` int(11) NOT NULL COMMENT '总共可被帮助次数',
  `reduceTime` int(11) NOT NULL COMMENT '减少时间',
  `cdId` int(11) NOT NULL COMMENT '申请帮助的建筑CD ID',
  `cdTime` int(11) NOT NULL COMMENT '申请帮助的建筑CD结束时间',
  `helpUserList` varchar(1024) NOT NULL COMMENT '提供帮助的玩家',
  `helpDesc` varchar(512) NOT NULL COMMENT '帮助详细信息',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='联盟帮助表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_alliance_hero_lease`
--

DROP TABLE IF EXISTS `s_alliance_hero_lease`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_alliance_hero_lease` (
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `aid` bigint(20) NOT NULL COMMENT '联盟ID',
  `incomeCount` int(11) NOT NULL COMMENT '已收益次数',
  `leaseList` text NOT NULL COMMENT '出租列表详细信息',
  PRIMARY KEY (`uid`),
  KEY `index_aid` (`aid`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='联盟武将租借表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_alliance_invite`
--

DROP TABLE IF EXISTS `s_alliance_invite`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_alliance_invite` (
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `inviteList` text NOT NULL COMMENT '邀请列表',
  `createTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '邀请时间',
  PRIMARY KEY (`uid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='联盟邀请表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_alliance_member`
--

DROP TABLE IF EXISTS `s_alliance_member`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_alliance_member` (
  `aid` bigint(20) NOT NULL COMMENT '联盟ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `userLevel` int(11) NOT NULL COMMENT '玩家等级',
  `vipLevel` int(11) NOT NULL COMMENT 'vip等级',
  `rankLevel` int(11) NOT NULL COMMENT '阶级',
  `activeWeek` int(11) NOT NULL COMMENT '个人周活跃值',
  `contribution` int(11) NOT NULL COMMENT '联盟科技贡献值',
  `sendMailCount` int(11) NOT NULL COMMENT '已发送全体邮件数量',
  `marketIsOpen` tinyint(1) NOT NULL DEFAULT '0' COMMENT '市场是否开启，（0：未开启，1：开启）',
  `lastOnlineTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '上次在线时间',
  `joinTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '入盟时间',
  PRIMARY KEY (`uid`),
  KEY `index_aid` (`aid`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='联盟成员表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_arena`
--

DROP TABLE IF EXISTS `s_arena`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_arena` (
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `aid` bigint(20) NOT NULL COMMENT 'AID',
  `nickname` varchar(256) NOT NULL COMMENT '玩家昵称',
  `level` int(11) NOT NULL COMMENT '玩家等级',
  `headId` int(11) NOT NULL COMMENT '玩家头像',
  `allianceName` varchar(256) NOT NULL COMMENT '联盟全称',
  `allianceNickname` varchar(256) NOT NULL COMMENT '联盟昵称',
  `bannerId` int(11) NOT NULL COMMENT '联盟旗帜',
  `rank` int(11) NOT NULL COMMENT '玩家排名',
  `lastRank` int(11) NOT NULL COMMENT '上一次排行榜最终排名',
  `winCount` int(11) NOT NULL COMMENT '赢的数量',
  `loseCount` int(11) NOT NULL COMMENT '输的数量',
  `winStreak` int(11) NOT NULL COMMENT '连胜数量',
  `createTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '创建时间',
  `isRobot` tinyint(1) NOT NULL DEFAULT '0' COMMENT '是否是机器人',
  `heroPower` int(11) NOT NULL COMMENT '武将防守总战力',
  `heroList` text NOT NULL COMMENT '擂台武将防守列表',
  `attrList` text NOT NULL COMMENT '玩家全局属性(武将属性)',
  PRIMARY KEY (`uid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='擂台数据表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_arena_record`
--

DROP TABLE IF EXISTS `s_arena_record`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_arena_record` (
  `id` bigint(20) NOT NULL COMMENT '记录ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `nickname` varchar(256) NOT NULL COMMENT '玩家昵称',
  `level` int(11) NOT NULL COMMENT '玩家等级',
  `headId` int(11) NOT NULL COMMENT '玩家头像',
  `allianceName` varchar(256) NOT NULL COMMENT '联盟全称',
  `allianceNickname` varchar(256) NOT NULL COMMENT '联盟昵称',
  `bannerId` int(11) NOT NULL COMMENT '联盟旗帜',
  `toUid` bigint(20) NOT NULL COMMENT '对方UID',
  `toNickname` varchar(256) NOT NULL COMMENT '对方昵称',
  `toLevel` int(11) NOT NULL COMMENT '对方等级',
  `toHeadId` int(11) NOT NULL COMMENT '对方头像',
  `toAllianceName` varchar(256) NOT NULL COMMENT '对方联盟全称',
  `toAllianceNickname` varchar(256) NOT NULL COMMENT '对方联盟昵称',
  `toBannerId` int(11) NOT NULL COMMENT '对方联盟旗帜',
  `isWin` tinyint(1) NOT NULL DEFAULT '0' COMMENT '是否赢了',
  `isAttacker` tinyint(1) NOT NULL DEFAULT '0' COMMENT '是否是攻击者',
  `changRank` int(11) NOT NULL COMMENT '(输)下降或(赢)上升x名 0表示没有变化',
  `createTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '创建时间',
  `battleData` text NOT NULL COMMENT '战斗数据',
  `canRevenge` tinyint(1) NOT NULL DEFAULT '0' COMMENT '是否可以复仇',
  `toPower` int(11) NOT NULL DEFAULT '0' COMMENT '目标玩家的武将总战力',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_toUid` (`toUid`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='擂台战斗记录表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_bag`
--

DROP TABLE IF EXISTS `s_bag`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_bag` (
  `bid` bigint(20) NOT NULL COMMENT 'BID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `itemId` int(11) NOT NULL COMMENT '物品ID',
  `count` int(11) NOT NULL DEFAULT '0' COMMENT '数量',
  `data` varchar(1024) NOT NULL DEFAULT '{}' COMMENT '装备|宝物|技能书数据',
  `ext1` varchar(1024) NOT NULL DEFAULT '{}' COMMENT '装备|宝物|技能书额外数据1',
  `ext2` varchar(1024) NOT NULL DEFAULT '{}' COMMENT '装备|宝物|技能书额外数据2',
  PRIMARY KEY (`bid`),
  KEY `index_uid` (`uid`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='背包表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_charge`
--

DROP TABLE IF EXISTS `s_charge`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_charge` (
  `orderId` varchar(128) NOT NULL COMMENT '订单号',
  `uid` bigint(20) NOT NULL COMMENT '用户ID',
  `level` int(11) NOT NULL COMMENT '玩家等级',
  `chargeId` varchar(64) NOT NULL COMMENT '充值ID（对应s3_hub.s_charge.id）',
  `chargeType` int(11) NOT NULL COMMENT '充值类型（对应s3_hub.s_charge.type）',
  `gold` int(11) NOT NULL DEFAULT '0' COMMENT '金币（对应s3_hub.s_charge.moneyToGold）',
  `draw` tinyint(1) NOT NULL DEFAULT '0' COMMENT '是否已提取',
  `drawTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '提取时间',
  `payType` varchar(128) NOT NULL DEFAULT '' COMMENT '充值类型',
  `payTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '充值时间',
  PRIMARY KEY (`orderId`,`uid`),
  KEY `index_orderId` (`orderId`) USING BTREE,
  KEY `index_uid` (`uid`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='充值记录表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_dict`
--

DROP TABLE IF EXISTS `s_dict`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_dict` (
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `k` varchar(64) NOT NULL,
  `v` text NOT NULL,
  PRIMARY KEY (`uid`,`k`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='角色私有数据字典';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_hero`
--

DROP TABLE IF EXISTS `s_hero`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_hero` (
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `heroId` int(11) NOT NULL COMMENT '武将ID',
  `exp` int(11) NOT NULL COMMENT '武将ID',
  `level` int(11) NOT NULL COMMENT '武将等级',
  `star` int(11) NOT NULL COMMENT '武将星级',
  `soulLevel` int(11) NOT NULL COMMENT '命魂等级',
  `physical` int(11) NOT NULL COMMENT '武将体力',
  `physicalRecoveryTimeStamp` bigint(20) NOT NULL COMMENT '武将体力恢复时间戳',
  `slotNum` int(11) NOT NULL COMMENT '特殊技开放的槽位数量',
  `isLock` tinyint(1) NOT NULL DEFAULT '0' COMMENT '是否锁定',
  `skill` varchar(2048) NOT NULL DEFAULT '{}' COMMENT '技能',
  `soul` varchar(2048) NOT NULL DEFAULT '{}' COMMENT '命魂',
  UNIQUE KEY `index_uid_id` (`uid`,`heroId`) USING BTREE,
  KEY `index_uid` (`uid`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='武将表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_incr`
--

DROP TABLE IF EXISTS `s_incr`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_incr` (
  `k` varchar(64) NOT NULL COMMENT '键值',
  `v` int(11) NOT NULL DEFAULT '0' COMMENT '当前递增值',
  PRIMARY KEY (`k`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='ID自增表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_mail`
--

DROP TABLE IF EXISTS `s_mail`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_mail` (
  `id` bigint(20) NOT NULL COMMENT 'ID',
  `parentId` bigint(20) NOT NULL DEFAULT '0' COMMENT '父ID',
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `otherUid` bigint(20) NOT NULL DEFAULT '0' COMMENT '对方UID（私人邮件）',
  `type` int(11) NOT NULL COMMENT '邮件类型',
  `subType` int(11) NOT NULL COMMENT '邮件子类型',
  `title` varchar(256) NOT NULL COMMENT '标题',
  `content` varchar(4096) NOT NULL COMMENT '内容',
  `isLang` tinyint(1) NOT NULL DEFAULT '1' COMMENT '是否多语言',
  `isRead` tinyint(1) NOT NULL DEFAULT '0' COMMENT '是否已读',
  `isDraw` tinyint(1) NOT NULL DEFAULT '0' COMMENT '是否已领取',
  `deleteType` int(11) NOT NULL COMMENT '删除类型 0存在 1用户删除 2超出上限删除 3过期删除',
  `attachment` text NOT NULL COMMENT '附件',
  `createTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '创建时间',
  `deleteTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '删除时间',
  `params` text NOT NULL COMMENT '参数内容',
  `reportId` int(11) NOT NULL COMMENT '战报Id',
  PRIMARY KEY (`id`),
  KEY `index_uid` (`uid`) USING BTREE,
  KEY `index_otherUid` (`otherUid`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='邮件';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_mail_batch`
--

DROP TABLE IF EXISTS `s_mail_batch`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_mail_batch` (
  `id` int(11) NOT NULL COMMENT '跑马灯ID',
  `openTime` bigint(20) NOT NULL COMMENT '有效开启时间',
  `closeTime` bigint(20) NOT NULL COMMENT '有效结束时间',
  `active` tinyint(1) NOT NULL DEFAULT '0' COMMENT '是否已激活',
  `dropList` varchar(1024) NOT NULL COMMENT '掉落物品列表　格式： {[物品ID]=数量,[物品ID]=数量}',
  `title_CN` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 中文简体',
  `title_TW` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 中文繁体',
  `title_EN` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 英语',
  `title_FR` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 法语',
  `title_DE` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 德语',
  `title_RU` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 俄语',
  `title_KR` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 韩语',
  `title_TH` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 泰语',
  `title_JP` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 日语',
  `title_PT` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 葡萄牙语',
  `title_ES` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 西班牙语',
  `title_TR` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 土耳其语',
  `title_ID` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 印尼语',
  `title_IT` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 意大利语',
  `title_PL` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 波兰语',
  `title_NL` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 荷兰语',
  `title_AR` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 阿拉伯语',
  `title_RO` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 罗马尼亚语',
  `title_FA` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 波斯语',
  `content_CN` text NOT NULL COMMENT '内容 中文简体',
  `content_TW` text NOT NULL COMMENT '内容 中文繁体',
  `content_EN` text NOT NULL COMMENT '内容 英语',
  `content_FR` text NOT NULL COMMENT '内容 法语',
  `content_DE` text NOT NULL COMMENT '内容 德语',
  `content_RU` text NOT NULL COMMENT '内容 俄语',
  `content_KR` text NOT NULL COMMENT '内容 韩语',
  `content_TH` text NOT NULL COMMENT '内容 泰语',
  `content_JP` text NOT NULL COMMENT '内容 日语',
  `content_PT` text NOT NULL COMMENT '内容 葡萄牙语',
  `content_ES` text NOT NULL COMMENT '内容 西班牙语',
  `content_TR` text NOT NULL COMMENT '内容 土耳其语',
  `content_ID` text NOT NULL COMMENT '内容 印尼语',
  `content_IT` text NOT NULL COMMENT '内容 意大利语',
  `content_PL` text NOT NULL COMMENT '内容 波兰语',
  `content_NL` text NOT NULL COMMENT '内容 荷兰语',
  `content_AR` text NOT NULL COMMENT '内容 阿拉伯语',
  `content_RO` text NOT NULL COMMENT '内容 罗马尼亚语',
  `content_FA` text NOT NULL COMMENT '内容 波斯语',
  `mailTime` bigint(20) NOT NULL COMMENT '邮件时间（邮件显示的时间）',
  `matchLang` varchar(512) NOT NULL DEFAULT '' COMMENT '匹配语言,留空表示任意, 例如 1|2|3',
  `matchVersion` varchar(512) NOT NULL DEFAULT '' COMMENT '匹配版本号,留空表示任意, 例如 >1.6.8.0 或 <1.6.8.0 或 =1.6.8.0',
  `matchChannel` varchar(512) NOT NULL DEFAULT '' COMMENT '匹配渠道号,留空表示任意, 例如 xy_360|xy_uc|xy_apple',
  `matchLevel` varchar(512) NOT NULL DEFAULT '' COMMENT '匹配玩家等级,留空表示任意，例如 >8 或 <8 或 =8',
  `matchLoginTime` varchar(128) NOT NULL DEFAULT '' COMMENT '匹配玩家登录时间（限近7天）,留空表示任意，例 开始时间|结束时间',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='邮件批量表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_marquee`
--

DROP TABLE IF EXISTS `s_marquee`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_marquee` (
  `id` int(11) NOT NULL COMMENT '跑马灯ID',
  `type` int(11) NOT NULL DEFAULT '1' COMMENT '跑马灯类型 1跑马灯 2系统信息(在国王聊天显示)',
  `openTime` bigint(20) NOT NULL COMMENT '开启时间',
  `closeTime` bigint(20) NOT NULL COMMENT '结束时间',
  `active` tinyint(1) NOT NULL DEFAULT '0' COMMENT '是否已激活',
  `doLoop` int(11) NOT NULL DEFAULT '1' COMMENT '循环次数',
  `doInterval` int(11) NOT NULL DEFAULT '300' COMMENT '间隔时间(秒)',
  `msg_CN` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 中文简体',
  `msg_TW` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 中文繁体',
  `msg_EN` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 英语',
  `msg_FR` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 法语',
  `msg_DE` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 德语',
  `msg_RU` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 俄语',
  `msg_KR` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 韩语',
  `msg_TH` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 泰语',
  `msg_JP` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 日语',
  `msg_PT` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 葡萄牙语',
  `msg_ES` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 西班牙语',
  `msg_TR` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 土耳其语',
  `msg_ID` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 印尼语',
  `msg_IT` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 意大利语',
  `msg_PL` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 波兰语',
  `msg_NL` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 荷兰语',
  `msg_AR` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 阿拉伯语',
  `msg_RO` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 罗马尼亚语',
  `msg_FA` varchar(256) NOT NULL DEFAULT '' COMMENT '内容 波斯语',
  `matchLang` varchar(512) NOT NULL DEFAULT '' COMMENT '匹配语言,留空表示任意, 例如(1|2|3)',
  `matchVersion` varchar(512) NOT NULL DEFAULT '' COMMENT '匹配版本号,留空表示任意, 例如 >1.6.8.0 或 <1.6.8.0 或 =1.6.8.0',
  `matchChannel` varchar(512) NOT NULL DEFAULT '' COMMENT '匹配渠道号,留空表示任意, 例如 xy_360|xy_uc|xy_apple',
  `matchLevel` varchar(512) NOT NULL DEFAULT '' COMMENT '匹配玩家等级,留空表示任意，例如 >8 或 <8 或 =8',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='跑马灯表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_notice`
--

DROP TABLE IF EXISTS `s_notice`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_notice` (
  `id` int(11) NOT NULL COMMENT '公告ID',
  `priority` int(11) NOT NULL DEFAULT '0' COMMENT '优先级，数字越大，表示优先级越高',
  `expireTime` bigint(20) NOT NULL COMMENT '过期时间',
  `title_CN` varchar(256) NOT NULL DEFAULT '' COMMENT '标题 中文简体',
  `title_TW` varchar(256) NOT NULL DEFAULT '' COMMENT '标题 中文繁体',
  `title_EN` varchar(256) NOT NULL DEFAULT '' COMMENT '标题 英语',
  `title_FR` varchar(256) NOT NULL DEFAULT '' COMMENT '标题 法语',
  `title_DE` varchar(256) NOT NULL DEFAULT '' COMMENT '标题 德语',
  `title_RU` varchar(256) NOT NULL DEFAULT '' COMMENT '标题 俄语',
  `title_KR` varchar(256) NOT NULL DEFAULT '' COMMENT '标题 韩语',
  `title_TH` varchar(256) NOT NULL DEFAULT '' COMMENT '标题 泰语',
  `title_JP` varchar(256) NOT NULL DEFAULT '' COMMENT '标题 日语',
  `title_PT` varchar(256) NOT NULL DEFAULT '' COMMENT '标题 葡萄牙语',
  `title_ES` varchar(256) NOT NULL DEFAULT '' COMMENT '标题 西班牙语',
  `title_TR` varchar(256) NOT NULL DEFAULT '' COMMENT '标题 土耳其语',
  `title_ID` varchar(256) NOT NULL DEFAULT '' COMMENT '标题 印尼语',
  `title_IT` varchar(256) NOT NULL DEFAULT '' COMMENT '标题 意大利语',
  `title_PL` varchar(256) NOT NULL DEFAULT '' COMMENT '标题 波兰语',
  `title_NL` varchar(256) NOT NULL DEFAULT '' COMMENT '标题 荷兰语',
  `title_AR` varchar(256) NOT NULL DEFAULT '' COMMENT '标题 阿拉伯语',
  `title_RO` varchar(256) NOT NULL DEFAULT '' COMMENT '标题 罗马尼亚语',
  `title_FA` varchar(256) NOT NULL DEFAULT '' COMMENT '标题 波斯语',
  `content_CN` text NOT NULL COMMENT '内容 中文简体',
  `content_TW` text NOT NULL COMMENT '内容 中文繁体',
  `content_EN` text NOT NULL COMMENT '内容 英语',
  `content_FR` text NOT NULL COMMENT '内容 法语',
  `content_DE` text NOT NULL COMMENT '内容 德语',
  `content_RU` text NOT NULL COMMENT '内容 俄语',
  `content_KR` text NOT NULL COMMENT '内容 韩语',
  `content_TH` text NOT NULL COMMENT '内容 泰语',
  `content_JP` text NOT NULL COMMENT '内容 日语',
  `content_PT` text NOT NULL COMMENT '内容 葡萄牙语',
  `content_ES` text NOT NULL COMMENT '内容 西班牙语',
  `content_TR` text NOT NULL COMMENT '内容 土耳其语',
  `content_ID` text NOT NULL COMMENT '内容 印尼语',
  `content_IT` text NOT NULL COMMENT '内容 意大利语',
  `content_PL` text NOT NULL COMMENT '内容 波兰语',
  `content_NL` text NOT NULL COMMENT '内容 荷兰语',
  `content_AR` text NOT NULL COMMENT '内容 阿拉伯语',
  `content_RO` text NOT NULL COMMENT '内容 罗马尼亚语',
  `content_FA` text NOT NULL COMMENT '内容 波斯语',
  `publishTime` bigint(20) NOT NULL COMMENT '发布时间',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='公告表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_quest`
--

DROP TABLE IF EXISTS `s_quest`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_quest` (
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `questId` int(11) NOT NULL COMMENT '任务ID',
  `total` int(11) NOT NULL DEFAULT '0' COMMENT '总计',
  `progress` int(11) NOT NULL DEFAULT '0' COMMENT '当前进度',
  `finishTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '完成时间',
  `drawTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '取奖励时间',
  `createTime` bigint(20) NOT NULL COMMENT '创建时间',
  UNIQUE KEY `index_uid_questid` (`uid`,`questId`) USING BTREE,
  KEY `index_uid` (`uid`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='任务表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_report`
--

DROP TABLE IF EXISTS `s_report`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_report` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT COMMENT '战报ID',
  `uid` bigint(20) DEFAULT NULL,
  `createTime` bigint(20) NOT NULL COMMENT '创建时间',
  `data` text CHARACTER SET utf8 NOT NULL COMMENT '战斗数据',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_seven_target`
--

DROP TABLE IF EXISTS `s_seven_target`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_seven_target` (
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `id` int(11) NOT NULL COMMENT '任务ID',
  `day` int(11) NOT NULL COMMENT '所属天数',
  `total` int(11) NOT NULL COMMENT '总计',
  `progress` int(11) NOT NULL DEFAULT '0' COMMENT '当前进度',
  `finishTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '完成时间',
  `drawTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '取奖励时间',
  `createTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '创建时间',
  UNIQUE KEY `index_uid_id` (`uid`,`id`) USING BTREE,
  KEY `index_uid` (`uid`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='开服活动之7日目标表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_stat`
--

DROP TABLE IF EXISTS `s_stat`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_stat` (
  `k` varchar(64) NOT NULL COMMENT '键值',
  `v` text NOT NULL COMMENT '内容(json 格式)',
  PRIMARY KEY (`k`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='数据统计表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_transport_record`
--

DROP TABLE IF EXISTS `s_transport_record`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_transport_record` (
  `id` bigint(20) NOT NULL,
  `uid` bigint(20) NOT NULL DEFAULT '0' COMMENT '玩家id',
  `headId` int(11) NOT NULL DEFAULT '0' COMMENT '头像Id',
  `nickName` varchar(128) CHARACTER SET utf8 NOT NULL COMMENT '玩家名字',
  `toUid` bigint(20) NOT NULL DEFAULT '0' COMMENT '玩家Id',
  `toHeadId` int(11) NOT NULL DEFAULT '0' COMMENT '头像Id',
  `toNickName` varchar(128) CHARACTER SET utf8 NOT NULL COMMENT '玩家名字',
  `transportType` int(11) NOT NULL DEFAULT '0' COMMENT '0:传送   1:接收',
  `carry` varchar(128) CHARACTER SET utf8 NOT NULL COMMENT '运输的物品',
  `arriveType` int(11) NOT NULL DEFAULT '0' COMMENT '0:失败  1:到达 2：运输中',
  `arriveTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '到达时间',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `s_user`
--

DROP TABLE IF EXISTS `s_user`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `s_user` (
  `uid` bigint(20) NOT NULL COMMENT 'UID',
  `idfa` varchar(128) NOT NULL DEFAULT '' COMMENT '广告标示符',
  `channel` varchar(128) NOT NULL DEFAULT '' COMMENT '来源渠道号',
  `username` varchar(128) NOT NULL COMMENT '用户名',
  `nickname` varchar(128) NOT NULL COMMENT '昵称',
  `headId` bigint(20) NOT NULL DEFAULT '0' COMMENT '头像ID',
  `camp` int(11) NOT NULL DEFAULT '1' COMMENT '阵营',
  `level` int(11) NOT NULL DEFAULT '1' COMMENT '等级',
  `exp` int(11) NOT NULL DEFAULT '0' COMMENT '经验值',
  `silver` int(11) NOT NULL DEFAULT '0' COMMENT '银两',
  `gold` int(11) NOT NULL DEFAULT '0' COMMENT '金币',
  `food` int(11) NOT NULL DEFAULT '0' COMMENT '粮食',
  `wood` int(11) NOT NULL DEFAULT '0' COMMENT '木材',
  `iron` int(11) NOT NULL DEFAULT '0' COMMENT '铁矿',
  `stone` int(11) NOT NULL DEFAULT '0' COMMENT '石料',
  `stamina` int(11) NOT NULL DEFAULT '0' COMMENT '体力',
  `skillPoint` int(11) NOT NULL COMMENT '技能点',
  `staminaBuyCount` int(11) NOT NULL DEFAULT '0' COMMENT '体力已购买次数',
  `lastStaminaRecoverTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '上次体力恢复时间戳',
  `goldCharged` int(11) NOT NULL DEFAULT '0' COMMENT '累计充值金币',
  `langType` int(11) NOT NULL DEFAULT '1' COMMENT '语言类型',
  `x` int(11) NOT NULL DEFAULT '0' COMMENT '缓存坐标X',
  `y` int(11) NOT NULL DEFAULT '0' COMMENT '缓存坐标Y',
  `attackWins` int(11) NOT NULL DEFAULT '0' COMMENT '进攻胜利次数',
  `attackLosses` int(11) NOT NULL DEFAULT '0' COMMENT '进攻失败次数',
  `defenseWins` int(11) NOT NULL DEFAULT '0' COMMENT '防御胜利次数',
  `defenseLosses` int(11) NOT NULL DEFAULT '0' COMMENT '防御失败次数',
  `scoutCount` int(11) NOT NULL DEFAULT '0' COMMENT '侦查次数',
  `kills` int(11) NOT NULL DEFAULT '0' COMMENT '击杀部队数量',
  `losses` int(11) NOT NULL DEFAULT '0' COMMENT '部队损失数量',
  `heals` int(11) NOT NULL DEFAULT '0' COMMENT '部队治疗数量',
  `lordPower` int(11) NOT NULL DEFAULT '0' COMMENT '领主战斗力',
  `troopPower` int(11) NOT NULL DEFAULT '0' COMMENT '军队战斗力',
  `buildingPower` int(11) NOT NULL DEFAULT '0' COMMENT '建筑战斗力',
  `sciencePower` int(11) NOT NULL DEFAULT '0' COMMENT '科技战斗力',
  `trapPower` int(11) NOT NULL DEFAULT '0' COMMENT '陷阱战斗力',
  `heroPower` int(11) NOT NULL DEFAULT '0' COMMENT '武将战斗力',
  `totalPower` int(11) NOT NULL DEFAULT '0' COMMENT '总战斗力',
  `captives` int(11) NOT NULL DEFAULT '0' COMMENT '俘虏数量',
  `castleLevel` int(11) NOT NULL DEFAULT '0' COMMENT '城堡等级',
  `allianceId` bigint(20) NOT NULL DEFAULT '0' COMMENT '联盟ID',
  `allianceName` varchar(256) NOT NULL DEFAULT '' COMMENT '联盟名称',
  `allianceNickname` varchar(256) NOT NULL DEFAULT '' COMMENT '联盟简称',
  `allianceBannerId` int(11) NOT NULL DEFAULT '0' COMMENT '联盟旗帜ID',
  `lastLoginTimestamp` bigint(20) NOT NULL DEFAULT '0' COMMENT '上一次登录时间戳',
  `registerTimestamp` bigint(20) NOT NULL DEFAULT '0' COMMENT '注册时间戳',
  `banChatTimestamp` bigint(20) NOT NULL DEFAULT '0' COMMENT '禁言时间戳，0表示正常',
  `banChatReason` varchar(256) NOT NULL DEFAULT '' COMMENT '禁言原因',
  `lockedTimestamp` bigint(20) NOT NULL DEFAULT '0' COMMENT '锁定时间戳，0表示正常',
  `lockedReason` varchar(256) NOT NULL DEFAULT '' COMMENT '锁定原因',
  `isJoinRank` tinyint(1) NOT NULL DEFAULT '1' COMMENT '是否参与排行榜',
  `isCrossTeleport` tinyint(1) NOT NULL DEFAULT '0' COMMENT '是否已跨服迁城',
  `babelMaxLayer` int(11) NOT NULL DEFAULT '0' COMMENT '千层楼通关最大层数',
  `babelTimestamp` bigint(20) NOT NULL DEFAULT '0' COMMENT '千层楼通关时间',
  `bronzeMaxScore` int(11) NOT NULL DEFAULT '0' COMMENT '铜雀台历史累计积分',
  `bronzeTodayScore` int(11) NOT NULL DEFAULT '0' COMMENT '铜雀台今天累计积分',
  `bronzeTodayTimestamp` bigint(20) NOT NULL DEFAULT '0' COMMENT '铜雀台今天积分保存时间',
  `isSetName` tinyint(1) NOT NULL DEFAULT '0' COMMENT '是否设置名字',
  `storyId` int(11) NOT NULL DEFAULT '0' COMMENT '对白id',
  `staminaPropUseCount` int(11) NOT NULL DEFAULT '0' COMMENT '体力道具使用次数',
  `energy` int(11) NOT NULL DEFAULT '0' COMMENT '行动力',
  `energyBuyCount` int(11) NOT NULL DEFAULT '0' COMMENT '行动力购买次数',
  `lastEnergyRecoverTime` bigint(20) NOT NULL DEFAULT '0' COMMENT '最后一次行动力恢复时间',
  `vipLevel` int(11) NOT NULL DEFAULT '0' COMMENT 'vip等级',
  PRIMARY KEY (`uid`),
  UNIQUE KEY `index_username` (`username`) USING BTREE,
  UNIQUE KEY `index_nickname` (`nickname`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='角色表';
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
