cmake_minimum_required(VERSION 3.0)

include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
include("$ENV{VITASDK}/share/vita.toolchain.cmake" REQUIRED)

project(log_module)

# set build date variable
execute_process(COMMAND "date" "+%Y.%m.%d" OUTPUT_VARIABLE BUILD_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)

set(SRC_FILES
        src/main.c
        )
set(INC_FILES
        include
        )

set(CFLAGS -Wall -O2 -std=c99 -D__PSP2__ -DMODULE
        -Wno-unused-variable -Wno-unused-but-set-variable -Wno-format
        -Wno-strict-aliasing -Wno-attributes -ffreestanding
        -DVERSION=\"${BUILD_DATE}\"
        )

# user module
add_executable(${PROJECT_NAME} ${SRC_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${INC_FILES})
target_compile_options(${PROJECT_NAME} PRIVATE ${CFLAGS} -DMODULE)

set(LDFLAGS
        taihen_stub
        SceLibKernel_stub
        SceKernelThreadMgr_stub
        SceKernelModulemgr_stub
        SceSysmodule_stub
        SceSysmem_stub
        SceIofilemgr_stub
        SceNet_stub_weak
        SceNetCtl_stub_weak
        SceAppMgr_stub
        ScePower_stub
        k
        gcc
        -nostdlib
        )

target_link_libraries(${PROJECT_NAME} ${LDFLAGS})

vita_create_self(${PROJECT_NAME}.suprx ${PROJECT_NAME}
        CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/log_module.yml
        UNSAFE)
