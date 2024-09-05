cmake_minimum_required(VERSION 3.16.3)
PROJECT("dictu-pg")
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_library(dictu-pg SHARED src/dictu-pg.c)

target_include_directories(dictu-pg PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third-party/dictu/")
target_include_directories(dictu-pg PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third-party/pq/build/include")
if(WIN32)
      target_link_libraries(dictu-pg PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third-party/pq/build/lib/libpq.lib")
elseif(APPLE)
  target_link_libraries(dictu-pg PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third-party/pq/build/lib/libpq.a"
                                          "${CMAKE_CURRENT_SOURCE_DIR}/third-party/pq/build/lib/libpgport.a"
                                          "${CMAKE_CURRENT_SOURCE_DIR}/third-party/pq/build/lib/libpgcommon.a"
                                          "${CMAKE_CURRENT_SOURCE_DIR}/third-party/pq/build/lib/libpgcommon_shlib.a")
else()
    target_link_libraries(dictu-pg PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third-party/pq/build/lib/libpq.so")
endif()
