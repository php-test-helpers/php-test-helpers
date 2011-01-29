--TEST--
rename_method() and user-defined functions
--SKIPIF--
<?php 
if (!extension_loaded('test_helpers')) die('skip test_helpers extension not loaded');
?>
--FILE--
<?php
class A {
    static function foo()
    {
        print 'foo';
    }
}

rename_method('A', 'foo', 'bar');
A::bar();
--EXPECT--
foo
