--TEST--
A non-static method of the same object can be registered as a callback with set_new_overload()
--SKIPIF--
<?php
if (!extension_loaded('test_helpers')) die('skip test_helpers extension not loaded');
?>
--FILE--
<?php
class Foo
{
    public function doSomething()
    {
        var_dump(__METHOD__);

        // ...

        $bar = new Bar;
        $bar->doSomethingElse();

        // ...
    }
}

class Bar
{
    public function doSomethingElse()
    {
        var_dump(__METHOD__);
    }
}

class BarMock extends Bar
{
    public function doSomethingElse()
    {
        var_dump(__METHOD__);
    }
}

class FooTest
{
    public function setUp()
    {
        var_dump(__METHOD__);

        set_new_overload(array($this, 'newCallback'));
    }

    public function tearDown()
    {
        var_dump(__METHOD__);

        unset_new_overload();
    }

    protected function newCallback($className)
    {
        var_dump(__METHOD__);

        switch ($className) {
            case 'Bar': return 'BarMock';
            default:    return $className;
        }
    }

    public function testDoSomething()
    {
        var_dump(__METHOD__);

        $foo = new Foo;
        $foo->doSomething();
    }
}

$test = new FooTest;
$test->setUp();
$test->testDoSomething();
$test->tearDown();
--EXPECT--
string(14) "FooTest::setUp"
string(24) "FooTest::testDoSomething"
string(20) "FooTest::newCallback"
string(16) "Foo::doSomething"
string(20) "FooTest::newCallback"
string(24) "BarMock::doSomethingElse"
string(17) "FooTest::tearDown"
