--TEST--
rename_function() and internal functions
--SKIPIF--
<?php 
if (!extension_loaded('test_helpers')) die('skip test_helpers extension not loaded');
?>
--XFAIL--
Not yet implemented
--FILE--
<?php
$headers = array();

function my_header($header)
{
    $GLOBALS['headers'][] = $header;
}

rename_function('header', 'internal_header');
rename_function('my_header', 'header');
header('Location: http://www.example.com/');
rename_function('header', 'my_header');
rename_function('internal_header', 'header');
var_dump($headers);
--EXPECT--
array(1) {
  [0]=>
  string(33) "Location: http://www.example.com/"
}
