cmake_minimum_required(VERSION 3.21)
project("project-name")

include("bread/cmake/engine.cmake")
add_subdirectory("bread")

file(GLOB GAME_SRC
    App/Main.cpp
)

bread_project(
    NAME
        "project-name"

    PACKAGES
        

    SOURCES
        ${GAME_SRC}
)

