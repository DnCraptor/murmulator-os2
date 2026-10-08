#!/usr/bin/env python3
"""Compatibility entry point: install the MOS SDK, TCC and hello.c."""
from install_sdk import main

if __name__ == '__main__':
    main(with_tcc=True)
