cmake_minimum_required(VERSION 3.16)

project(SectionPropertyTool VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Library/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Library/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Library/bin)

set(SPT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Source)

set(SPT_CORE_SOURCES
    Source/geometry/GeometryUtils.cpp
    Source/geometry/SectionBuilder.cpp
    Source/calculation/SectionCalculator.cpp
    Source/stress/StressPointEngine.cpp
    Source/mesh/MeshEngine.cpp
    Source/import_export/Exporter.cpp
)

add_library(SectionPropertyCore STATIC ${SPT_CORE_SOURCES})
set_target_properties(SectionPropertyCore PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(SectionPropertyCore
    PUBLIC
        ${SPT_SOURCE_DIR}
        ${SPT_SOURCE_DIR}/api
)

add_library(SectionPropertyApi SHARED Source/api/section_property_tool.cpp)
target_link_libraries(SectionPropertyApi PRIVATE SectionPropertyCore)
target_compile_definitions(SectionPropertyApi PRIVATE SPT_BUILD_DLL)
target_include_directories(SectionPropertyApi PUBLIC ${SPT_SOURCE_DIR}/api)
set_target_properties(SectionPropertyApi PROPERTIES
    OUTPUT_NAME SectionPropertyTool
    WINDOWS_EXPORT_ALL_SYMBOLS OFF
)

add_executable(example1_parametric Examples/example1_parametric/main.cpp)
target_link_libraries(example1_parametric PRIVATE SectionPropertyApi)
target_include_directories(example1_parametric PRIVATE ${SPT_SOURCE_DIR}/api)

add_executable(example2_canvas Examples/example2_canvas/main.cpp)
target_link_libraries(example2_canvas PRIVATE SectionPropertyApi)
target_include_directories(example2_canvas PRIVATE ${SPT_SOURCE_DIR}/api)

add_executable(example3_dll_batch Examples/example3_dll_batch/main.cpp)
target_link_libraries(example3_dll_batch PRIVATE SectionPropertyApi)
target_include_directories(example3_dll_batch PRIVATE ${SPT_SOURCE_DIR}/api)

if(BUILD_TESTING)
    add_executable(SectionPropertyTests
        tests/unit/test_main.cpp
        tests/regression/test_regression.cpp
        tests/integration/test_api.cpp
    )
    target_link_libraries(SectionPropertyTests PRIVATE SectionPropertyCore SectionPropertyApi)
    target_include_directories(SectionPropertyTests PRIVATE ${SPT_SOURCE_DIR} ${SPT_SOURCE_DIR}/api)
    add_test(NAME SectionPropertyTests COMMAND SectionPropertyTests)
endif()

set(SPT_QT_VERSION "AUTO" CACHE STRING "Qt major version for the optional GUI target: AUTO, 6, 5, or OFF")
set_property(CACHE SPT_QT_VERSION PROPERTY STRINGS AUTO 6 5 OFF)
string(TOUPPER "${SPT_QT_VERSION}" SPT_QT_VERSION_NORMALIZED)
set(SPT_QT_WIDGETS_TARGET "")

if(SPT_QT_VERSION_NORMALIZED STREQUAL "6")
    find_package(Qt6 COMPONENTS Widgets QUIET)
    if(Qt6_FOUND)
        set(SPT_QT_WIDGETS_TARGET Qt6::Widgets)
    endif()
elseif(SPT_QT_VERSION_NORMALIZED STREQUAL "5")
    find_package(Qt5 COMPONENTS Widgets QUIET)
    if(Qt5_FOUND)
        set(SPT_QT_WIDGETS_TARGET Qt5::Widgets)
    endif()
elseif(SPT_QT_VERSION_NORMALIZED STREQUAL "AUTO")
    find_package(Qt6 COMPONENTS Widgets QUIET)
    if(Qt6_FOUND)
        set(SPT_QT_WIDGETS_TARGET Qt6::Widgets)
    else()
        find_package(Qt5 COMPONENTS Widgets QUIET)
        if(Qt5_FOUND)
            set(SPT_QT_WIDGETS_TARGET Qt5::Widgets)
        endif()
    endif()
elseif(NOT SPT_QT_VERSION_NORMALIZED STREQUAL "OFF")
    message(FATAL_ERROR "SPT_QT_VERSION must be AUTO, 6, 5, or OFF.")
endif()

if(SPT_QT_WIDGETS_TARGET)
    add_executable(SectionPropertyGui Source/gui/main.cpp)
    target_link_libraries(SectionPropertyGui PRIVATE ${SPT_QT_WIDGETS_TARGET} SectionPropertyCore)
    target_include_directories(SectionPropertyGui PRIVATE ${SPT_SOURCE_DIR})
    target_compile_definitions(SectionPropertyGui PRIVATE QT_NO_KEYWORDS)
else()
    message(STATUS "Qt Widgets not found. Core library, DLL, examples, and tests will still build.")
endif()

install(TARGETS SectionPropertyApi
    RUNTIME DESTINATION Library/bin
    LIBRARY DESTINATION Library/bin
    ARCHIVE DESTINATION Library/lib
)
install(FILES Source/api/section_property_tool.h DESTINATION Library/include)
install(DIRECTORY Documents/ DESTINATION Documents)
install(DIRECTORY Examples/ DESTINATION Examples)
