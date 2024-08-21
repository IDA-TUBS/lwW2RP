cmake_minimum_required(VERSION 3.0.0)
project(w2rp VERSION 0.1.0)
set (CMAKE_CXX_STANDARD 17)

include(GNUInstallDirs)

### cmake macros
include(${PROJECT_SOURCE_DIR}/cmake/dev/set_sources.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/dev/generate_msvc_libraries.cmake)

### Build directory structure
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

### search for local boost
find_package(Boost 1.73.0 EXACT COMPONENTS thread log log_setup REQUIRED)

find_package(OpenCV REQUIRED)
include_directories( ${OpenCV_INCLUDE_DIRS} )

### Define library content
set(
    LIB_SRC_FILES
    ${PROJECT_SOURCE_DIR}/src/w2rp/timer/timerManager.cpp
    
    ${PROJECT_SOURCE_DIR}/src/w2rp/entities/writer.cpp
    ${PROJECT_SOURCE_DIR}/src/w2rp/entities/reader.cpp
    
    ${PROJECT_SOURCE_DIR}/src/w2rp/helper/fragmentation.cpp
    
    ${PROJECT_SOURCE_DIR}/src/w2rp/messages/messages.cpp
    
    ${PROJECT_SOURCE_DIR}/src/w2rp/comm/UDPComm.cpp

    ${PROJECT_SOURCE_DIR}/src/w2rp/config/readerConfig.cpp
    ${PROJECT_SOURCE_DIR}/src/w2rp/config/writerConfig.cpp
    ${PROJECT_SOURCE_DIR}/src/w2rp/config/setupConfig.cpp

    ${PROJECT_SOURCE_DIR}/src/w2rp/changeForReader.cpp
    ${PROJECT_SOURCE_DIR}/src/w2rp/changeForWriter.cpp
    ${PROJECT_SOURCE_DIR}/src/w2rp/readerProxy.cpp
    ${PROJECT_SOURCE_DIR}/src/w2rp/writerProxy.cpp
    ${PROJECT_SOURCE_DIR}/src/w2rp/log.cpp
    
)

## Define library content
add_library(w2rp SHARED ${LIB_SRC_FILES})

target_include_directories(w2rp
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(w2rp PUBLIC boost_system boost_log boost_log_setup boost_thread pthread)

target_compile_definitions(w2rp PUBLIC BOOST_LOG_DYN_LINK)

# Enable logging via -DLOG during build process
if(DEFINED LOG)
    message("Logging activated")
    target_compile_definitions(w2rp PUBLIC LOG_ON)
endif()

# Enable file log via -DFILE during build process
if(DEFINED FILE)
    message("File logging activated")
    target_compile_definitions(w2rp PUBLIC FILE_ON)
endif()

# Enable console log via -DCONSOLE during build process
if(DEFINED CONSOLE)
    message("Console logging activated")
    target_compile_definitions(w2rp PUBLIC CONSOLE_ON)
endif()

target_compile_options(w2rp PUBLIC -g)

include(CMakePackageConfigHelpers)

### Add headers to system include dir 
set_public_headers_directory(${PROJECT_SOURCE_DIR}/include w2rp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT headers
    INSTALL
)

### Install lib
install(
    TARGETS w2rp
    EXPORT w2rpTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    EXPORT w2rpTargets
    NAMESPACE w2rp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/w2rp
    FILE w2rpTargets.cmake
)

### Install template implementation files
install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} 
    FILES_MATCHING PATTERN "*.tpp"
)

### Add package info for cmake
configure_package_config_file( 
    "w2rpConfig.cmake.in" 
    "w2rpConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/w2rp
    PATH_VARS
        CMAKE_INSTALL_LIBDIR
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/w2rpConfigVersion.cmake
    VERSION 0.1.0
    COMPATIBILITY SameMajorVersion
)

### Install Config and ConfigVersion files
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/w2rpConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/w2rpConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/w2rp"
)

### Build Targets
file(GLOB_RECURSE ALL_HEADERS
    ${PROJECT_SOURCE_DIR}/include/w2rp/**/*.hpp
    ${PROJECT_SOURCE_DIR}/include/w2rp/*.hpp
)

set(SRC_FILES 
  ${ALL_HEADERS}

  ${PROJECT_SOURCE_DIR}/src/w2rp/config/readerConfig.cpp
  ${PROJECT_SOURCE_DIR}/src/w2rp/config/writerConfig.cpp
  ${PROJECT_SOURCE_DIR}/src/w2rp/config/setupConfig.cpp
)

# ## TimedEvent testing
# add_executable(timer_test_function test/timer/timer_test_function.cpp ${SRC_FILES})
# target_link_libraries(timer_test_function w2rp)

# ## TimedEvent testing
# add_executable(timer_test_object test/timer/timer_test_object.cpp ${SRC_FILES})
# target_link_libraries(timer_test_object w2rp)

# ## Writer testing
# add_executable(writer_test test/entities/writer_test.cpp ${SRC_FILES})
# target_link_libraries(writer_test w2rp)

# ## Reader testing
# add_executable(reader_test test/entities/reader_test.cpp ${SRC_FILES})
# target_link_libraries(reader_test w2rp)

# ## Config testing
# add_executable(readConfig_test test/config/readConfig_test.cpp ${SRC_FILES})
# target_link_libraries(readConfig_test w2rp)

## Writer testing (video stream)
add_executable(writer_test_video test/app_videoStream/writer_test.cpp ${SRC_FILES})
target_link_libraries(writer_test_video w2rp ${OpenCV_LIBS})

## Reader testing (video stream)
add_executable(reader_test_video test/app_videoStream/reader_test.cpp ${SRC_FILES})
target_link_libraries(reader_test_video w2rp ${OpenCV_LIBS})

# ## GUID testing
# add_executable(guid_test test/guid/guid_test.cpp ${SRC_FILES})
# target_link_libraries(guid_test w2rp)

