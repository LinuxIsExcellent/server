
-- 区服信息&GMS平台库
create database sg_hub;
create user sg_hub_user@'%' identified by 'LkjHgf1DsaT2reVb';
grant all privileges on sg_hub.* to sg_hub_user@'%';
flush privileges;