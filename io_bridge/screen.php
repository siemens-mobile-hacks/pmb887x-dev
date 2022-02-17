<?php
$data = file_get_contents("/tmp/ram.bin");

// echo substr($data, 0xa8da8080 - 0xA8000000, 240*320*2);
echo substr($data, 0xa8da8080 - 0xA8000000, 240*320*3);
