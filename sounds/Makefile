
#from https://stackoverflow.com/questions/714100/os-detecting-makefile
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	OPUSENC = ../../nest-libs/linux/opus-tools/bin/opusenc
else
	OPUSENC = ../../nest-libs/macos/opus-tools/bin/opusenc
endif
../dist/sand.opus : sand.wav
	$(OPUSENC) --vbr --bitrate 128 sand.wav ../dist/sand.opus
