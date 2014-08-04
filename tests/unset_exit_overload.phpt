--TEST--
set_exit_overload()
--SKIPIF--
<?php
if (version_compare(PHP_VERSION, '5.3', '<')) die("skip this test is for PHP 5.3+.");
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
