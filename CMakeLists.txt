cmake_minimum_required(VERSION 3.15)

project (mikado)

SET(CMAKE_CXX_STANDARD 14)

LIST(APPEND LIB_SOURCES
    include/mikado.h
    include/packets.h
    include/utils.h
    src/mikado.cpp
    src/packets.cpp
    src/utils.cpp
    )

# Configure gsl-lite
set( GSL_LITE_URL https://github.com/gsl-lite/gsl-lite.git )

include( ExternalProject )
find_package( Git REQUIRED )
set_directory_properties( PROPERTIES EP_PREFIX ${CMAKE_BINARY_DIR}/3rd_party )

ExternalProject_Add(
    gsl-extern
    GIT_REPOSITORY ${GSL_LITE_URL}
    TIMEOUT 10
    UPDATE_COMMAND ${GIT_EXECUTABLE} pull
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
   )

ExternalProject_Get_Property( gsl-extern SOURCE_DIR )
set( GSL_LITE_INCLUDE_DIR ${SOURCE_DIR}/include CACHE INTERNAL "Include folder for gsl-lite" )
add_library( gsl INTERFACE )
target_include_directories( gsl INTERFACE ${GSL_LITE_INCLUDE_DIR} )


SET (LIBRARY_NAME ${PROJECT_NAME})
add_library(${LIBRARY_NAME} ${LIB_SOURCES})
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries( ${LIBRARY_NAME} PUBLIC gsl )
target_compile_definitions( ${LIBRARY_NAME} PUBLIC gsl_CONFIG_DEFAULTS_VERSION=1)

LIST(APPEND TEST_SOURCES
    test/test_mikado.cpp
    test/test_vbi.cpp
    )

LIST(APPEND EXAMPLE_SOURCES
    examples/connect.cpp
    )

ENABLE_TESTING()

foreach(EXAMPLE_SOURCE ${EXAMPLE_SOURCES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE_SOURCE} NAME_WLE)
    MESSAGE(NOTICE "Found example " ${EXAMPLE_NAME})

    add_executable(${EXAMPLE_NAME} ${EXAMPLE_SOURCE})
    target_link_libraries(${EXAMPLE_NAME} ${LIBRARY_NAME})
endforeach()

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.55 REQUIRED COMPONENTS unit_test_framework)

foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WLE)
    MESSAGE(NOTICE "Found test " ${TEST_NAME})

    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME} ${LIBRARY_NAME})

    target_include_directories(${TEST_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(${TEST_NAME} ${Boost_LIBRARIES})

    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})

endforeach()
