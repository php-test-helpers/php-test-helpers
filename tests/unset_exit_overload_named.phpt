--TEST--
set_exit_overload() with a named function
--SKIPIF--
<?php
if (!extension_loaded('test_helpers')) die('skip test_helpers extension not loaded');
?>
--FILE--
<?php
function _exit() { echo "FALSE\n"; return false; }
set_exit_overload('_exit');
unset_exit_overload();
die("DIE");
echo "HAHA";
?>
--EXPECT--
DIE
