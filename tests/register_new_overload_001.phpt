--TEST--
register_new_overload() works
--SKIPIF--
<?php 
if (!extension_loaded('test_helpers')) die('test_helpers extension not loaded');
?>
--FILE--
<?php
class Foo {}

function callback($className) {
    return new Foo;
}

register_new_overload('callback');

var_dump(get_class(new Bar));
--EXPECT--
string(3) "Foo"
