#!/usr/bin/env python3
"""Extract ChocToObj integration-test inputs from the APDL ISO image."""

import argparse
import struct
from pathlib import Path, PurePosixPath


SECTOR_SIZE = 2048


def read_u32_le(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def normalize_identifier(identifier):
    return identifier.split(";", 1)[0].upper()


class ISO9660:
    def __init__(self, path):
        self.file = open(path, "rb")
        self.file.seek(16 * SECTOR_SIZE)
        pvd = self.file.read(SECTOR_SIZE)
        if len(pvd) != SECTOR_SIZE or pvd[:7] != b"\x01CD001\x01":
            raise ValueError("not an ISO 9660 image with a primary descriptor")
        self.root = self.parse_record(pvd, 156)

    def close(self):
        self.file.close()

    @staticmethod
    def parse_record(data, offset):
        length = data[offset]
        if length == 0 or offset + length > len(data):
            raise ValueError("invalid ISO 9660 directory record")
        name_length = data[offset + 32]
        name = data[offset + 33:offset + 33 + name_length]
        return {
            "extent": read_u32_le(data, offset + 2),
            "size": read_u32_le(data, offset + 10),
            "directory": bool(data[offset + 25] & 2),
            "name": name.decode("ascii"),
        }

    def read_record(self, record):
        self.file.seek(record["extent"] * SECTOR_SIZE)
        data = self.file.read(record["size"])
        if len(data) != record["size"]:
            raise ValueError("truncated ISO 9660 extent")
        return data

    def children(self, directory):
        data = self.read_record(directory)
        offset = 0
        while offset < len(data):
            length = data[offset]
            if length == 0:
                offset = ((offset // SECTOR_SIZE) + 1) * SECTOR_SIZE
                continue
            record = self.parse_record(data, offset)
            offset += length
            if record["name"] not in ("\x00", "\x01"):
                yield record

    def find(self, path):
        record = self.root
        for component in PurePosixPath(path).parts:
            if component in ("/", ""):
                continue
            wanted = normalize_identifier(component)
            if not record["directory"]:
                raise FileNotFoundError(path)
            record = next(
                (child for child in self.children(record)
                 if normalize_identifier(child["name"]) == wanted),
                None,
            )
            if record is None:
                raise FileNotFoundError(path)
        return record


def catalogue_entries(data):
    for line in data.decode("latin-1").splitlines():
        fields = line.split("\t")
        if len(fields) == 3:
            yield fields[1], fields[0]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("iso")
    parser.add_argument("output_directory")
    args = parser.parse_args()

    iso = ISO9660(args.iso)
    try:
        catalogue = dict(catalogue_entries(
            iso.read_record(iso.find("/_INSTALL/DATA/000"))))
        output_directory = Path(args.output_directory)
        output_directory.mkdir(parents=True, exist_ok=True)
        files = [
            ("ChocksAway.!Chocks.Maps.Land", "Land"),
            ("ChocksAway.!Chocks.Maps.Obj3D", "Obj3D"),
            ("ChocksAway.ExtraMissions.!Maps_2.LAND", "Extra/Land"),
        ]
        for suffix in tuple("0123456789ABCDEF") + ("10",):
            files.append(("ChocksAway.ExtraMissions.!Maps_2.LandEx" + suffix,
                          "Extra/LandEx" + suffix))
            files.append(("ChocksAway.ExtraMissions.!Maps_2.THINGS.Obj3D" + suffix,
                          "Extra/Obj3D" + suffix))

        for riscos_path, output_name in files:
            identifier = catalogue.get(riscos_path)
            if identifier is None:
                raise ValueError("file is absent from installer catalogue: "
                                 + riscos_path)
            contents = iso.read_record(
                iso.find("/_INSTALL/DATA/" + identifier))
            output_path = output_directory / output_name
            output_path.parent.mkdir(parents=True, exist_ok=True)
            output_path.write_bytes(contents)
    finally:
        iso.close()


if __name__ == "__main__":
    main()
