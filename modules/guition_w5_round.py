from machine import I2C, Pin, PWM, I2S, SDCard
from vfs import mount

from st77916 import ST77916
from cst8xx import CST8XX
from qspi import QSPI

_backlight = None


def backlight(value: int) -> None:
    """
    Set the backlight brightness percentage for the display

    Args:
        value: brightness as percentage from 0 - 100

    Returns:
        None
    """
    global _backlight

    if value == 0 and _backlight:
        _backlight.deinit()
        _backlight = None
        return

    if not _backlight:
        _backlight = PWM(Pin(Pin.board.LCD_BLK), freq=5000, duty_u16=0)

    _backlight.duty_u16((value * 65535) // 100)


def make_speaker(num_chan: int, bits: int, sample_rate: int) -> I2S:
    """
    Make a speaker/DAC using I2S to the PCM5100 stereo DAC

    Also disables output mute by setting Pin.board.DAC_XSMT pin to high

    Args:
        num_chan: number of audio channels - 1 or 2
        bits: number of bits in a sample - 8, 16, 32
        sample_rate: audio sample rate in Hz

    Returns:
        I2S object
    """
    Pin(Pin.board.DAC_XSMT, Pin.OUT, value=1)
    return I2S(0, sck=Pin(Pin.board.DAC_BCK), ws=Pin(Pin.board.DAC_WS),
               sd=Pin(Pin.board.DAC_DIN), mode=I2S.TX, bits=bits,
               format=I2S.STEREO if num_chan == 2 else I2S.MONO,
               rate=sample_rate, ibuf=16_000)


def make_mic(sample_rate: int=16000) -> I2S:
    """
    Make a microphone using I2S

    Args:
        sample_rate: audio sample rate in Hz

    Returns:
        I2S object
    """
    return I2S(1, sck=Pin(Pin.board.MIC_SCK), ws=Pin(Pin.board.MIC_WS), sd=Pin(Pin.board.MIC_SD),
               mode=I2S.RX, bits=16, format=I2S.MONO, rate=sample_rate, ibuf=16_000)


def make_touch() -> CST8XX:
    """
    Make a touchscreen controller using I2C for the CS816 device

    Returns:
        CST8XX instance
    """
    return CST8XX(I2C(0), intr=Pin(Pin.board.TP_INT, Pin.IN))


def make_display() -> ST77916:
    """
    Make a display instance based on FrameBuffer for the ST77916 round controller

    Also setups a PWM controller for the backlight, initially set at 75%

    Returns:
        ST77916 instance
    """
    backlight(75)
    qspi = QSPI(1, Pin(Pin.board.QSPI_SCL),
                [Pin.board.QSPI_D0, Pin.board.QSPI_D1, Pin.board.QSPI_D2, Pin.board.QSPI_D3],
                40_000_000)
    display = ST77916(qspi, cs=Pin(Pin.board.QSPI_CS, Pin.OUT),
                      reset=Pin(Pin.board.LCD_RST, Pin.OUT, value=1))
    display.fill(0)
    display.show()
    return display


def mount_sd(root: str="/sd") -> None:
    """
    Mount the SD card using the SDIO controller

    Args:
        root: directory point to mount the VFS

    Returns:
        None
    """
    sd = SDCard(slot=0, width=4, sck=Pin.board.SDMMC_SCK, cmd=Pin.board.SDMMC_CMD,
                data=(Pin.board.SDMMC_D0, Pin.board.SDMMC_D1, Pin.board.SDMMC_D2, Pin.board.SDMMC_D3),
                freq=80_000_000)
    mount(sd, root)
