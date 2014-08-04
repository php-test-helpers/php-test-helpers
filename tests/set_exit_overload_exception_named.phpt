--TEST--
set_exit_overload() with an uncaught exception with a named function
--SKIPIF--
<?php
if (!extension_loaded('test_helpers')) die('skip test_helpers extension not loaded');
?>
--FILE--
<?php
function _exit($arg = NULL) { throw new Exception("Please don't segfault"); }
set_exit_overload('_exit');
try {
    exit("hi");
} catch(Exception $exception) {
    echo $exception->getMessage();
}
?>
--EXPECT--
Please don't segfault