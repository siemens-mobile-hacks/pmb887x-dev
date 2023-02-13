<?php
$img = imagecreatetruecolor(240, 320);

for ($x = 0; $x < 240; $x++) {
	for ($y = 0; $y < 320; $y++) {
		write_pixel($img, $x, $y);
	}
}

imagepng($img, "/tmp/test.jpg");

function incr() {
	
}

function translate_xy($img, $x, $y) {
	$id0 = 0;
	$id1 = 1;
	$am = 0;
	
	if ($id0 == 0 && $id1 == 0) {
		$x = imagesx($img) - $x - 1;
		$y = imagesy($img) - $y - 1;
	} else if ($id0 == 1 && $id1 == 0) {
		$y = imagesy($img) - $y - 1;
	} else if ($id0 == 0 && $id1 == 1) {
		$x = imagesx($img) - $x - 1;
	}
	
	return [$x, $y];
}

function write_pixel($img, $x, $y) {
	$l = 10 + floor($x / imagesx($img) * 80);
	$s = floor($y / imagesy($img) * 100);
	
	list ($x, $y) = translate_xy($img, $x, $y);
	
	list ($r, $g, $b) = hsl2rgb(0, $s, $l);
	
	imageline($img, $x, $y, $x, $y, $b | ($g << 8) | ($r << 16));
}

function hsl2rgb($h, $s, $l) {
	$c = ((1 - abs(2 * ($l / 100) - 1)) * $s) / 100;
	$x = $c * (1 - abs(fmod($h / 60, 2) - 1));
	$m = $l / 100 - $c / 2;
	if ($h < 60) {
		$r = $c;
		$g = $x;
		$b = 0;
	} elseif ($h < 120) {
		$r = $x;
		$g = $c;
		$b = 0;
	} elseif ($h < 180) {
		$r = 0;
		$g = $c;
		$b = $x;
	} elseif ($h < 240) {
		$r = 0;
		$g = $x;
		$b = $c;
	} elseif ($h < 300) {
		$r = $x;
		$g = 0;
		$b = $c;
	} else {
		$r = $c;
		$g = 0;
		$b = $x;
	}
	return [
		floor(($r + $m) * 255),
		floor(($g + $m) * 255),
		floor(($b + $m) * 255),
	];
}
