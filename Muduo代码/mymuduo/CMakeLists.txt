cmake_minimum_required(VERSION 2.8...3.22)
project(mymuduo)

#mymuduo最终编译成动态库，设置动态库路径,放在根目录lib文件夹下
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
#设置C++编译器的编译标志，包括所有之前设置的标志（如果有的话），然后添加-g和-std=c++11
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -fPIC")
#定义参与编译的源文件, . 也就是当前文件夹所有文件,用SRC_LIST表示
aux_source_directory(. SRC_LIST)
#编译生成库，并且是动态库，参与编译的源文件为SRC_LIST
add_library(mymuduo SHARED ${SRC_LIST})
