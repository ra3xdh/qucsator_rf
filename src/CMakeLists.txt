cmake_minimum_required(VERSION 3.10)
project(qucsator_rf CXX C)

#
# Checks for libraries.
#
# AC_CHECK_LIB(m, sin) need to check for sin?
if(NOT WIN32)
  find_library(MATH_LIB NAMES m)
  if(NOT MATH_LIB)
    message(SEND_ERROR "Math lib not found: ${MATH_LIB}")
  else()
    message(STATUS "Math lib found at: ${MATH_LIB}")
  endif()
endif()

#
# Check for type sizes.
#
include(CheckTypeSize)

#
# Check for double type. * valid types are: double, float and long double. *
# defines: nr_double_t,  The global type of double representation. * defines:
# NR_DOUBLE_SIZE,  The size of the double representation. * Use -DENABLE-
# DOUBLE="[float,double, long double]"
if(ENABLE-DOUBLE)
  # User defined
  set(DoubleType ${ENABLE-DOUBLE})

  # valid types
  set(ValidTypes "float" "double" "long double")

  list(FIND ValidTypes ${DoubleType} HasType)
  if(HasType EQUAL -1)
    message(FATAL_ERROR "Valid types are: ${ValidTypes}")
  endif()

  # The global type of double representation.
  set(nr_double_t DoubleType)
  check_type_size(${DoubleType} DoubleSize)

  # The size of the double representation.
  set(NR_DOUBLE_SIZE ${DoubleSize})
else()
  # Default double
  set(DoubleType "double")
  # The global type of double representation.
  set(nr_double_t ${DoubleType})
  check_type_size(${DoubleType} DoubleSize)

  # The size of the double representation.
  set(NR_DOUBLE_SIZE ${DoubleSize})
endif()
message(STATUS "using double type: ${DoubleType}; size: ${DoubleSize}")

# defines used in qucs_typedefs.h
set(QUCS_INT32_TYPE ${nr_int32_t})
set(QUCS_INT16_TYPE ${nr_int16_t})
set(QUCS_DOUBLE_TYPE ${DoubleType})
set(QUCS_DOUBLE_SIZE ${DoubleSize})
#
# Configure the header qucs_typedefs.h, interpolate above definitions.
#
configure_file("${qucs-core_SOURCE_DIR}/qucs_typedefs.h.cmake"
               "${qucs-core_BINARY_DIR}/qucs_typedefs.h")


#
# Configure the header config.h, interpolate above definitions.
#
configure_file("${qucs-core_SOURCE_DIR}/config.h.cmake"
               "${qucs-core_BINARY_DIR}/config.h")

#
# List of lexer/parsers type names
#
set(ParserTypes
    csv
    citi
    dataset
    mdl
    netlist
    touchstone
    zvr)

set(generated_SRC)
foreach(type ${ParserTypes})
  # Create custom Bison
  set(bisonIn "${CMAKE_CURRENT_SOURCE_DIR}/parse_${type}.ypp")
  set(bisonOut "parse_${type}.hpp"
               "parse_${type}.cpp")
  add_custom_command(
    OUTPUT ${bisonOut}
    COMMAND
      ${BISON_EXECUTABLE}
      --defines=parse_${type}.hpp
      --output=parse_${type}.cpp
      -v
      -Wce
      -Wprecedence
      -Wmidrule-value
      ${bisonIn}
    DEPENDS ${bisonIn})
  # Create custom Flex
  set(flexIn "${CMAKE_CURRENT_SOURCE_DIR}/scan_${type}.lpp")
  set(flexOut "scan_${type}.cpp")
  add_custom_command(
    OUTPUT ${flexOut}
    COMMAND ${FLEX_EXECUTABLE} --outfile=${flexOut} ${flexIn}
    DEPENDS ${flexIn})

  list(APPEND generated_SRC ${bisonOut})
  list(APPEND generated_SRC ${flexOut})
endforeach()


