cmake_minimum_required(VERSION 3.21)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin" AND NOT DEFINED CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures")
endif()

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" BACKTYPE_VERSION_LABEL LIMIT_COUNT 1)
if(NOT BACKTYPE_VERSION_LABEL MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)-pre\\.([0-9]+)$")
    message(FATAL_ERROR "VERSION must use the form major.minor.patch-pre.build")
endif()
set(BACKTYPE_VERSION_MAJOR "${CMAKE_MATCH_1}")
set(BACKTYPE_VERSION_MINOR "${CMAKE_MATCH_2}")
set(BACKTYPE_VERSION_PATCH "${CMAKE_MATCH_3}")
set(BACKTYPE_VERSION_BUILD "${CMAKE_MATCH_4}")

project(BackType
    VERSION "${BACKTYPE_VERSION_MAJOR}.${BACKTYPE_VERSION_MINOR}.${BACKTYPE_VERSION_PATCH}"
    LANGUAGES CXX
)
set(BACKTYPE_BUNDLE_VERSION "${PROJECT_VERSION}.${BACKTYPE_VERSION_BUILD}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE)
    enable_language(OBJCXX)
endif()

option(BACKTYPE_BUILD_AE_PLUGIN "Build the After Effects plugin target" OFF)
option(BACKTYPE_BUILD_TESTS "Build platform-neutral tests" ON)
option(BACKTYPE_WARNINGS_AS_ERRORS "Treat BackType warnings as errors" OFF)
option(BACKTYPE_ENABLE_ASAN "Enable AddressSanitizer for tests" OFF)
option(BACKTYPE_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer for tests" OFF)
option(BACKTYPE_ENABLE_TSAN "Enable ThreadSanitizer for tests" OFF)

if(BACKTYPE_ENABLE_TSAN AND (BACKTYPE_ENABLE_ASAN OR BACKTYPE_ENABLE_UBSAN))
    message(FATAL_ERROR "ThreadSanitizer must use a separate build from ASan/UBSan")
endif()

math(EXPR BACKTYPE_AE_EFFECT_VERSION
    "((${BACKTYPE_VERSION_MAJOR} & 7) << 19) | ((${BACKTYPE_VERSION_MAJOR} >> 3) << 26) | ((${BACKTYPE_VERSION_MINOR} & 15) << 15) | ((${BACKTYPE_VERSION_PATCH} & 15) << 11) | ((${BACKTYPE_VERSION_BUILD} & 511))"
    OUTPUT_FORMAT HEXADECIMAL
)

set(BACKTYPE_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${BACKTYPE_GENERATED_DIR}")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/BackType_Version.h.in"
    "${BACKTYPE_GENERATED_DIR}/BackType_Version.h"
    @ONLY
)

add_library(backtype_core STATIC
    src/BackType_RasterLogic.cpp
    src/BackType_TextLogic.cpp
)
target_include_directories(backtype_core PUBLIC src "${BACKTYPE_GENERATED_DIR}")
set_target_properties(backtype_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

function(backtype_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /Zc:__cplusplus)
        if(BACKTYPE_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
        if(BACKTYPE_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

function(backtype_enable_sanitizers target)
    if(MSVC)
        if(BACKTYPE_ENABLE_ASAN)
            target_compile_options(${target} PRIVATE /fsanitize=address)
        endif()
        return()
    endif()
    if(BACKTYPE_ENABLE_ASAN)
        target_compile_options(${target} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address)
    endif()
    if(BACKTYPE_ENABLE_UBSAN)
        target_compile_options(${target} PRIVATE -fsanitize=undefined -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=undefined)
    endif()
    if(BACKTYPE_ENABLE_TSAN)
        target_compile_options(${target} PRIVATE -fsanitize=thread -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=thread)
    endif()
endfunction()

backtype_enable_warnings(backtype_core)
backtype_enable_sanitizers(backtype_core)

if(BACKTYPE_BUILD_TESTS)
    include(CTest)
    find_package(Threads REQUIRED)

    add_executable(backtype_text_logic_tests tests/BackType_TextLogicTests.cpp)
    target_link_libraries(backtype_text_logic_tests PRIVATE backtype_core)
    backtype_enable_warnings(backtype_text_logic_tests)
    backtype_enable_sanitizers(backtype_text_logic_tests)

    add_executable(backtype_raster_logic_tests tests/BackType_RasterLogicTests.cpp)
    target_link_libraries(backtype_raster_logic_tests PRIVATE backtype_core Threads::Threads)
    backtype_enable_warnings(backtype_raster_logic_tests)
    backtype_enable_sanitizers(backtype_raster_logic_tests)

    add_test(NAME backtype_text_logic COMMAND backtype_text_logic_tests)
    add_test(NAME backtype_raster_logic COMMAND backtype_raster_logic_tests)

endif()

if(NOT BACKTYPE_BUILD_AE_PLUGIN)
    return()
endif()

set(AE_SDK_ROOT "$ENV{AE_SDK_ROOT}" CACHE PATH "Path to the Adobe After Effects C++ SDK root")
if(NOT AE_SDK_ROOT)
    message(FATAL_ERROR "Set AE_SDK_ROOT or pass -DAE_SDK_ROOT=... to build the plugin.")
endif()

set(BACKTYPE_AE_SDK_CANDIDATES "${AE_SDK_ROOT}" "${AE_SDK_ROOT}/Examples")
file(GLOB BACKTYPE_AE_SDK_EXTRACTED_CANDIDATES LIST_DIRECTORIES true
    "${AE_SDK_ROOT}/AfterEffectsSDK_*_mac/ae*.AfterEffectsSDK/Examples"
    "${AE_SDK_ROOT}/AfterEffectsSDK_*_win/ae*.AfterEffectsSDK/Examples"
    "${AE_SDK_ROOT}/ae*.AfterEffectsSDK/Examples"
)
list(APPEND BACKTYPE_AE_SDK_CANDIDATES ${BACKTYPE_AE_SDK_EXTRACTED_CANDIDATES})

set(BACKTYPE_AE_SDK_EXAMPLES_DIR "")
foreach(candidate IN LISTS BACKTYPE_AE_SDK_CANDIDATES)
    if(EXISTS "${candidate}/Headers/AE_Effect.h" AND EXISTS "${candidate}/Util/Param_Utils.h")
        set(BACKTYPE_AE_SDK_EXAMPLES_DIR "${candidate}")
        break()
    endif()
endforeach()
if(NOT BACKTYPE_AE_SDK_EXAMPLES_DIR)
    message(FATAL_ERROR "Could not find the AE SDK Examples directory under AE_SDK_ROOT='${AE_SDK_ROOT}'.")
endif()
message(STATUS "Using AE SDK examples directory: ${BACKTYPE_AE_SDK_EXAMPLES_DIR}")

set(BACKTYPE_SDK_INCLUDE_DIRS
    "${BACKTYPE_AE_SDK_EXAMPLES_DIR}/Headers"
    "${BACKTYPE_AE_SDK_EXAMPLES_DIR}/Headers/SP"
    "${BACKTYPE_AE_SDK_EXAMPLES_DIR}/Resources"
    "${BACKTYPE_AE_SDK_EXAMPLES_DIR}/Util"
)

if(WIN32)
    enable_language(RC)
endif()

add_library(BackType MODULE src/BackType.cpp src/BackType_Strings.cpp)
target_link_libraries(BackType PRIVATE backtype_core)
target_include_directories(BackType PRIVATE src "${BACKTYPE_GENERATED_DIR}")
target_include_directories(BackType SYSTEM PRIVATE ${BACKTYPE_SDK_INCLUDE_DIRS})
backtype_enable_warnings(BackType)
backtype_enable_sanitizers(BackType)

if(WIN32)
    set(AE_PIPL_TOOL "${BACKTYPE_AE_SDK_EXAMPLES_DIR}/Resources/PiPLtool.exe" CACHE FILEPATH "Path to Adobe PiPLTool.exe")
    set(BACKTYPE_PREBUILT_PIPL_RRC "" CACHE FILEPATH "Optional prebuilt BackType_PiPL.rrc")

    if(BACKTYPE_PREBUILT_PIPL_RRC)
        set(BACKTYPE_PIPL_RRC "${BACKTYPE_PREBUILT_PIPL_RRC}")
    elseif(EXISTS "${AE_PIPL_TOOL}")
        set(BACKTYPE_PIPL_RR "${BACKTYPE_GENERATED_DIR}/BackType_PiPL.rr")
        set(BACKTYPE_PIPL_RRC "${BACKTYPE_GENERATED_DIR}/BackType_PiPL.rrc")
        if(CMAKE_GENERATOR_PLATFORM MATCHES "ARM64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "[Aa][Rr][Mm]64")
            set(BACKTYPE_PIPL_ARCH_DEFINE AE_PROC_ARM64=1)
        else()
            set(BACKTYPE_PIPL_ARCH_DEFINE AE_PROC_INTELx64=1)
        endif()
        add_custom_command(
            OUTPUT "${BACKTYPE_PIPL_RR}"
            COMMAND "${CMAKE_CXX_COMPILER}" /nologo /EP /P "/Fi${BACKTYPE_PIPL_RR}"
                /DAE_OS_WIN=1 "/D${BACKTYPE_PIPL_ARCH_DEFINE}"
                "/I${BACKTYPE_AE_SDK_EXAMPLES_DIR}/Headers"
                "/I${BACKTYPE_AE_SDK_EXAMPLES_DIR}/Resources"
                "/I${CMAKE_CURRENT_SOURCE_DIR}/src"
                "/I${BACKTYPE_GENERATED_DIR}"
                "/Tc${CMAKE_CURRENT_SOURCE_DIR}/src/BackType_PiPL.r"
            DEPENDS src/BackType_PiPL.r src/BackType.h "${BACKTYPE_GENERATED_DIR}/BackType_Version.h"
            VERBATIM
        )
        add_custom_command(
            OUTPUT "${BACKTYPE_PIPL_RRC}"
            COMMAND "${AE_PIPL_TOOL}" "${BACKTYPE_PIPL_RR}" "${BACKTYPE_PIPL_RRC}"
            DEPENDS "${BACKTYPE_PIPL_RR}"
            VERBATIM
        )
    else()
        message(FATAL_ERROR "Windows builds require AE_PIPL_TOOL or BACKTYPE_PREBUILT_PIPL_RRC.")
    endif()

    cmake_path(CONVERT "${BACKTYPE_PIPL_RRC}" TO_CMAKE_PATH_LIST BACKTYPE_PIPL_RRC NORMALIZE)
    configure_file(packaging/BackType.rc.in "${BACKTYPE_GENERATED_DIR}/BackType.rc" @ONLY)
    set_source_files_properties("${BACKTYPE_GENERATED_DIR}/BackType.rc" PROPERTIES
        OBJECT_DEPENDS "${BACKTYPE_PIPL_RRC}"
    )
    target_sources(BackType PRIVATE "${BACKTYPE_GENERATED_DIR}/BackType.rc" "${BACKTYPE_PIPL_RRC}")
    target_compile_definitions(BackType PRIVATE WIN_ENV=1 _WINDOWS=1 NOMINMAX=1)
    set_target_properties(BackType PROPERTIES PREFIX "" OUTPUT_NAME BackType SUFFIX ".aex")
elseif(APPLE)
    target_compile_definitions(BackType PRIVATE MAC_ENV=1)
    target_link_libraries(BackType PRIVATE "-framework CoreFoundation")
    set_target_properties(BackType PROPERTIES
        PREFIX ""
        OUTPUT_NAME BackType
        BUNDLE TRUE
        BUNDLE_EXTENSION plugin
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/packaging/Info.plist.in"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
    add_custom_command(TARGET BackType POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:BackType>/Contents/Resources"
        COMMAND /usr/bin/Rez -useDF
            -I "${BACKTYPE_AE_SDK_EXAMPLES_DIR}/Headers"
            -I "${BACKTYPE_AE_SDK_EXAMPLES_DIR}/Resources"
            -I "${CMAKE_CURRENT_SOURCE_DIR}/src"
            -I "${BACKTYPE_GENERATED_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/BackType_PiPL.r"
            -o "$<TARGET_BUNDLE_DIR:BackType>/Contents/Resources/BackType.rsrc"
        VERBATIM
    )
else()
    message(FATAL_ERROR "BackType plugin targets support Windows and macOS only.")
endif()

install(TARGETS BackType
    BUNDLE DESTINATION .
    LIBRARY DESTINATION .
    RUNTIME DESTINATION .
)
install(FILES README.md LICENSE DESTINATION .)
set(CPACK_GENERATOR ZIP)
if(APPLE AND CMAKE_OSX_ARCHITECTURES MATCHES ";")
    set(BACKTYPE_PACKAGE_PLATFORM "macOS-universal")
elseif(WIN32 AND CMAKE_GENERATOR_PLATFORM)
    set(BACKTYPE_PACKAGE_PLATFORM "Windows-${CMAKE_GENERATOR_PLATFORM}")
else()
    set(BACKTYPE_PACKAGE_PLATFORM "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
endif()
set(CPACK_PACKAGE_FILE_NAME "BackType-v${BACKTYPE_VERSION_LABEL}-${BACKTYPE_PACKAGE_PLATFORM}")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
include(CPack)
