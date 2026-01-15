##
# @file CMakeLists.txt
# @brief 
#/

# APP_PATH
set(APP_PATH ${CMAKE_CURRENT_LIST_DIR})

# APP_NAME
get_filename_component(APP_NAME ${APP_PATH} NAME)

# APP_SRCS
aux_source_directory(${APP_PATH}/src APP_SRCS)

# Add utf8_to_gbk source
list(APPEND APP_SRCS 
    ${CMAKE_SOURCE_DIR}/apps/tuya_t5_pocket/tuya_t5_pocket_ai/src/expand/src/utf8_to_gbk.c
)

########################################
# Target Configure
########################################
add_library(${EXAMPLE_LIB})

target_include_directories(${EXAMPLE_LIB}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/apps/tuya_t5_pocket/tuya_t5_pocket_ai/src/expand/inc
)

target_sources(${EXAMPLE_LIB}
    PRIVATE
        ${APP_SRCS}
    )