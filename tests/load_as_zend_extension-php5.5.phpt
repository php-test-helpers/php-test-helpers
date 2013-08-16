--TEST--
Loading test_helpers as zend_extension
--INI--
zend_extension=test_helpers.so
error_log=
display_errors=0
--SKIPIF--
<?php
if (version_compare(PHP_VERSION, '5.5', '<')) die("skip this test is for PHP 5.5+.");
if (!extension_loaded('test_helpers')) die('skip test_helpers extension not loaded');
if (!file_exists('modules/test_helpers.so')) die('skip test_helpers.so not found Static build? out-of-src-dir build?');
?>
--FILE--
<?php
// The test framework will load the extension as PHP extension, we verify that a second load attempt is made
// This test has some flaws, the major one is that it expects a specific .so file at a specific location ...
echo "done";
?>
--EXPECT--
PHP Warning:  Module 'test_helpers' already loaded in Unknown on line 0
done
