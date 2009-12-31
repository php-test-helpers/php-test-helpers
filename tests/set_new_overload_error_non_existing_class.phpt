--TEST--
Callback for the new operator is invoked for non-existing classes
--SKIPIF--
<?php 
if (!extension_loaded('test_helpers')) die('skip test_helpers extension not loaded');
?>
--XFAIL--
Currently not supported by the implementation but nice to have
--FILE--
<?php
class Foo {}

function callback($className) {
    return 'Foo';
}

set_new_overload('callback');

var_dump(get_class(new Bar));
--EXPECT--
string(3) "Foo"
