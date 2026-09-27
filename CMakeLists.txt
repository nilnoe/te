cmake_minimum_required(VERSION 3.16)
project(te LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 引入 FetchContent 模块
include(FetchContent)

# 声明 SDL3 依赖：锁定一个稳定的 release 版本
# FetchContent_Declare(
#   SDL3
#   GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
#   GIT_TAG release-3.4.16   # 建议锁定具体版本号，不要用 origin/main
#   GIT_SHALLOW TRUE         # 只拉取指定 tag 的历史，减少下载量
#   GIT_PROGRESS TRUE        # 显示下载进度，网络慢时能看到状态
# )
# 
# # 让 CMake 拉取源码并纳入构建
# FetchContent_MakeAvailable(SDL3)

find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(SDL3_ttf REQUIRED)

# 创建你的可执行文件
add_executable(te main.c la.c)

# 链接 SDL3::SDL3 目标：自动传递头文件路径和库
target_link_libraries(te PRIVATE SDL3::SDL3 SDL3_image::SDL3_image SDL3_ttf::SDL3_ttf)
