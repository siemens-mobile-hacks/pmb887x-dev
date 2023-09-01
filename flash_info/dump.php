<?php
$data = file_get_contents("app.bin");

for ($i = 0; $i < strlen($data); $i++) {
	echo "0x".bin2hex($data[$i]).", ";
}
echo "\n";
