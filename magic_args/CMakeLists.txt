add_library(magic_args INTERFACE)
set(
  HEADER_FILES
  # DO NOT SORT THESE
  #
  # These are in the order required for the single-header version
  detail/concepts.hpp
  detail/reflection.hpp
  gnu_style_parsing_traits.hpp
  detail/print.hpp
  argument_definitions.hpp
  detail/get_argument_definition.hpp
  detail/validation.hpp
  program_info.hpp
  incomplete_parse_reason.hpp
  detail/from_string.hpp
  detail/to_formattable.hpp
  detail/visitors.hpp
  detail/overloaded.hpp
  detail/parse.hpp
  dump.hpp
  detail/usage.hpp
  detail/print_incomplete_parse_reason.hpp
  parse.hpp
  magic_args.hpp
  # Extensions that build on top of the core functionality
  ## Subcommands
  value_wrapper_t.hpp
  subcommands/declarations.hpp
  subcommands/parse_subcommands_silent.hpp
  subcommands/print_incomplete_command_parse_reason.hpp
  subcommands/parse_subcommands.hpp
  subcommands/invocable_declarations.hpp
  subcommands/invoke_subcommands_silent.hpp
  subcommands/invoke_subcommands.hpp
  subcommands/is_error.hpp
  subcommands.hpp
  ## Other extensions
  powershell_style_parsing_traits.hpp
  verbatim_names.hpp
  windows.hpp
  enum.hpp
)

# Safety check: verify all .hpp files are listed in HEADER_FILES, so the header-only
# version is complete
file(GLOB_RECURSE ALL_HPP_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.hpp")
foreach (HPP_FILE ${ALL_HPP_FILES})
  if (NOT "${HPP_FILE}" IN_LIST HEADER_FILES AND NOT "${HPP_FILE}" STREQUAL "detail/single_header_prefix.hpp")
    message(FATAL_ERROR "Header file '${HPP_FILE}' is not listed in HEADER_FILES in ${CMAKE_CURRENT_LIST_FILE}")
  endif ()
endforeach ()


# Perhaps we'll also support C++26 reflection in the future
define_property(TARGET PROPERTY MAGIC_ARGS_HAVE_ENUM_SUPPORT)
find_package(magic_enum CONFIG)
set_property(TARGET magic_args PROPERTY MAGIC_ARGS_HAVE_ENUM_SUPPORT "${magic_enum_FOUND}")
if (magic_enum_FOUND)
  message(STATUS "Found magic_enum, adding enum support")
  target_link_libraries(magic_args INTERFACE magic_enum::magic_enum)
else () # magic_enum_FOUND
  message(STATUS "Did not find magic_enum library, skipping enum support")
endif ()

target_sources(
  magic_args
  INTERFACE
  FILE_SET HEADERS
  TYPE HEADERS
  FILES ${HEADER_FILES}
)
target_include_directories(
  magic_args
  INTERFACE
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
  "$<INSTALL_INTERFACE:include>"
)
target_compile_features(
  magic_args
  INTERFACE
  cxx_std_23
)

set(SINGLE_HEADER_SOURCES "${HEADER_FILES}")
list(PREPEND SINGLE_HEADER_SOURCES detail/single_header_prefix.hpp)
set(SINGLE_HEADER_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/single-header/magic_args")
set(SINGLE_HEADER_OUTPUT_FILE "${SINGLE_HEADER_OUTPUT_DIRECTORY}/magic_args.hpp")

add_custom_command(
  OUTPUT "${SINGLE_HEADER_OUTPUT_FILE}"
  COMMAND
  "${CMAKE_COMMAND}" -E make_directory "${SINGLE_HEADER_OUTPUT_DIRECTORY}"
  COMMAND
  "${CMAKE_COMMAND}" -E cat "${SINGLE_HEADER_SOURCES}" > "${SINGLE_HEADER_OUTPUT_FILE}"
  DEPENDS
  "${SINGLE_HEADER_SOURCES}"
  "${CMAKE_CURRENT_LIST_FILE}"
  WORKING_DIRECTORY
  "${CMAKE_CURRENT_SOURCE_DIR}"
  VERBATIM
  COMMAND_EXPAND_LISTS
)
add_library(
  magic_args-single-header
  INTERFACE
  "${SINGLE_HEADER_OUTPUT_FILE}"
)
target_include_directories(
  magic_args-single-header
  INTERFACE
  "${SINGLE_HEADER_OUTPUT_DIRECTORY}/.."
)
target_compile_features(
  magic_args-single-header
  INTERFACE
  cxx_std_23
)

install(
  TARGETS magic_args
  EXPORT exports
  FILE_SET HEADERS
  DESTINATION include/magic_args
)
install(
  EXPORT
  exports
  NAMESPACE magic_args::
  FILE magic_args-targets.cmake
  DESTINATION lib/cmake/magic_args
)
install(
  FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/magic_args-config.cmake"
  DESTINATION lib/cmake/magic_args
)

install(
  FILES "${SINGLE_HEADER_OUTPUT_FILE}"
  DESTINATION "include/magic_args"
  COMPONENT SingleHeader
  EXCLUDE_FROM_ALL
)

# For projects using add_subdirectory()
add_library(magic_args::magic_args ALIAS magic_args)