--TEST--
set_new_overload() withan object method
--SKIPIF--
<?php 
if (!extension_loaded('test_helpers')) die('skip test_helpers extension not loaded');
?>
--FILE--
<?php
class Foo {}
class Bar {}

class CB {
    function callback($className) {
        if ($className == 'Bar') {
            return 'Foo';
        } else {
            return $classiName;
        }
    }
}

var_dump(set_new_overload(array(new CB(), 'callback')));

var_dump(get_class(new Bar));
--EXPECT--
bool(true)
string(3) "Foo"
