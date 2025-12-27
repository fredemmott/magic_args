  add_test(NAME test-enumerate-subcommands
    COMMAND enumerate-subcommands $<TARGET_FILE:example-multicall>
  )
  set_tests_properties(test-enumerate-subcommands PROPERTIES
    PASS_REGULAR_EXPRESSION "foo-bar\nherp\n"
  )

  add_test(NAME test-enumerate-no-subcommands
    COMMAND enumerate-subcommands $<TARGET_FILE:example-minimal>
  )
  set_tests_properties(
    test-enumerate-no-subcommands
    PROPERTIES
    PASS_REGULAR_EXPRESSION "^[[:space:]]*$"
    # Non-zero exit code is expected
    WILL_FAIL true
  )
