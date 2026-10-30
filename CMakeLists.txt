cmake_minimum_required(VERSION 3.10)

# 通过 -DSDK_PATH=... 或 LUCKFOX_SDK_PATH 指定 SDK，避免绑定开发者本机路径。
set(SDK_PATH "$ENV{LUCKFOX_SDK_PATH}" CACHE PATH "Path to the Luckfox Pico SDK")
if(NOT SDK_PATH)
    message(FATAL_ERROR "SDK_PATH is not set. Export LUCKFOX_SDK_PATH or pass -DSDK_PATH=/absolute/path/to/luckfox-pico")
endif()

# 设置编译器
set(CMAKE_C_COMPILER "${SDK_PATH}/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-gcc")
set(CMAKE_CXX_COMPILER "${SDK_PATH}/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++")


# 创建项目
project("af_terminal")

# 链接目录
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/arm-rockchip830-linux-uclibcgnueabihf)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ 3rdparty.out)

# 设置RPATH
set(CMAKE_INSTALL_RPATH "$ORIGIN/lib")
add_definitions(-DRV1106_1103 -DWITH_NO_C_LOCALE)

#Opencv 4 查找OpenCV
set(OpenCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/cmake/opencv4")
find_package(OpenCV REQUIRED)

#Thread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# 源文件
set(CODE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/code")
file(GLOB SRC_FILES 
                    "${CODE_DIR}/param/*.c"
                    "${CODE_DIR}/osd/*.c"
                    "${CODE_DIR}/display/*.cc"
                    "${CODE_DIR}/display/*.c"
                    "${CODE_DIR}/video/*.cc"
                    "${CODE_DIR}/video/*.c"
                    "${CODE_DIR}/ptz/*.cpp"
                    "${CODE_DIR}/ptz/*.c"
                    "${CODE_DIR}/observer/*.cpp"
                    "${CODE_DIR}/tcp/*.cpp"
                    "${CODE_DIR}/control/*.cpp"
                    "${CODE_DIR}/onvif/soap/*.c"
                    "${CODE_DIR}/onvif/server/*.c"
                    "${CODE_DIR}/*.c"
                    "*.cc" 
)  

add_executable(${PROJECT_NAME} ${SRC_FILES})

add_compile_options(-g -Wall
                    -DISP_HW_V30 -DRKPLATFORM=ON -DARCH64=OFF
                    -DROCKIVA -DUAPI2
                    -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
                    -WITH_NO_C_LOCALE
                    )


# Set linker flags to handle runtime paths for shared libraries
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-as-needed,-rpath-link,${CMAKE_CURRENT_SOURCE_DIR}/lib:/usr/lib")

link_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib/arm-rockchip830-linux-uclibcgnueabihf")

target_link_libraries(${PROJECT_NAME}  
                    ${OpenCV_LIBS}
                    ${LIBRKNNRT}
                    Threads::Threads
                    rockiva
                    sample_comm
                    rockit
                    rockchip_mpp
                    rkaiq
                    pthread
                    rtsp
                    freetype
                    iconv
                    )
# 头文件
target_include_directories(${PROJECT_NAME} PRIVATE                        
                            ${OpenCV_INCLUDE_DIRS}
                            ${CMAKE_CURRENT_SOURCE_DIR}/code
                            ${CMAKE_CURRENT_SOURCE_DIR}
                            ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/rknpu2/include
                            ${CMAKE_CURRENT_SOURCE_DIR}/common
                            ${CMAKE_CURRENT_SOURCE_DIR}/common/isp3.x   
                            ${CMAKE_CURRENT_SOURCE_DIR}/include
                            ${CMAKE_CURRENT_SOURCE_DIR}/include/rknn
                            ${CMAKE_CURRENT_SOURCE_DIR}/include/rkaiq
                            ${CMAKE_CURRENT_SOURCE_DIR}/include/rkaiq/uAPI2
                            ${CMAKE_CURRENT_SOURCE_DIR}/include/rkaiq/common
                            ${CMAKE_CURRENT_SOURCE_DIR}/include/rkaiq/xcore
                            ${CMAKE_CURRENT_SOURCE_DIR}/include/rkaiq/algos
                            ${CMAKE_CURRENT_SOURCE_DIR}/include/rkaiq/iq_parser
                            ${CMAKE_CURRENT_SOURCE_DIR}/include/rkaiq/iq_parser_v2
                            ${CMAKE_CURRENT_SOURCE_DIR}/include/rkaiq/smartIr
                            ${CMAKE_CURRENT_SOURCE_DIR}/include/freetype2
                            ${CMAKE_CURRENT_SOURCE_DIR}/code/osd
                            ${CMAKE_CURRENT_SOURCE_DIR}/code/param
                            ${CMAKE_CURRENT_SOURCE_DIR}/code/display
                            ${CMAKE_CURRENT_SOURCE_DIR}/code/log
                            ${CMAKE_CURRENT_SOURCE_DIR}/code/net
                            ${CMAKE_CURRENT_SOURCE_DIR}/code/video
                            ${CMAKE_CURRENT_SOURCE_DIR}/code/ptz
                            ${CMAKE_CURRENT_SOURCE_DIR}/code/control
                            ${CMAKE_CURRENT_SOURCE_DIR}/code/onvif/soap
                            ${CMAKE_CURRENT_SOURCE_DIR}/code/onvif/server
                            ${CMAKE_CURRENT_SOURCE_DIR}/code/tcp
                            ${CMAKE_CURRENT_SOURCE_DIR}/code/factory
                            ${CMAKE_CURRENT_SOURCE_DIR}/code/observer
                          )

message(STATUS "model dir PATH:model")

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/install/${PROJECT_NAME}_demo")
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/code/rkipc.ini DESTINATION ${CMAKE_INSTALL_PREFIX})
SET(MODEL_DIR "model")
file(GLOB MODEL_FILES "${MODEL_DIR}/*")  
install(FILES ${MODEL_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/model)                    
