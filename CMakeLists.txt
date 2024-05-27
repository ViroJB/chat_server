cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(chat_server)

# Set the build type. Options are: None, Debug, Release, RelWithDebInfo, MinSizeRel.
set(CMAKE_BUILD_TYPE Debug) 

include_directories(C:\\Development\\Tools\\vcpkg\\installed\\x64-windows\\include)
link_directories(C:\\Development\\Tools\\vcpkg\\installed\\x64-windows\\lib)

find_package(GTest CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

# create seperate cmakelist files for src and tests dirs and see move the stuff there, see how it goes
# add_subdirectory(src)
# add_subdirectory(tests)

add_executable(chat_server 
    src/app/app.cpp
    src/config/config.cpp
    src/server/server.cpp
)
target_link_libraries(chat_server PRIVATE ws2_32 fmt::fmt)

enable_testing()

# so we have to create a seperate executable for each test file to make em show at least a bit seperate in vscode testexplorer
# do we have to link the libraries like sqlite for the tests too? i guess?
add_executable(tests
    tests/main.cpp
)

target_link_libraries(tests PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)
add_test(AllTests tests)