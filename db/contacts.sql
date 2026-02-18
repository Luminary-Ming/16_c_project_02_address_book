/*
 Navicat Premium Data Transfer

 Source Server         : Ubuntu 22.04
 Source Server Type    : MySQL
 Source Server Version : 80044 (8.0.44-0ubuntu0.22.04.2)
 Source Host           : 10.11.17.172:3306
 Source Schema         : address_book

 Target Server Type    : MySQL
 Target Server Version : 80044 (8.0.44-0ubuntu0.22.04.2)
 File Encoding         : 65001

 Date: 30/01/2026 23:31:01
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for contacts
-- ----------------------------
DROP TABLE IF EXISTS `contacts`;
CREATE TABLE `contacts`  (
  `id` int NOT NULL AUTO_INCREMENT,
  `name` varchar(100) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL,
  `telephone` varchar(20) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL,
  `email` varchar(100) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NULL DEFAULT NULL,
  `initial` char(1) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL,
  `image` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NULL DEFAULT NULL,
  `del` int NOT NULL,
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`) USING BTREE,
  INDEX `idx_name`(`name` ASC) USING BTREE,
  INDEX `idx_telephone`(`telephone` ASC) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 88 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_0900_ai_ci ROW_FORMAT = Dynamic;

-- ----------------------------
-- Records of contacts
-- ----------------------------
INSERT INTO `contacts` VALUES (50, 'W 王子琛', '13292809055', '13292809055@qq.com', 'W', 'http://10.11.17.141:9000/filebucket/address_book/wzc.jpg', 0, '2026-01-30 19:06:37', '2026-01-30 19:20:40');
INSERT INTO `contacts` VALUES (51, 'D 段闯', '13458813116', '13458813116@qq.com', 'D', 'http://10.11.17.141:9000/filebucket/address_book/dc.jpg', 0, '2026-01-30 19:19:15', '2026-01-30 19:19:15');
INSERT INTO `contacts` VALUES (52, 'C 崔磊', '15862304371', '15862304371@qq.com', 'C', 'http://10.11.17.141:9000/filebucket/address_book/cl.jpg', 0, '2026-01-30 19:25:59', '2026-01-30 23:11:45');
INSERT INTO `contacts` VALUES (53, 'Z 郑博方', '18788813143', '18788813143@qq.com', 'Z', 'http://10.11.17.141:9000/filebucket/address_book/2321_1769779745.jpg', 0, '2026-01-30 19:26:59', '2026-01-30 21:29:05');
INSERT INTO `contacts` VALUES (54, 'C 崔天祥', '13571938758', '13571938758@qq.com', 'C', 'http://10.11.17.141:9000/filebucket/address_book/ctx.jpg', 0, '2026-01-30 19:28:01', '2026-01-30 19:28:01');
INSERT INTO `contacts` VALUES (55, 'C 曹小龙', '15092706109', '15092706109@qq.com', 'C', 'http://10.11.17.141:9000/filebucket/address_book/cxl.jpg', 0, '2026-01-30 19:29:46', '2026-01-30 19:29:46');
INSERT INTO `contacts` VALUES (56, 'C 陈艺敏', '18487819822', '18487819822@qq.com', 'C', 'http://10.11.17.141:9000/filebucket/address_book/cym_1769779723.jpg', 0, '2026-01-30 20:52:43', '2026-01-30 21:28:43');
INSERT INTO `contacts` VALUES (57, 'Z 张伟', '17271006541', '17271006541@qq.com', 'Z', 'http://10.11.17.141:9000/filebucket/address_book/zw_1769779663.jpg', 0, '2026-01-30 20:59:34', '2026-01-30 22:44:29');
INSERT INTO `contacts` VALUES (58, 'L 李明硕', '18225363122', '18225363122@qq.com', 'L', 'http://10.11.17.141:9000/filebucket/address_book/lms_1769784211.jpg', 0, '2026-01-30 22:43:48', '2026-01-30 22:44:01');
INSERT INTO `contacts` VALUES (59, 'L 刘佳', '17820265657', '17820265657@qq.com', 'L', 'http://10.11.17.141:9000/filebucket/address_book/lj_1769784387.jpg', 0, '2026-01-30 22:46:48', '2026-01-30 22:46:48');
INSERT INTO `contacts` VALUES (60, 'L 刘茜', '15878979641', '15878979641@qq.com', 'L', 'http://10.11.17.141:9000/filebucket/address_book/lq_1769784422.jpg', 0, '2026-01-30 22:47:19', '2026-01-30 22:47:19');
INSERT INTO `contacts` VALUES (61, 'L 李雨灿', '13918173570', '13918173570@qq.com', 'L', 'http://10.11.17.141:9000/filebucket/address_book/lyc_1769784451.jpg', 0, '2026-01-30 22:47:45', '2026-01-30 22:47:45');
INSERT INTO `contacts` VALUES (62, 'L 李泽康', '13990247628', '13990247628@qq.com', 'L', 'http://10.11.17.141:9000/filebucket/address_book/lzk_1769784483.jpg', 0, '2026-01-30 22:48:33', '2026-01-30 22:48:33');
INSERT INTO `contacts` VALUES (63, 'S 石建荣', '15265708565', '15265708565@qq.com', 'S', 'http://10.11.17.141:9000/filebucket/address_book/sjr_1769784521.jpg', 0, '2026-01-30 22:49:00', '2026-01-30 22:49:00');
INSERT INTO `contacts` VALUES (64, 'W 王俊贺', '16558962347', '16558962347@qq.com', 'W', 'http://10.11.17.141:9000/filebucket/address_book/wjh_1769784547.jpg', 0, '2026-01-30 22:49:20', '2026-01-30 22:49:20');
INSERT INTO `contacts` VALUES (65, 'W 王逍遥', '13542476179', '13542476179@qq.com', 'W', 'http://10.11.17.141:9000/filebucket/address_book/wxy_1769784588.jpg', 0, '2026-01-30 22:50:01', '2026-01-30 22:50:01');
INSERT INTO `contacts` VALUES (66, 'Y 袁宝林', '18746756902', '18746756902@qq.com', 'Y', 'http://10.11.17.141:9000/filebucket/address_book/ybl_1769784727.jpg', 0, '2026-01-30 22:52:18', '2026-01-30 22:52:18');
INSERT INTO `contacts` VALUES (67, 'Y 杨胜', '18419141343', '18419141343@qq.com', 'Y', 'http://10.11.17.141:9000/filebucket/address_book/ys_1769784751.jpg', 0, '2026-01-30 22:52:47', '2026-01-30 22:52:47');
INSERT INTO `contacts` VALUES (68, 'Y 袁新平', '15242795221', '15242795221@qq.com', 'Y', 'http://10.11.17.141:9000/filebucket/address_book/yxp_1769784778.jpg', 0, '2026-01-30 22:53:14', '2026-01-30 22:53:14');
INSERT INTO `contacts` VALUES (69, 'Y 杨阳', '13815021710', '13815021710@qq.com', 'Y', 'http://10.11.17.141:9000/filebucket/address_book/yy_1769784802.jpg', 0, '2026-01-30 22:53:52', '2026-01-30 22:53:52');
INSERT INTO `contacts` VALUES (70, 'Y 杨洋', '15888107710', '15888107710@qq.com', 'Y', 'http://10.11.17.141:9000/filebucket/address_book/yy1_1769784847.jpg', 0, '2026-01-30 22:54:23', '2026-01-30 22:54:23');
INSERT INTO `contacts` VALUES (71, 'Y 杨玉龙', '15892759302', '15892759302@qq.com', 'Y', 'http://10.11.17.141:9000/filebucket/address_book/yyl_1769784868.jpg', 0, '2026-01-30 22:54:40', '2026-01-30 22:54:40');
INSERT INTO `contacts` VALUES (72, 'Z 张冰涵', '19895626876', '19895626876@qq.com', 'Z', 'http://10.11.17.141:9000/filebucket/address_book/zbh_1769784888.jpg', 0, '2026-01-30 22:55:13', '2026-01-30 22:55:13');
INSERT INTO `contacts` VALUES (73, 'Z 赵海超', '18772036486', '18772036486@qq.com', 'Z', 'http://10.11.17.141:9000/filebucket/address_book/zhc_1769784953.jpg', 0, '2026-01-30 22:56:06', '2026-01-30 22:56:06');
INSERT INTO `contacts` VALUES (74, 'Z 周浪', '15806368497', '15806368497@qq.com', 'Z', 'http://10.11.17.141:9000/filebucket/address_book/zl_1769784989.jpg', 0, '2026-01-30 22:56:52', '2026-01-30 22:56:52');
INSERT INTO `contacts` VALUES (75, 'Z 张苏单', '15886924594', '15886924594@qq.com', 'Z', 'http://10.11.17.141:9000/filebucket/address_book/zsd_1769785048.jpg', 0, '2026-01-30 23:01:31', '2026-01-30 23:01:31');
INSERT INTO `contacts` VALUES (76, 'B 白云飞', '18095426745', '18095426745@qq.com', 'B', 'http://10.11.17.141:9000/filebucket/address_book/byf_1769785509.jpg', 0, '2026-01-30 23:05:23', '2026-01-30 23:05:23');
INSERT INTO `contacts` VALUES (77, 'L 李桐', '15676906545', '15676906545@qq.com', 'L', 'http://10.11.17.141:9000/filebucket/address_book/lt_1769785532.jpg', 0, '2026-01-30 23:05:44', '2026-01-30 23:05:44');
INSERT INTO `contacts` VALUES (78, 'Z 张宇鹏', '18541839949', '18541839949@qq.com', 'Z', 'http://10.11.17.141:9000/filebucket/address_book/zyp_1769785550.jpg', 0, '2026-01-30 23:06:06', '2026-01-30 23:06:06');
INSERT INTO `contacts` VALUES (79, 'Y 杨可欣', '17784289612', '17784289612@qq.com', 'Y', 'http://10.11.17.141:9000/filebucket/address_book/ykx_1769785580.jpg', 0, '2026-01-30 23:06:31', '2026-01-30 23:06:31');
INSERT INTO `contacts` VALUES (80, 'Q 祁书通', '13677003803', '13677003803@qq.com', 'Q', 'http://10.11.17.141:9000/filebucket/address_book/qst_1769785600.jpg', 0, '2026-01-30 23:06:52', '2026-01-30 23:06:52');
INSERT INTO `contacts` VALUES (82, 'Z 赵进科', '15676906545', '15676906545@qq.com', 'Z', 'http://10.11.17.141:9000/filebucket/address_book/zjk_1769785736.jpg', 0, '2026-01-30 23:09:13', '2026-01-30 23:09:13');
INSERT INTO `contacts` VALUES (83, 'L 刘丛兵', '17784289612', '17784289612@qq.com', 'L', 'http://10.11.17.141:9000/filebucket/address_book/lcb_1769785863.jpg', 0, '2026-01-30 23:11:24', '2026-01-30 23:11:24');
INSERT INTO `contacts` VALUES (85, 'Y 杨劲邦', '15587712637', '15587712637@qq.com', 'Y', 'http://10.11.17.141:9000/filebucket/address_book/yjb_1769786042.jpg', 0, '2026-01-30 23:14:22', '2026-01-30 23:14:22');
INSERT INTO `contacts` VALUES (86, 'M 马福', '15330399913', '15330399913@qq.com', 'M', 'http://10.11.17.141:9000/filebucket/address_book/mf_1769786072.jpg', 0, '2026-01-30 23:14:45', '2026-01-30 23:16:21');
INSERT INTO `contacts` VALUES (87, 'L 竺宝宇', '15393624556', '15393624556@qq.com', 'L', 'http://10.11.17.141:9000/filebucket/address_book/lby_1769786150.jpg', 0, '2026-01-30 23:16:12', '2026-01-30 23:16:12');
INSERT INTO `contacts` VALUES (88, 'G 郭仁杰', '17321441764', '17321441764@qq.com', 'G', 'http://10.11.17.141:9000/filebucket/address_book/grj_1769935794.jpg', 0, '2026-02-01 16:50:11', '2026-02-01 16:50:11');

SET FOREIGN_KEY_CHECKS = 1;
