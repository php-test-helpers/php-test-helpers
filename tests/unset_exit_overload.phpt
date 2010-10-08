--TEST--
set_exit_overload()
--FILE--
<?php
set_exit_overload(function() { echo "FALSE\n"; return false; });
unset_exit_overload();
die("DIE");
echo "HAHA";
?>
--EXPECT--
DIE
