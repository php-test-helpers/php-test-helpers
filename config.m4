PHP_ARG_ENABLE(test_helpers, whether to enable test_helpers support,
[  --enable-test-helpers   Enable test_helpers support])

if test "$PHP_TEST_HELPERS" != "no"; then
  PHP_NEW_EXTENSION(test_helpers, test_helpers.c, $ext_shared)
fi
