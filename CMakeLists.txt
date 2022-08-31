cmake_minimum_required(VERSION 3.21)
project(myLittleWebserv)

set(CMAKE_CXX_STANDARD 98)

include_directories(Config)
include_directories(EventHandler)
include_directories(Router)
include_directories(VirtualServer)
include_directories(CgiResponse)
include_directories(HttpRequest)
include_directories(HttpResponse)
include_directories(Log)


add_executable(myLittleWebserv
        CgiResponse/CgiResponse.hpp
        Config/Config.hpp
        Config/Config.cpp
        EventHandler/Event.hpp
        EventHandler/EventHandler.hpp
        EventHandler/EventHandler.cpp
        HttpRequest/HttpRequest.hpp
        HttpRequest/HttpRequest.cpp
        HttpRequest/Storage.hpp
        HttpRequest/Storage.cpp
        HttpResponse/HttpResponse.hpp
        HttpResponse/HttpResponse.cpp
        HttpResponse/FileManager.hpp
        HttpResponse/FileManager.cpp
        Log/Log.hpp
        Log/Log.cpp
        Router/Router.cpp
        Router/Router.hpp
        VirtualServer/VirtualServer.hpp VirtualServer/VirtualServer.cpp)
