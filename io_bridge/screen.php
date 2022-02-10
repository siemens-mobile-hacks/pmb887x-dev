<?php
$data = file_get_contents("/tmp/ram.bin");

echo substr($data, 0xA8DF3130 - 0xA8000000 + 240*320*2, 240*320*2);
