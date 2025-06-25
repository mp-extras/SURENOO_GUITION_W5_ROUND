from time import sleep

from board import make_touch


def main():
    cst = make_touch()

    while True:
        if (p := cst.points) and p[2] == 0:
            print(p[0], p[1])

        sleep(0.01)


main()
