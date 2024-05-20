cmake_minimum_required(VERSION 3.21)

project (sqlitecpp)

# libsqlitecpp depends on the following libraries:
# - sqlite3

set(EXT_LIB_DIR "${EXT_LIB_DIR}")
set(INC_DIR     "${EXT_LIB_DIR}/include")
set(LIB_DIR     "${EXT_LIB_DIR}/lib")
set(SRC_DIR     "${CMAKE_SOURCE_DIR}")

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

# install

install(FILES "${LIB_DIR}/${EXT_LIB_SQL}" DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")

install(DIRECTORY "${SRC_DIR}/include/" DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libsqlitecpp")

install(FILES "${SRC_DIR}/LICENSE" DESTINATION "${CMAKE_INSTALL_PREFIX}" RENAME "LICENSE-libsqlitecpp.txt")

if (IOS)
    install(FILES "${CMAKE_BINARY_DIR}/$<CONFIG>-${IOS_SDK}/libsqlitecpp.a" DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
    install(FILES "${CMAKE_BINARY_DIR}/$<CONFIG>-${IOS_SDK}/libsqlitecpp.a" DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
elseif (APPLE)
    install(FILES "${CMAKE_BINARY_DIR}/$<CONFIG>/libsqlitecpp.dylib" DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
    install(FILES "${CMAKE_BINARY_DIR}/$<CONFIG>/libsqlitecpp.dylib" DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
elseif (WIN32)
    install(FILES "${CMAKE_BINARY_DIR}/$<CONFIG>/sqlitecpp.dll" DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
    install(FILES "${CMAKE_BINARY_DIR}/$<CONFIG>/sqlitecpp.dll" "${CMAKE_BINARY_DIR}/$<CONFIG>/sqlitecpp.lib" DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
else()
    install(FILES "${CMAKE_BINARY_DIR}/libsqlitecpp.so" DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
    install(FILES "${CMAKE_BINARY_DIR}/libsqlitecpp.so" DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
endif()

# unittest

if (WIN32)
    add_library(unittest SHARED "${SRC_DIR}/test/unittest.cpp")

    target_compile_definitions(unittest PRIVATE ${OS})
    target_include_directories(unittest PRIVATE ${INCS})
    target_link_directories(unittest    PRIVATE ${LIB_DIR})
    target_link_libraries(unittest      PRIVATE sqlitecpp ${LIBS})
endif()
