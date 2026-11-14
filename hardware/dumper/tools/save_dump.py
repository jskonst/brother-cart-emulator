#!/usr/bin/env python3
"""Save a read-only CK-35 cartridge dump from the Mega sketch.

The sketch speaks 115200 8N1. After it prints CK35-DUMP 1 and a VMEM estimate,
this script sends ``G`` and writes ``ck35-rom.bin`` and ``ck35-ram.bin``.
It does not send any other command. The sketch has no write command.
"""

import argparse
import sys
import time

try:
    import serial
except ImportError:
    sys.stderr.write("pip install pyserial\n")
    sys.exit(1)


def read_line(port, deadline):
    buf = bytearray()
    while time.time() < deadline:
        chunk = port.read(1)
        if not chunk:
            continue
        if chunk == b"\n":
            return buf.decode("ascii", "replace").strip()
        if chunk != b"\r":
            buf.extend(chunk)
    raise TimeoutError("no serial line")


def read_exact(port, count):
    data = bytearray()
    while len(data) < count:
        chunk = port.read(count - len(data))
        if not chunk:
            raise TimeoutError("short read, got %d of %d" % (len(data), count))
        data.extend(chunk)
    return bytes(data)


def take_region(port, name, out_path):
    header = read_line(port, time.time() + 30)
    expect = "BEGIN %s " % name
    if not header.startswith(expect):
        raise RuntimeError("expected %r, got %r" % (expect, header))
    length = int(header.split()[-1])
    payload = read_exact(port, length)
    end = read_line(port, time.time() + 30)
    if end != "END %s" % name:
        raise RuntimeError("expected end of %s, got %r" % (name, end))
    with open(out_path, "wb") as handle:
        handle.write(payload)
    print("%s: %d bytes -> %s" % (name, len(payload), out_path))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("port")
    parser.add_argument("--rom", default="ck35-rom.bin")
    parser.add_argument("--ram", default="ck35-ram.bin")
    args = parser.parse_args()

    port = serial.Serial(args.port, 115200, timeout=2)
    time.sleep(2.0)  # Mega resets when the port opens
    port.reset_input_buffer()
    seen = False
    deadline = time.time() + 15
    while time.time() < deadline:
        line = read_line(port, deadline)
        print(line)
        if line.startswith("CK35-DUMP"):
            seen = True
        if seen and line.startswith("Send G"):
            break
    if not seen:
        raise RuntimeError("dumper banner not seen")
    port.write(b"G")
    port.flush()
    take_region(port, "ROM", args.rom)
    take_region(port, "RAM", args.ram)
    done = read_line(port, time.time() + 10)
    print(done)
    if done != "DONE":
        raise RuntimeError("dump did not finish cleanly")


if __name__ == "__main__":
    main()
