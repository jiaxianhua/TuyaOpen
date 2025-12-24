##
# @file CMakeLists.txt
# @brief 4.26inch E-Paper Network Novel Reader
#/

# APP_PATH
set(APP_PATH ${CMAKE_CURRENT_LIST_DIR})

# APP_NAME
get_filename_component(APP_NAME ${APP_PATH} NAME)

# APP SRC - Set the source files by directory
file(GLOB APP_SRC_EXAMPLES    "${APP_PATH}/examples/*.c")
file(GLOB APP_SRC_SRC         "${APP_PATH}/src/*.c")
file(GLOB APP_SRC_CONFIG      "${APP_PATH}/lib/Config/*.c")
file(GLOB APP_SRC_EPAPER      "${APP_PATH}/lib/e-Paper/*.c")
file(GLOB APP_SRC_FONTS       "${APP_PATH}/lib/Fonts/*.c")
file(GLOB APP_SRC_GUI         "${APP_PATH}/lib/GUI/*.c")

set(APP_SRC
    ${APP_SRC_EXAMPLES}
    ${APP_SRC_SRC}
    ${APP_SRC_CONFIG}
    ${APP_SRC_EPAPER}
    ${APP_SRC_FONTS}
    ${APP_SRC_GUI}
)

# Exclude common build/cache directories
list(FILTER APP_SRC EXCLUDE REGEX "/CMakeFiles/")
list(FILTER APP_SRC EXCLUDE REGEX "\\.build/")
list(FILTER APP_SRC EXCLUDE REGEX "\\.git/")

# APP INC - Set the header file directory
set(APP_INC 
    ${APP_PATH}/examples
    ${APP_PATH}/include
    ${APP_PATH}
    ${APP_PATH}/lib/Config
    ${APP_PATH}/lib/e-Paper
    ${APP_PATH}/lib/Fonts
    ${APP_PATH}/lib/GUI
)

# APP_OPTIONS
set(APP_OPTIONS "-DDEBUG")

# Disable treating warnings as errors
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    list(APPEND APP_OPTIONS "-Wno-error")
elseif(MSVC)
    list(APPEND APP_OPTIONS "/WX-")
endif()

########################################
# Target Configure
########################################
add_library(${EXAMPLE_LIB})
message(STATUS "EXAMPLE_LIB:${APP_PATH}")

target_sources(${EXAMPLE_LIB}
    PRIVATE
        ${APP_SRC}
)

target_include_directories(${EXAMPLE_LIB}
    PRIVATE
        ${APP_INC}
)

target_compile_options(${EXAMPLE_LIB}
    PRIVATE
        ${APP_OPTIONS}
)