#
# Source code libqucs
#
set(LIBQUCS_SRC
    ${generated_SRC}
    analysis.cpp
    check_zvr.cpp
    interpolator.cpp
    parasweep.cpp
    property.cpp
    range.cpp
    spline.cpp
    strlist.cpp
    trsolver.cpp
    acsolver.cpp
    check_citi.cpp
    check_csv.cpp
    check_dataset.cpp
    check_mdl.cpp
    check_netlist.cpp
    check_touchstone.cpp
    circuit.cpp
    dataset.cpp
    dcsolver.cpp
    devstates.cpp
    differentiate.cpp
    environment.cpp
    equation.cpp # <= depends on gperfapphash.cpp
    evaluate.cpp
    exception.cpp
    exceptionstack.cpp
    fourier.cpp
    hbsolver.cpp
    history.cpp
    input.cpp
    integrator.cpp
    logging.c
    matvec.cpp
    module.cpp
    net.cpp
    nodelist.cpp
    nodeset.cpp
    object.cpp
    receiver.cpp
    spsolver.cpp
    sweep.cpp
    transient.cpp
    variable.cpp
    vector.cpp)

#
# Template classes
#
set(TEMPLATES
    tmatrix.h
    tvector.h
    eqnsys.h
    nasolver.h
    states.h
    tvector.h
    ptrlist.h
    tridiag.h
    hash.h
    valuelist.h
    nasolution.h)

#
# Include headers to be installed
#
set(PUBLIC_HEADERS
    ${qucs-core_BINARY_DIR}/config.h
    ${qucs-core_BINARY_DIR}/qucs_typedefs.h
    circuit.h
    compat.h
    constants.h
    consts.h
    integrator.h
    logging.h
    net.h
    netdefs.h
    node.h
    object.h
    states.h
    valuelist.h
    vector.h
    property.h
    ptrlist.h
    characteristic.h
    pair.h
    operatingpoint.h)

include_directories(
  ${qucs-core_SOURCE_DIR}/src/math # precision.h
  ${qucs-core_SOURCE_DIR}/src/ # compat.h
  ${qucs-core_SOURCE_DIR}/src/components # microstrip/substrate.h
  ${qucs-core_SOURCE_DIR}/src/interface
  ${qucs-core_BINARY_DIR} # cmake generated config.h
  ${qucs-core_BINARY_DIR}/src # cmake generated gperfapphash.h
  ${qucs-core_BINARY_DIR}/src/components # generated verilog/[].core.h
)

add_executable(repl_reg repl_reg.cpp)
IF(WIN32)
    SET(repl_reg_exe repl_reg.exe)
ELSE() # Unix
    SET(repl_reg_exe repl_reg)
ENDIF()
#
# Replace 'evaluate::[whatever]' by NULL
#
# * evaluate.h (class evaluate): New class implementing the actual evaluation
#   function (applications) for the equations in Qucs.
#
add_custom_command(
  OUTPUT gperfappgen.h
  COMMAND ${repl_reg_exe} ${CMAKE_CURRENT_SOURCE_DIR}/applications.h  ${CMAKE_CURRENT_BINARY_DIR}/gperfappgen.h 0
  DEPENDS ${repl_reg_exe} ${applications.h} )

#
# Compile gperfappgen * used to generate gperf input file (used in qucsator)
#
set(gperf_SRC gperfappgen.cpp gperfappgen.h)

add_executable(gperfappgen ${gperf_SRC})

#
# Run gperfappgen, pipe to gperf input to gperfapphash.gph
#
IF(WIN32)
    SET(gperfappgen_exe gperfappgen.exe)
ELSE() # Unix
    SET(gperfappgen_exe gperfappgen)
ENDIF()

add_custom_command(
  OUTPUT gperfapphash.gph
  COMMAND ${gperfappgen_exe} > ${CMAKE_CURRENT_BINARY_DIR}/gperfapphash.gph && dos2unix ${CMAKE_CURRENT_BINARY_DIR}/gperfapphash.gph  
  DEPENDS ${gperfappgen_exe})

