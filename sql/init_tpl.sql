
-- 创建模板库
create database sg_tpl;
create user sg_tpl_user@'%' identified by 'LkjHgf1DsaT2reVb';
grant all privileges on sg_tpl.* to sg_tpl_user@'%';
flush privileges;
