cmake_minimum_required(VERSION 3.16)

project(Thrasher)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)


# Include AWS SDK
#find_package(AWSSDK REQUIRED COMPONENTS sqs)
#find_package(Arrow)

add_executable(Thrasher src/thrasher.c)
target_link_libraries(Thrasher PRIVATE pthread)


add_executable(GetInterval src/GetInterval.c)


