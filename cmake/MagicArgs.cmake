include_guard(GLOBAL)

define_property(TARGET PROPERTY MAGIC_ARGS_SUBCOMMANDS_LIST_TEXT_FILE)
function(magic_args_enumerate_subcommands TARGET)
  set(options "")
  set(oneValueArgs SYMLINKS_DIR HARDLINKS_DIR TEXT_FILE)
  set(multiValueArgs "")
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  add_custom_command(
    TARGET "${TARGET}"
    POST_BUILD
    COMMAND
    magic_args::enumerate-subcommands
    "--text-file=${ARG_TEXT_FILE}"
    "--symlinks=${ARG_SYMLINKS_DIR}"
    "--hardlinks=${ARG_HARDLINKS_DIR}"
    "--quiet"
    "$<TARGET_FILE:${TARGET}>"
    VERBATIM
  )
  if (ARG_TEXT_FILE)
    set_target_properties("${TARGET}" PROPERTIES MAGIC_ARGS_SUBCOMMANDS_LIST_TEXT_FILE "${ARG_TEXT_FILE}")
  endif ()
endfunction()
