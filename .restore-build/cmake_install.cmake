# Install script for directory: /home/geng/tool/luckfox-pico/shili/rkipc-terminal

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/home/geng/tool/luckfox-pico/shili/rkipc-terminal/install/rkipc_terminal_demo")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/home/geng/tool/luckfox-pico/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-objdump")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}/home/geng/tool/luckfox-pico/shili/rkipc-terminal/install/rkipc_terminal_demo/rkipc_terminal" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/geng/tool/luckfox-pico/shili/rkipc-terminal/install/rkipc_terminal_demo/rkipc_terminal")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/home/geng/tool/luckfox-pico/shili/rkipc-terminal/install/rkipc_terminal_demo/rkipc_terminal"
         RPATH "$ORIGIN/lib")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/geng/tool/luckfox-pico/shili/rkipc-terminal/install/rkipc_terminal_demo/rkipc_terminal")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/geng/tool/luckfox-pico/shili/rkipc-terminal/install/rkipc_terminal_demo" TYPE EXECUTABLE FILES "/home/geng/tool/luckfox-pico/shili/rkipc-terminal/.restore-build/rkipc_terminal")
  if(EXISTS "$ENV{DESTDIR}/home/geng/tool/luckfox-pico/shili/rkipc-terminal/install/rkipc_terminal_demo/rkipc_terminal" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/geng/tool/luckfox-pico/shili/rkipc-terminal/install/rkipc_terminal_demo/rkipc_terminal")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}/home/geng/tool/luckfox-pico/shili/rkipc-terminal/install/rkipc_terminal_demo/rkipc_terminal"
         OLD_RPATH "/home/geng/tool/luckfox-pico/shili/rkipc-terminal/3rdparty/rknpu2/Linux/armhf-uclibc:/home/geng/tool/luckfox-pico/shili/rkipc-terminal/lib:/home/geng/tool/luckfox-pico/shili/rkipc-terminal/lib/arm-rockchip830-linux-uclibcgnueabihf:"
         NEW_RPATH "$ORIGIN/lib")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/home/geng/tool/luckfox-pico/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-strip" "$ENV{DESTDIR}/home/geng/tool/luckfox-pico/shili/rkipc-terminal/install/rkipc_terminal_demo/rkipc_terminal")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/geng/tool/luckfox-pico/shili/rkipc-terminal/install/rkipc_terminal_demo/rkipc.ini")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/geng/tool/luckfox-pico/shili/rkipc-terminal/install/rkipc_terminal_demo" TYPE FILE FILES "/home/geng/tool/luckfox-pico/shili/rkipc-terminal/code/rkipc.ini")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/geng/tool/luckfox-pico/shili/rkipc-terminal/install/rkipc_terminal_demo/model/coco_80_labels_list.txt;/home/geng/tool/luckfox-pico/shili/rkipc-terminal/install/rkipc_terminal_demo/model/yolov5.rknn")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/geng/tool/luckfox-pico/shili/rkipc-terminal/install/rkipc_terminal_demo/model" TYPE FILE FILES
    "/home/geng/tool/luckfox-pico/shili/rkipc-terminal/model/coco_80_labels_list.txt"
    "/home/geng/tool/luckfox-pico/shili/rkipc-terminal/model/yolov5.rknn"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/geng/tool/luckfox-pico/shili/rkipc-terminal/.restore-build/3rdparty.out/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/geng/tool/luckfox-pico/shili/rkipc-terminal/.restore-build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
