add_library(
  magic_args
  INTERFACE
  # DO NOT SORT THESE
  #
  # These are in the order required for the single-header version
  detail/concepts.hpp
  detail/reflection.hpp
  gnu_style_parsing_traits.hpp
  powershell_style_parsing_traits.hpp
  detail/print.hpp
  argument_definitions.hpp
  detail/get_argument_definition.hpp
  detail/validation.hpp
  program_info.hpp
  detail/usage.hpp
  incomplete_parse_reason.hpp
  detail/parse.hpp
  dump.hpp
  parse.hpp
  verbatim_names.hpp
  magic_args.hpp
  windows.hpp
)
target_include_directories(
  magic_args
  INTERFACE
  "${CMAKE_CURRENT_LIST_DIR}/.."
)
target_compile_features(
  magic_args
  INTERFACE
  cxx_std_23
)

get_target_property(HEADER_FILES magic_args SOURCES)
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

foreach (HEADER IN LISTS HEADER_FILES)
  cmake_path(GET HEADER PARENT_PATH DIRECTORY)
  install(FILES "${HEADER}" DESTINATION "include/magic_args/${DIRECTORY}")
endforeach ()

install(
  FILES "${SINGLE_HEADER_OUTPUT_FILE}"
  DESTINATION "include/magic_args"
  COMPONENT SingleHeader
  EXCLUDE_FROM_ALL
)