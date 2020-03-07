<?php

$command = escapeshellcmd('python3 /var/www/html/test.py');
$output = shell_exec($command);
echo $output;

// $test = 3;
// echo $test;

?>