#
# Run gperf, create hash table. * -I, Include the necessary system include files
# at the beginning of the code. * -m, Perform multiple iterations to minimize
# generated table. * Replace '{""}' by '{"",0}; (why?)
#
add_custom_command(
  OUTPUT gperfapphash.cpp
  COMMAND ${GPERF_TOOL} -I -m 8 ${CMAKE_CURRENT_BINARY_DIR}/gperfapphash.gph >
          temp.gperf
  COMMAND ${repl_reg_exe} temp.gperf ${CMAKE_CURRENT_BINARY_DIR}/gperfapphash.cpp 1
  DEPENDS gperfapphash.gph ${repl_reg_exe})

# target <- source (includea) equation.cpp: gperfapphash.cpp
#
# noinst_PROGRAMS = gperfappgen gperfappgen_SOURCES = gperfappgen.cpp

# for cleaning (autogenerated) set(gperf_FILES gperfapphash.cpp gperfapphash.gph
# gperfappgen.h)

# Qucs library dependencies
add_subdirectory(interface)
add_subdirectory(components)
add_subdirectory(components/digital)
add_subdirectory(components/devices)
add_subdirectory(components/microstrip)
if (WITH_ADMS)
add_subdirectory(components/verilog)
endif()
add_subdirectory(math)

# Qucsconv application
add_subdirectory(converter)

# Linux? set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--export-all-symbols")

#
# Create qucsator
#
add_executable(qucsator_rf ucs.cpp ${PUBLIC_HEADERS} ${TEMPLATES})

#
# Build libqucs as SHARED, dynamic library
#
# After: - http://stackoverflow.com/questions/11429055/cmake-how-create-a-
# single-shared-library-from-all-static-libraries-of-subprojec
#
if (WITH_ADMS)
add_library(
    libqucsator STATIC
  ${LIBQUCS_SRC}
  ${TEMPLATES}
  $<TARGET_OBJECTS:coreMath>
  $<TARGET_OBJECTS:coreComponents>
  $<TARGET_OBJECTS:coreInterface>
  $<TARGET_OBJECTS:coreVerilog>
  $<TARGET_OBJECTS:coreMicrostrip>
  $<TARGET_OBJECTS:coreDevices>
  $<TARGET_OBJECTS:coreDigital>)
else()
add_library(
    libqucsator STATIC
  ${LIBQUCS_SRC}
  ${TEMPLATES}
  $<TARGET_OBJECTS:coreMath>
  $<TARGET_OBJECTS:coreComponents>
  $<TARGET_OBJECTS:coreInterface>
# $<TARGET_OBJECTS:coreVerilog>
  $<TARGET_OBJECTS:coreMicrostrip>
  $<TARGET_OBJECTS:coreDevices>
  $<TARGET_OBJECTS:coreDigital>)
endif()

# rename the library to let it be libqucsator (not liblibqucsator)
set_target_properties(libqucsator PROPERTIES OUTPUT_NAME qucsator)

#
# Create target to handle gperfapp dependency
#
add_custom_target(equation DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/gperfapphash.cpp
                                   equation.cpp)
add_dependencies(libqucsator equation)

#
# Link qucsator and libqucsator
#
target_link_libraries(qucsator_rf libqucsator ${CMAKE_DL_LIBS})

#
# Handle install
#
install(TARGETS qucsator_rf DESTINATION bin)

# set Windows runtime location for libqucsator See:
# http://www.cmake.org/pipermail/cmake/2010-June/037461.html
#install(
#  TARGETS libqucsator
#  RUNTIME DESTINATION bin COMPONENT runtime
#  ARCHIVE DESTINATION lib COMPONENT devel
#  LIBRARY DESTINATION lib COMPONENT library)

if(WITH_ADMS)
install(FILES ${PUBLIC_HEADERS} DESTINATION include/qucs-core)
endif()
