from math import sin, tau
import array

from board import make_speaker

NUM_SAMPLES = 1000


def gen_tone(mul):
    freq = (tau / 40.0) * (1 + mul)
    return array.array("h", [int(8000.0 * sin(freq * i * chan))
                             for i in range(NUM_SAMPLES) for chan in (1, 2)])


def main():
    spkr = make_speaker(2, 16, 16_000)
    samples = gen_tone(1)
    while True:
        spkr.write(samples)


main()
