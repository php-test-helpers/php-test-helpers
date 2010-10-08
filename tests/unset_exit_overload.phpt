--TEST--
set_exit_overload()
--SKIPIF--
<?php
if (!extension_loaded('test_helpers')) die('skip test_helpers extension not loaded');
?>
--FILE--
<?php
set_exit_overload(function() { echo "FALSE\n"; return false; });
unset_exit_overload();
die("DIE");
echo "HAHA";
?>
--EXPECT--
DIE
