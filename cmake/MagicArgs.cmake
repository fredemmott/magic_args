include_guard(GLOBAL)

function(magic_args_enumerate_subcommands TARGET)
  set(options "")
  set(oneValueArgs SYMLINKS_DIR HARDLINKS_DIR TEXT_FILE STAMP_FILE)
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
    "--stamp-file=${ARG_STAMP_FILE}"
    "--output-style=quiet"
    "$<TARGET_FILE:${TARGET}>"
    VERBATIM
  )
endfunction()

function(magic_args_install_multicall_links TARGET)
  set(options RELATIVE_SYMLINKS)
  set(oneValueArgs DESTINATION SYMLINKS_DESTINATION HARDLINKS_DESTINATION STAMP_FILE COMPONENT)
  set(multiValueArgs "")
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if (ARG_COMPONENT)
    set(INSTALL_COMPONENT "COMPONENT \"${ARG_COMPONENT}\"")
  else ()
    set(INSTALL_COMPONENT)
  endif ()
  if (ARG_DESTINATION)
    set(DESTINATION "${ARG_DESTINATION}")
  else ()
    set(DESTINATION "bin")
  endif ()

  set(RELATIVE_SYMLINKS)
  if (ARG_RELATIVE_SYMLINKS)
    set(RELATIVE_SYMLINKS "--relative-symlinks")
  endif ()

  install(CODE "
set(ROOT \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}\")
set(HARDLINKS \"${ARG_HARDLINKS_DESTINATION}\")
set(SYMLINKS \"${ARG_SYMLINKS_DESTINATION}\")
set(STAMP_FILE \"${ARG_STAMP_FILE}\")
set(TARGET_EXECUTABLE \"\${ROOT}/${DESTINATION}/$<TARGET_FILE_NAME:${TARGET}>\")

if (NOT EXISTS \"\${TARGET_EXECUTABLE}\")
  message(FATAL_ERROR \"\${CMAKE_CURRENT_FUNCTION}: can't create symlinks for '\${TARGET_EXECUTABLE}' because it does not exist; is it installed?\")
endif ()

if (NOT \"\${HARDLINKS}\" STREQUAL \"\" AND NOT IS_ABSOLUTE \"\${HARDLINKS}\")
  set(HARDLINKS \"\${ROOT}/\${HARDLINKS}\")
endif ()
if (NOT \"\${SYMLINKS}\" STREQUAL \"\" AND NOT IS_ABSOLUTE \"\${SYMLINKS}\")
  set(SYMLINKS \"\${ROOT}/\${SYMLINKS}\")
endif ()
if (NOT \"\${STAMP_FILE}\" STREQUAL \"\" AND NOT IS_ABSOLUTE \"\${STAMP_FILE}\")
  set(STAMP_FILE \"\${ROOT}/\${STAMP_FILE}\")
endif ()

execute_process(
  COMMAND
  \"$<TARGET_FILE:magic_args::enumerate-subcommands>\"
  \"--hardlinks=\${HARDLINKS}\"
  \"--symlinks=\${SYMLINKS}\"
  \"--stamp-file=\${STAMP_FILE}\"
  --output-style=cmake-install
  ${RELATIVE_SYMLINKS}
  \"\${TARGET_EXECUTABLE}\"
  COMMAND_ERROR_IS_FATAL ANY
)"
    ${INSTALL_COMPONENT}
  )
endfunction()