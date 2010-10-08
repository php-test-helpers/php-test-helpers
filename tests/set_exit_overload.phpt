--TEST--
set_exit_overload()
--SKIPIF--
<?php
if (!extension_loaded('test_helpers')) die('skip test_helpers extension not loaded');
?>
--FILE--
<?php
set_exit_overload(function($arg = NULL) { var_dump($arg); echo "FALSE\n"; return false; });
die("DIE 1");
die;
exit;
set_exit_overload(function($arg) { var_dump($arg); echo "TRUE\n"; return true; });
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
