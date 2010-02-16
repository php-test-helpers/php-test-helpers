--TEST--
debug_print_backtrace() should work inside the callback.
--SKIPIF--
<?php 
if (!extension_loaded('test_helpers')) die('skip test_helpers extension not loaded');
?>
--FILE--
<?php
class Foo {}
class Bar {}

function callback($className) {
    debug_print_backtrace();
    return 'Foo';
}

function getObject() {
    return new Bar();
}

var_dump(set_new_overload('callback'));
var_dump(getObject());
--EXPECTF--
bool(true)
#0  callback(Bar) called at [%s:%d]
#1  getObject() called at [%s:%d]
object(Foo)#1 (0) {
}
