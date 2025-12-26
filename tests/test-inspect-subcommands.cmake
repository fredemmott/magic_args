  add_test(NAME test-inspect-subcommands
    COMMAND list-subcommands $<TARGET_FILE:example-multicall>
  )
  set_tests_properties(test-inspect-subcommands PROPERTIES
    PASS_REGULAR_EXPRESSION "foo-bar\nherp\n"
  )

  add_test(NAME test-inspect-no-subcommands
    COMMAND list-subcommands $<TARGET_FILE:example-minimal>
  )
  set_tests_properties(
    test-inspect-no-subcommands
    PROPERTIES
    PASS_REGULAR_EXPRESSION "^[[:space:]]*$"
    # Non-zero exit code is expected
    WILL_FAIL true
  )
