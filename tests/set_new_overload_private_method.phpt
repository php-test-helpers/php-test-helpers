--TEST--
A private method can be registered from the right context with set_new_overload()
--SKIPIF--
<?php
if (!extension_loaded('test_helpers')) die('skip test_helpers extension not loaded');
?>
--FILE--
<?php
class Foo {}
class Bar {}

class CB {
    private function callback($className) {
        if ($className == 'Bar') {
            return 'Foo';
        } else {
            return $className;
        }
    }

    public function set_overload() {
        return set_new_overload(array($this, 'callback'));
    }
}

$cb = new CB();

var_dump(set_new_overload(array($cb, 'callback')));
var_dump(get_class(new Bar));

var_dump($cb->set_overload());
var_dump(get_class(new Bar));
--EXPECTF--
Warning: set_new_overload() expects parameter 1 to be %s
NULL
string(3) "Bar"
bool(true)
string(3) "Foo"
