/*
Navicat MySQL Data Transfer

Source Server         : 1.5
Source Server Version : 50520
Source Host           : 192.168.1.5:3306
Source Database       : forceview

Target Server Type    : MYSQL
Target Server Version : 50520
File Encoding         : 65001

Date: 2014-02-24 11:34:46
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for audit_ftp
-- ----------------------------
DROP TABLE IF EXISTS `audit_ftp`;
CREATE TABLE `audit_ftp` (
  `ftp_id` bigint(32) NOT NULL AUTO_INCREMENT,
  `user_id` int(11) NOT NULL,
  `src_ip` varchar(25) NOT NULL,
  `src_mac` varchar(25) NOT NULL,
  `des_ip` varchar(25) NOT NULL,
  `des_mac` varchar(25) NOT NULL,
  `ftp_acct` varchar(255) NOT NULL,
  PRIMARY KEY (`ftp_id`)
) ENGINE=InnoDB AUTO_INCREMENT=5 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for audit_ftp_cmd
-- ----------------------------
DROP TABLE IF EXISTS `audit_ftp_cmd`;
CREATE TABLE `audit_ftp_cmd` (
  `ftp_id` bigint(32) NOT NULL AUTO_INCREMENT,
  `cmd_id` int(11) NOT NULL,
  `ftp_time` varchar(25) NOT NULL,
  `cmd` varchar(255) NOT NULL,
  PRIMARY KEY (`ftp_id`,`cmd_id`)
) ENGINE=InnoDB AUTO_INCREMENT=5 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for audit_search
-- ----------------------------
DROP TABLE IF EXISTS `audit_search`;
CREATE TABLE `audit_search` (
  `search_id` bigint(32) NOT NULL AUTO_INCREMENT,
  `usr_id` varchar(255) NOT NULL,
  `ip` varchar(255) NOT NULL,
  `time` varchar(255) NOT NULL,
  `content` varchar(255) DEFAULT NULL,
  `type` int(255) unsigned zerofill NOT NULL,
  PRIMARY KEY (`search_id`)
) ENGINE=InnoDB AUTO_INCREMENT=353 DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for audit_telnet
-- ----------------------------
DROP TABLE IF EXISTS `audit_telnet`;
CREATE TABLE `audit_telnet` (
  `telnet_id` bigint(32) NOT NULL,
  `user_id` int(11) NOT NULL,
  `src_ip` varchar(25) NOT NULL,
  `src_mac` varchar(25) NOT NULL,
  `des_ip` varchar(25) NOT NULL,
  `des_mac` varchar(25) NOT NULL,
  `telnet_acct` varchar(255) NOT NULL,
  PRIMARY KEY (`telnet_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for audit_telnet_cmd
-- ----------------------------
DROP TABLE IF EXISTS `audit_telnet_cmd`;
CREATE TABLE `audit_telnet_cmd` (
  `telnet_id` bigint(32) NOT NULL,
  `cmd_id` int(11) NOT NULL,
  `telnet_time` varchar(25) NOT NULL,
  `cmd` varchar(255) NOT NULL,
  PRIMARY KEY (`telnet_id`,`cmd_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
