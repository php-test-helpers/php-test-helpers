--TEST--
rename_function() and user-defined functions
--SKIPIF--
<?php 
if (!extension_loaded('test_helpers')) die('skip test_helpers extension not loaded');
?>
--FILE--
<?php
function foo()
{
    print 'foo';
}

rename_function('foo', 'bar');
bar();
--EXPECT--
foo
