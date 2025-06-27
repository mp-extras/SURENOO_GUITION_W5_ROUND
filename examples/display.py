from time import sleep

from board import make_display, backlight


def main():
    display = make_display()
    display.fill(display.rgb(100, 0, 0))
    display.rect(0, 0, display.width, display.height, display.rgb(0, 128, 0))
    display.rect(display.width//2, display.height//2, 40, 40, 0xffff)
    display.text("Hello", display.width//2, display.height//2+50, display.rgb(0, 0, 200))

    display.show()

    for i in [0, 1, 2, 3]:
        backlight(25 + i * 25)
        display.rotation = i
        display.invert = i & 0x1
        display.show()
        sleep(2)

    backlight(0)
    display.deinit()


main()
