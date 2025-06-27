import array
import sys

from board import make_display, make_mic, make_speaker, make_touch, mount_sd
from cst8xx import CST8XX
from machine import I2S
from st77916 import ST77916

SAMPLE_RATE = 16_000
RECORD = 0
PLAY = 1
AUDIO_FILE = "/sd/audio.raw"


def record(mic: I2S):
    with open(AUDIO_FILE, "wb") as audio_file:
        buffer = array.array("h", [0] * 1000)
        for _ in range(32):
            mic.readinto(buffer)
            audio_file.write(buffer)


def play(spkr: I2S):
    with open(AUDIO_FILE, "rb") as audio_file:
        buffer = array.array("h", [0] * 1000)
        for _ in range(32):
            audio_file.readinto(buffer)
            spkr.write(buffer)


def update_ui(display: ST77916, ts: CST8XX):
    def in_box(touch, box):
        if (touch[0] > box[0] and touch[0] < (box[0] + box[2]) and
            touch[1] > box[1] and touch[1] < (box[1] + box[3])):
            return True
        return False

    record_box = (50, 150, 60, 60)
    play_box = (250, 150, 60, 60)

    display.fill(display.rgb(50, 50, 50))

    display.rect(*record_box, display.rgb(128, 0, 0))
    display.text("Record", record_box[0], (record_box[1] + record_box[3] + 2))
    display.rect(*play_box, display.rgb(0, 0, 128))
    display.text("Play", play_box[0], (play_box[1] + play_box[3] + 2))

    result = None
    if point := ts.points:
        if in_box(point, record_box):
            print("record")
            display.rect(*record_box, display.rgb(128, 0, 0), True)
            result = RECORD
        elif in_box(point, play_box):
            print("play")
            display.rect(*play_box, display.rgb(0, 0, 128), True)
            result = PLAY

    display.show()

    return result


def main():
    try:
        mount_sd("/sd")
    except OSError as e:
        print(f"Insure a FAT formatted SD card is inserted: OSError {e}")
        sys.exit(1)

    spkr = make_speaker(1, 16, SAMPLE_RATE)
    mic = make_mic(SAMPLE_RATE)
    display = make_display()
    ts = make_touch()

    try:
        while True:
            if (pressed := update_ui(display, ts)) == RECORD:
                record(mic)
            elif pressed == PLAY:
                play(spkr)
    except KeyboardInterrupt:
        pass
    finally:
        display.deinit()


main()
