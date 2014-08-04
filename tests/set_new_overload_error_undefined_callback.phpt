--TEST--
A warning is triggered when the callback passed to set_new_overload() is not defined
--SKIPIF--
<?php
if (!extension_loaded('test_helpers')) die('skip test_helpers extension not loaded');
?>
--FILE--
<?php
class Bar {}

var_dump(set_new_overload('callback'));

var_dump(get_class(new Bar));
--EXPECTF--
Warning: set_new_overload() expects parameter 1 to be %s
NULL
string(3) "Bar"
