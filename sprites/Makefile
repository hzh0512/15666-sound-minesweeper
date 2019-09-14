
OS := $(shell uname)

ifeq ($(OS), Linux)
	GIMP=gimp
else
	GIMP=/Applications/GIMP.app/Contents/MacOS/gimp
endif


../dist/sound-minesweeper.png ../dist/sound-minesweeper.atlas : sound-minesweeper.xcf sound-minesweeper.list trade.xcf trade-font.list extract-sprites.py pack-sprites
	rm -rf sound-minesweeper
	./extract-sprites.py sound-minesweeper.list sound-minesweeper --gimp='$(GIMP)'
	./extract-sprites.py trade-font.list sound-minesweeper --gimp='$(GIMP)'
	./pack-sprites ../dist/sound-minesweeper sound-minesweeper/*
