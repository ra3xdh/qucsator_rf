# Google Test via FetchContent
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.17.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Unit test binary
set(TEST_SOURCES
  testMain.cpp
  test_libqucs.cpp
  Vector.cpp
  Matrix.cpp
  Fourier.cpp
  Math.cpp
  Spline.cpp
  SPFileNoise.cpp
)

add_executable(qucs_test ${TEST_SOURCES})
target_link_libraries(qucs_test gtest_main libqucsator)
target_include_directories(qucs_test PRIVATE
  ${qucs-core_SOURCE_DIR}/src
  ${qucs-core_SOURCE_DIR}/src/math
  ${qucs-core_SOURCE_DIR}/src/components
  ${qucs-core_BINARY_DIR}
  ${qucs-core_BINARY_DIR}/src
)

# Register unit tests with ctest
add_test(NAME qucs_unit COMMAND qucs_test)

# Netlist integration tests
# Run from netlist directory so that SnP file references resolve correctly
file(GLOB_RECURSE NETLIST_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/basic/*@*.net"
)
foreach(NETFILE ${NETLIST_FILES})
  file(RELATIVE_PATH relpath ${CMAKE_CURRENT_SOURCE_DIR} ${NETFILE})
  string(REGEX REPLACE "[/.@-]" "_" test_name ${relpath})
  get_filename_component(netdir ${NETFILE} DIRECTORY)
  add_test(NAME netlist${test_name}
    COMMAND $<TARGET_FILE:qucsator_rf> -i ${NETFILE}
    WORKING_DIRECTORY ${netdir})
endforeach()
