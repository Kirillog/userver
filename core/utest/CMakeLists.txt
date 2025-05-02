project(userver-utest CXX)

file(GLOB_RECURSE UNIT_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*_test.cpp
)
file(GLOB_RECURSE SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)
list(REMOVE_ITEM SOURCES ${UNIT_TEST_SOURCES})

add_library(userver-utest STATIC
    ${SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/main/main.cpp
)
target_compile_definitions(userver-utest PUBLIC
    $<TARGET_PROPERTY:userver-core,COMPILE_DEFINITIONS>
)
target_link_libraries(userver-utest
    PUBLIC
    userver-core
    userver-universal-utest-base
    PRIVATE
    userver-core-internal
    Boost::program_options
)
target_include_directories(userver-utest
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
    $<TARGET_PROPERTY:userver-core,INCLUDE_DIRECTORIES>
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
_userver_directory_install(COMPONENT core
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/..
)
_userver_install_targets(COMPONENT core
    TARGETS userver-utest
)

if(USERVER_BUILD_TESTS)
  add_executable(${PROJECT_NAME}-unittest ${UNIT_TEST_SOURCES})
  target_include_directories(${PROJECT_NAME}-unittest SYSTEM PRIVATE
      $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
  )
  target_link_libraries(${PROJECT_NAME}-unittest PUBLIC ${PROJECT_NAME})
  add_google_tests(${PROJECT_NAME}-unittest)
endif()

find_package(Boost REQUIRED COMPONENTS context)
set(CMAKE_ASAN_FLAGS -fsanitize=address -fsanitize=undefined -DADDRESS_SANITIZER)

add_executable(lincheck-test test.cpp)

target_compile_options(lincheck-test PRIVATE -fpass-plugin=${LTEST_BUILD_PATH}/codegen/libYieldPass.so ${CMAKE_ASAN_FLAGS})

target_link_options(lincheck-test PRIVATE ${CMAKE_ASAN_FLAGS})

target_compile_definitions(lincheck-test PUBLIC
    $<TARGET_PROPERTY:userver-core,COMPILE_DEFINITIONS>
)

target_include_directories(lincheck-test
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
    $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
    "/workspaces/LTest"
)


target_link_libraries(lincheck-test
    PRIVATE
    gflags
    ${LTEST_BUILD_PATH}/runtime/libruntime.so
    ${Boost_LIBRARIES}
    userver-core
)
