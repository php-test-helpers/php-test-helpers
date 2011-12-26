# ext/test_helpers #

`ext/test_helpers` is an extension for the PHP Interpreter to ease testing of PHP code.

## Installation ##

`ext/test_helpers` should be installed using the [PEAR Installer](http://pear.php.net/). This installer is the backbone of PEAR and PECL, which provides a distribution system for PHP packages and extensions, and is shipped with every release of PHP since version 4.3.0.

The PEAR channel (`pear.phpunit.de`) that is used to distribute `ext/test_helpers` needs to be registered with the local PEAR environment:

    sb@ubuntu ~ % pear channel-discover pear.phpunit.de
    Adding Channel "pear.phpunit.de" succeeded
    Discovery of channel "pear.phpunit.de" succeeded

This has to be done only once. Now the PEAR Installer can be used to install extensions and packages from the PHPUnit channel:

    sb@ubuntu ~ % pecl install phpunit/test_helpers
    downloading test_helpers-1.0.0.tgz ...
    Starting to download test_helpers-1.0.0.tgz (6,980 bytes)
    .....done: 6,980 bytes
    4 source files, building
    .
    .
    .
    install ok: channel://pear.phpunit.de/test_helpers-1.0.0
    You should add "extension=test_helpers.so" to php.ini

Further information about building stand-alone extensions for PHP can be found in the [Installation of PECL extensions](http://php.net/install.pecl) section of the PHP manual.

## Usage ##

### Intercepting the Exit Statement ###

When a [unit test](http://en.wikipedia.org/wiki/Unit_test) exercises code that contains an `exit` / `die` statement, the execution of the whole test suite is aborted. This is not a good thing.

With the `set_exit_overload()` function it is possible to overload the `exit` / `die` statement and make it a no-op, for instance:

    <?php
    set_exit_overload(function() { return FALSE; });
    exit;
    print 'We did not exit.';
    unset_exit_overload();
    exit;
    print 'We exited and this will not be printed.';
    ?>

The code above will output

    We did not exit.

The callback registered by `set_exit_overload()` might receive a parameter in case `exit` / `die` was called with a parameter:

    <?php
    set_exit_overload(function($param = NULL) { echo ($param ?: "No value given"), "\n"; return FALSE; }
    die("Hello");
    die;
    ?>

The code above will output

    Hello
    No value given

Another way of dealing with low-level functions and statements such as `die()` and `exit` is to wrap them in a proxy that by default (in production) delegates to the native implementation but for testing has a "testable" behaviour.

### Intercepting Object Creation ###

In a [unit test](http://en.wikipedia.org/wiki/Unit_test), [mock objects](http://en.wikipedia.org/wiki/Mock_Object) can simulate the behavior of complex, real (non-mock) objects and are therefore useful when a real object is difficult or impossible to incorporate into a unit test.

A mock object can be used anywhere in the program where the program expects an object of the mocked class. However, this only works as long as the object can be passed into the context where the original object is used.

Consider the following example:

    <?php
    class SomeClass
    {
        public function doSomething()
        {
            $object = new SomeOtherClass;
            // ...
        }
    }
    ?>

With the code above, it is impossible to run a unit test for the `SomeClass::doSomething()` method without also creating an object of `SomeOtherClass`. As the method creates the object of `SomeOtherClass` itself, we cannot inject a mock object in its stead.

In a perfect world, code such as the above could be refactored using [Dependency Injection](http://en.wikipedia.org/wiki/Dependency_Injection):

    <?php
    class SomeClass
    {
        protected $object;

        public function __construct(SomeOtherClass $object)
        {
            $this->object = $object;
        }

        public function doSomething()
        {
            // ...
        }
    }
    ?>

Unfortunately, this is not always possible (not because of technical reasons, though).

This is where the `set_new_overload()` function comes into play. It can be used to register a [callback](http://www.php.net/manual/en/language.pseudo-types.php) that is automatically invoked when the `new` operator is executed:

    <?php
    class Foo {}
    class Bar {}

    function callback($className) {
        if ($className == 'Foo') {
            $className = 'Bar';
        }

        return $className;
    }

    var_dump(get_class(new Foo));

    set_new_overload('callback');
    var_dump(get_class(new Foo));
    ?>

    string(3) "Foo"
    string(3) "Bar"

The `new` operator callback can be unset when it is no longer required:

    <?php
    class Foo {}
    class Bar {}

    function callback($className) {
        return 'Bar';
    }

    set_new_overload('callback');
    var_dump(get_class(new Foo));

    unset_new_overload();
    var_dump(get_class(new Foo));
    ?>

    string(3) "Bar"
    string(3) "Foo"

#### Class Posing ####

The `set_new_overload()` function can be used to implement a programming language feature named *Class Posing*. [The implementation of Class Posing in Objective-C](http://en.wikipedia.org/wiki/Objective-C#Posing), for instance, permits a class to wholly replace another class within a program. The replacing class is said to "pose as" the target class.

Class Posing has the following restrictions

* A class may only pose as one of its direct or indirect superclasses
* The posing class must not define any new instance variables which are absent from the target class (though it may define or override methods).
* The target class may not have received any messages prior to the posing.

These restrictions are not enforced by `ext/test_helpers` because the extension is only intended to ease the development of unit tests (for legacy software systems that cannot be refactored to use Dependency Injection).

### Renaming Functions ###

The `rename_function()` function can be used to rename function:

    <?php
    function foo()
    {
        // ...
    }

    function foo_stub()
    {
        return 'stubbed result';
    }

    rename_function('foo', 'foo_orig');
    rename_function('foo_stub', 'foo');
    var_dump(foo());
    rename_function('foo', 'foo_stub');
    rename_function('foo_orig', 'foo');
    ?>

    string(14) "stubbed result"

This allows the stubbing/mocking of functions.

## Notes ##

If this extension is used in combination with other extensions, such as Xdebug, which are also overloading the `ZEND_NEW` opcode you have to load it as `zend_extension` after loading the conflicting extension. This can be done in your `php.ini` like this:

    zend_extension=xdebug.so
    zend_extension=test-helpers.so

Please refer to `phpinfo()` to verify whether a conflict was detected and whether the work-around was enabled.

