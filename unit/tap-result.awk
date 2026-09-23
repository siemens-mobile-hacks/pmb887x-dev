{
	print
	fflush()

	line = $0
	gsub(/\033\[[0-9;]*m/, "", line)
	if (line != "")
		last = line
}

END {
	if (last != "# result: PASS (0 failed)") {
		print "Unit test did not finish with PASS (0 failed)" > "/dev/stderr"
		exit 1
	}
}
