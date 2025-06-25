set(IDF_TARGET esp32s3)

set(MICROPY_FROZEN_MANIFEST ${MICROPY_BOARD_DIR}/manifest.py)
set(USER_C_MODULES ${MICROPY_BOARD_DIR}/usercmodules.cmake)

set(SDKCONFIG_DEFAULTS
    boards/sdkconfig.base
    boards/sdkconfig.usb
    boards/sdkconfig.ble
    ${MICROPY_BOARD_DIR}/sdkconfig.board
)
