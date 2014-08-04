--TEST--
set_exit_overload() with a named function
--SKIPIF--
<?php
if (!extension_loaded('test_helpers')) die('skip test_helpers extension not loaded');
?>
--FILE--
<?php
function _exit1($arg = NULL) { var_dump($arg); echo "FALSE\n"; return false; }
set_exit_overload('_exit1');
die("DIE 1");
die;
exit;
function _exit2($arg = NULL) { var_dump($arg); echo "TRUE\n"; return true; }
set_exit_overload('_exit2');
die("DIE 4");
echo "HAHA";
?>
--EXPECT--
string(5) "DIE 1"
FALSE
NULL
FALSE
NULL
FALSE
string(5) "DIE 4"
TRUE
DIE 4
