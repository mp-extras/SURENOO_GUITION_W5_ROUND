from os import listdir

from board import mount_sd


def main():
    mount_sd()
    print(listdir("/sd"))


main()
