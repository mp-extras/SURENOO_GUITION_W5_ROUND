#
# Copyright (c) 2023 Mark Grosen <mark@grosen.org>
# SPDX-License-Identifier: MIT
#

"""
cst8xx

Configure and interact with CST8xx touchscreen controller via I2C
"""

from time import sleep
from micropython import const

WORK_MODE_REG = const(0x00)

TOUCH_NUM_REG = const(0x02)

TOUCH1_XH_REG = const(0x03)
TOUCH1_XL_REG = const(0x04)
TOUCH1_YH_REG = const(0x05)
TOUCH1_YL_REG = const(0x06)

TOUCH2_XH_REG = const(0x09)
TOUCH2_XL_REG = const(0x0a)
TOUCH2_YH_REG = const(0x0b)
TOUCH2_YL_REG = const(0x0c)

GES_STATE_REG = const(0xD0)
GES_ID_REG = const(0xD3)


class CST8XX:
    """
    Create a CST8XX object to interact with touchscreen

    :param i2c: I2C object
    :param addr: address on I2C bus
    :param reset: Pin OUT object for reset
    :param intr: Pin IN object for interrupts from CST8xx

    Example:

    .. code-block:: python

        from machine import Pin, I2C

        from cst8xx import CST8XX

        ts = CST8XX(I2C(0), 0x15, Pin(RESET, Pin.OUT), Pin(INTR, Pin.IN))

        if ts.touched:
            print(ts.points)

    """
    def __init__(self, i2c, addr=0x15, reset=None, intr=None):
        self._i2c = i2c
        self._addr = addr
        self._raw = bytearray(13)
        self._iob = bytearray(1)
        self._gestures = False
        self._touched = True

        if reset:
            reset(1)
            reset(0)
            sleep(0.2)
            reset(1)
            sleep(0.3)

        if intr:
            def t_cb(_):
                self._touched = True

            intr.irq(t_cb)

    def _read(self):
        try:
            self._i2c.readfrom_mem_into(self._addr, WORK_MODE_REG, self._raw)
            return True
        except IOError:
            return False

    @property
    def points(self):
        """
        Read the most recent touchscreen event

        :return: None if not touched, else tuple: (x, y, state)
        """
        result = None
        if self._read():
            raw = self._raw
            np = raw[TOUCH_NUM_REG] & 0x0f
            if np == 1:
                state = raw[TOUCH1_XH_REG] >> 6
                x = (raw[TOUCH1_XH_REG] & 0x0f) << 8 | raw[TOUCH1_XL_REG]
                y = (raw[TOUCH1_YH_REG] & 0x0f) << 8 | raw[TOUCH1_YL_REG]
                result =(x, y, state)

        return result

    @property
    def gesture(self):
        """NOTE: does not work on CST816"""
        # if not self._gestures:
        #     self._iob[0] = 1
        #     self._i2c.writeto_mem(self._addr, GES_STATE_REG, self._iob)
        #     self._gestures = True

        self._i2c.readfrom_mem_into(self._addr, 0x01, self._iob)
        return int(self._iob[0])

    @property
    def touched(self):
        """
        Has the touchscreen been touched

        :return: bool True if touched, else False
        """
        v = self._touched
        self._touched = False
        return v
