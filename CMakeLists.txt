cmake_minimum_required(VERSION 3.21)

project (sqlitecpp)

# libsqlitecpp depends on the following libraries:
# - sqlite3

set(EXT_LIB_DIR "${LSC_EXT_LIB_DIR}")
set(INC_DIR     "${LSC_EXT_LIB_DIR}/include")
set(LIB_DIR     "${LSC_EXT_LIB_DIR}/lib")
set(SRC_DIR     "${CMAKE_SOURCE_DIR}")
set(DIST_DIR    "${CMAKE_BINARY_DIR}/dist")

set(CMAKE_CXX_STANDARD          "20")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (IOS)
    set(EXT_LIB_SQL libsqlite3.a)
elseif (APPLE)
    set(EXT_LIB_SQL libsqlite3.dylib)
elseif (WIN32)
    set(EXT_LIB_SQL sqlite3.dll)
else()
    set(EXT_LIB_SQL libsqlite3.so)
endif()

find_file(LIB_SQL ${EXT_LIB_SQL} PATHS ${LIB_DIR} NO_CACHE REQUIRED CMAKE_FIND_ROOT_PATH_BOTH)

set(INCS "include" ${INC_DIR})

if (WIN32)
    set(LIBS sqlite3.lib)
else()
    set(LIBS "-lsqlite3")
endif()

if (ANDROID)
    set(OS _android)
elseif (IOS)
    set(OS _ios)
elseif (APPLE)
    set(OS _macosx)
elseif (WIN32)
    set(OS _windows)
else()
    set(OS _linux)
endif()

if (WIN32)
    set(RC "${SRC_DIR}/windows/libsqlitecpp.rc")

    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT sqlitecpp)
endif()

file(GLOB INC_H   "${SRC_DIR}/include/*.h")
file(GLOB SRC_H   "${SRC_DIR}/src/*.h")
file(GLOB SRC_CPP "${SRC_DIR}/src/*.cpp")

source_group("Resources" FILES ${RC})

if (IOS)
    add_library(sqlitecpp STATIC ${SRC_CPP} ${SRC_H} ${INC_H})
else()
    add_library(sqlitecpp SHARED ${SRC_CPP} ${SRC_H} ${INC_H} ${RC})
endif()

target_compile_definitions(sqlitecpp PRIVATE ${OS} MAKEDLL)
target_include_directories(sqlitecpp PRIVATE ${INCS})
target_link_directories(sqlitecpp    PRIVATE ${LIB_DIR})
target_link_libraries(sqlitecpp      PRIVATE ${LIBS})

# libsqlitecpp

if (WIN32)
    add_custom_command(TARGET sqlitecpp POST_BUILD COMMAND if exist \"${DIST_DIR}\" rmdir /s /q \"${DIST_DIR}\")

    add_custom_command(TARGET sqlitecpp POST_BUILD COMMAND if not exist \"${DIST_DIR}/bin\"     mkdir \"${DIST_DIR}/bin\")
    add_custom_command(TARGET sqlitecpp POST_BUILD COMMAND if not exist \"${DIST_DIR}/include\" mkdir \"${DIST_DIR}/include\")
    add_custom_command(TARGET sqlitecpp POST_BUILD COMMAND if not exist \"${DIST_DIR}/lib\"     mkdir \"${DIST_DIR}/lib\")
else()
    add_custom_command(TARGET sqlitecpp POST_BUILD COMMAND rm -rf \"${DIST_DIR}\")

    add_custom_command(TARGET sqlitecpp POST_BUILD COMMAND mkdir -p \"${DIST_DIR}/bin\")
    add_custom_command(TARGET sqlitecpp POST_BUILD COMMAND mkdir -p \"${DIST_DIR}/include\")
    add_custom_command(TARGET sqlitecpp POST_BUILD COMMAND mkdir -p \"${DIST_DIR}/lib\")
endif()

if (IOS)
    add_custom_command(TARGET sqlitecpp POST_BUILD COMMAND cp -f ${CFG_DIR}libsqlitecpp.a ${DIST_DIR}/lib/)
elseif (APPLE)
    add_custom_command(TARGET sqlitecpp POST_BUILD COMMAND cp -f ${CFG_DIR}libsqlitecpp.dylib ${DIST_DIR}/lib/)
elseif (WIN32)
    add_custom_command(TARGET sqlitecpp POST_BUILD COMMAND cp -f ${CFG_DIR}sqlitecpp.lib ${DIST_DIR}/lib/)
    add_custom_command(TARGET sqlitecpp POST_BUILD COMMAND cp -f ${CFG_DIR}sqlitecpp.dll ${DIST_DIR}/lib/)
else()
    add_custom_command(TARGET sqlitecpp POST_BUILD COMMAND cp -f ${CFG_DIR}libsqlitecpp.so ${DIST_DIR}/lib/)
endif()

add_custom_command(TARGET sqlitecpp POST_BUILD COMMAND cp -f ${LIB_SQL} ${DIST_DIR}/bin/)

add_custom_command(TARGET sqlitecpp POST_BUILD COMMAND cp -f ${SRC_DIR}/include/*.h ${DIST_DIR}/include/)

add_custom_command(TARGET sqlitecpp POST_BUILD COMMAND cp -f ${SRC_DIR}/LICENSE ${DIST_DIR}/LICENSE-libsqlitecpp.txt)

# install

install(DIRECTORY ${DIST_DIR}/lib/     DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(DIRECTORY ${DIST_DIR}/include/ DESTINATION ${CMAKE_INSTALL_PREFIX}/include/libsqlitecpp)
install(DIRECTORY ${DIST_DIR}/         DESTINATION ${CMAKE_INSTALL_PREFIX} FILES_MATCHING PATTERN "*.txt")

# unittest

if (WIN32)
    add_library(unittest SHARED "${SRC_DIR}/test/unittest.cpp")

    target_compile_definitions(unittest PRIVATE ${OS})
    target_include_directories(unittest PRIVATE ${INCS})
    target_link_directories(unittest    PRIVATE ${LIB_DIR})
    target_link_libraries(unittest      PRIVATE sqlitecpp ${LIBS})
endif()
