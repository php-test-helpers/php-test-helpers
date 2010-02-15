--TEST--
set_new_overload() can be called multiple times and unset_new_overload() works
--SKIPIF--
<?php
if (!extension_loaded('test_helpers')) die('skip test_helpers extension not loaded');
?>
--FILE--
<?php
class Foo1 {}
class Foo2 {}
class Bar {}

function callback1($className) {
    return 'Foo1';
}

function callback2($className) {
    return 'foo2';
}

var_dump(set_new_overload('callback1'));
var_dump(get_class(new Bar));
var_dump(set_new_overload('callback2'));
var_dump(get_class(new Bar));
var_dump(unset_new_overload());
var_dump(get_class(new Bar));
--EXPECT--
bool(true)
string(4) "Foo1"
bool(true)
string(4) "Foo2"
bool(true)
string(3) "Bar"
