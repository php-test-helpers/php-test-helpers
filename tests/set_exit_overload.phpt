--TEST--
set_exit_overload()
--FILE--
<?php
set_exit_overload(function() { echo "FALSE\n"; return false; });
die("DIE 1");
set_exit_overload(function() { echo "TRUE\n"; return true; });
die("DIE 2");
echo "HAHA";
?>
--EXPECT--
FALSE
TRUE
DIE 2
