--TEST--
set_exit_overload() with an uncaught exception
--SKIPIF--
<?php
if (!extension_loaded('test_helpers')) die('skip test_helpers extension not loaded');
?>
--FILE--
<?php
set_exit_overload(function($arg = NULL) { throw new Exception("Please don't segfault"); });
try {
    exit("hi");
} catch(Exception $exception) {
    echo $exception->getMessage();
}
?>
--EXPECT--
Please don't segfault