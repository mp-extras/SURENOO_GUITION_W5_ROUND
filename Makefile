# Manifest and user C modules are specified in mpconfigboard.cmake
ARGS = -C $(MICROPYTHON)/ports/esp32 BOARD_DIR=$(PWD) PORT=/dev/ttyACM0

all deploy clean erase:
	$(MAKE) $(ARGS) $@
