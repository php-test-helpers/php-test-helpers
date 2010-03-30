--TEST--
A functor can be registered as a callback with set_new_overload()
--SKIPIF--
<?php
if (!extension_loaded('test_helpers')) die('skip test_helpers extension not loaded');
if (version_compare(PHP_VERSION, '5.3.0') < 0) die('skip Test requires PHP 5.3');
?>
--FILE--
<?php
class Foo {}
class Bar {}

class Callback
{
    public function __invoke($className)
    {
        return 'Foo';
    }
}

var_dump(set_new_overload(new Callback));

var_dump(get_class(new Bar));
--EXPECT--
bool(true)
string(3) "Foo"
