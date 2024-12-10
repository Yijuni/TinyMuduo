#!/bin/bash

set -e

# 如果没有build目录创建build目录
if [ ! -d "$PWD/build" ]; then
    mkdir "$PWD/build"
fi

# 删除build里原有的文件
rm -rf "$PWD/build/*"

# 打开build并执行cmake和make
cd "$PWD/build" &&
    cmake .. &&
    make

# 回到项目根目录
cd ..

# 如果不存在mymuduo
if [ ! -d "/usr/include/mymuduo" ]; then
    mkdir "/usr/include/mymuduo"
fi

# 把头文件拷贝到/usr/include/mymuduo
for header in *.h
do 
    cp "$header" "/usr/include/mymuduo"
done

# so库拷贝到/usr/lib
cp "$PWD/lib/libmymuduo.so" "/usr/lib"

#刷新下动态库缓存
ldconfig