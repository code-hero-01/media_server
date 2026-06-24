cmake_minimum_required(VERSION 3.22.1)
project (media_server)
set(CMAKE_CXX_STANDARD 20)
add_executable(${PROJECT_NAME} main.cpp http_server.cpp request.cpp response.cpp file_handler.cpp http_connection.cpp router.cpp)
