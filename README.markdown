# ext/test_helpers #

`ext/test_helpers` is an extension for the PHP Interpreter to ease testing of PHP code.

## Installation ##

Clone the `ext/test_helpers` repository and build the extension:

    $ git clone git://github.com/sebastianbergmann/php-test-helpers.git
    $ cd php-test-helpers
    $ phpize
    $ ./configure
    $ make
    $ make install

Add `ext/test_helpers` to your `php.ini` configuration file:

    extension=test_helpers.so

Further information about building stand-alone extensions for PHP can be found in the [Installation of PECL extensions](http://php.net/install.pecl) section of the PHP manual.

## Usage ##

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

With the code above, it is impossible to run a unit test for the `SomeClass::doSomething()` method that does involve creating an object of `SomeOtherClass`. As the method creates the object of `SomeOtherClass` itself, we cannot inject a mock object in its stead.

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

These restrictions are not enforced by `ext/test_helpers` because the extension is only intended to ease the development of unit tests (for legacy software systems than cannot be refactored to use Dependency Injection).
