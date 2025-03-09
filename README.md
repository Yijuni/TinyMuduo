# 简易muduo网络库
## 简介
对muduo网络库的主要功能进行了重构，使用epoll进行轮询（没有实现poll的部分）
## 使用 
cd muduo代码/mymuduo
sudo ./autobuild.sh
自动生成库并且把头文件和生成的libmymuduo.so库放在/usr/include/mymuduo和/usr/lib
