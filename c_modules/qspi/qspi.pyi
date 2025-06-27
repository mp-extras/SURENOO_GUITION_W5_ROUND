class QSPI:
    def __init__(self, dev_id: int, sck, data_pins: list, baudrate: int, *, polarity: int=0, phase: int=0):
        pass

    def write_cmd(self, cmd: int, params: bytes | bytearray | None):
        pass

    def write_data(self, data: bytes | bytearray):
        pass

    def write_data_rotate(self, data: bytes, swap: bytearray):
        pass

    def deinit(self):
        pass
