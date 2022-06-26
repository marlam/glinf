# Copyright (C) 2017, 2018, 2019, 2020, 2021, 2022
# Martin Lambers <marlam@marlam.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

cmake_minimum_required(VERSION 3.4)

project(glinf)

find_package(Qt6 6.2.0 COMPONENTS OpenGL)

add_executable(glinf glinf.cpp)
target_link_libraries(glinf Qt6::OpenGL)
install(TARGETS glinf RUNTIME DESTINATION bin)
