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

This is where the `set_new_overload()` function comes into play. It can be used to register a [callback](http://www.php.net/manual/en/language.pseudo-types.php) that is automatically invoked when the `new` operator is executed.

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

If this is needed just temporarily the handler can also be unregistered.

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

