-- 创建游戏数据库
create database sg_game;
create user sg_game_user@'%' identified by 'LkjHgf1DsaT2reVb';
grant all privileges on sg_game.* to sg_game_user@'%';
flush privileges;

-- 创建地图数据库
create database sg_map;
create user sg_map_user@'%' identified by 'LkjHgf1DsaT2reVb';
grant all privileges on sg_map.* to sg_map_user@'%';
flush privileges;

-- 创建日志数据库
create database sg_log;
create user sg_log_user@'%' identified by 'LkjHgf1DsaT2reVb';
grant all privileges on sg_log.* to sg_log_user@'%';
flush privileges;

