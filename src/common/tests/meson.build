# Core unit tests
# Compile util.c directly (it's nearly self-contained).
# For url.c tests, we provide minimal stubs for symbols it references
# (tree, fe, prefs, hexchat_fopen_file) that are not needed by
# url_check_word().

core_test_sources = [
  'test_util.c',
  'test_url.c',
  'test_main.c',
  'stubs.c',
  '../util.c',
  '../url.c',
  '../tree.c',
]

core_tests = executable('core_tests', core_test_sources,
  include_directories: common_includes,
  dependencies: [libgio_dep, libssl_dep],
  c_args: common_cflags,
)

test('Core Tests', core_tests,
  protocol: 'tap',
  timeout: 60,
)
