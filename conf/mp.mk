# MP-specific Makefile configurations
UPROGS += \
	$U/_bigfile \
	$U/_symlinkfile \
	$U/_symlinkdir \
	$U/_symlinkcycle

CFLAGS += -DMP4
