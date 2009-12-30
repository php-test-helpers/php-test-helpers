--TEST--
register_new_overload() overloading a non-existing class
--SKIPIF--
<?php 
if (!extension_loaded('test_helpers')) die('skip test_helpers extension not loaded');
?>
--XFAIL--
currently not supported by the implementation but nice to have
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
