cmake_minimum_required(VERSION 3.10.0)
project(pingger VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED true)

add_executable(pingger main.cpp)

install(TARGETS pingger DESTINATION /usr/bin)
install(CODE "execute_process(COMMAND setcap cap_net_raw+ep /usr/bin/pingger)")