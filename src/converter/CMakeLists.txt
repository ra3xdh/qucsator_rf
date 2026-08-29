include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src/math
  ${CMAKE_CURRENT_BINARY_DIR}) # qucdefs.h

set(QUCSCONV_SRC
    check_spice.cpp
    check_vcd.cpp
    matlab_producer.cpp
    csv_producer.cpp
    qucs_producer.cpp
    qucsconv.cpp
    touchstone_producer.cpp)


set(ParserTypes spice vcd)

set(conv_generated)
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
  list(APPEND conv_generated ${bisonOut})
  list(APPEND conv_generated ${flexOut})
endforeach()

#
# qucsator is needed before qucsconv can be compiled run qucsator -l > pipe to
# qucsdef.h
#
add_custom_command(
  OUTPUT qucsdefs.h
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/../qucsator_rf -l >
          ${CMAKE_CURRENT_BINARY_DIR}/qucsdefs.h
  DEPENDS qucsator_rf)

add_custom_target(defs DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/qucsdefs.h)

add_executable(qucsconv_rf ${QUCSCONV_SRC} ${conv_generated})

add_dependencies(qucsconv_rf defs)

target_link_libraries(qucsconv_rf libqucsator ${CMAKE_DL_LIBS})

#
# Handle installation
#
install(TARGETS qucsconv_rf DESTINATION bin)
