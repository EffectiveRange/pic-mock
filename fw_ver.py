#!/usr/bin/python3

import subprocess
import pathlib
import argparse
import tempfile

argparser = argparse.ArgumentParser(description="Generate version file.")
argparser.add_argument("infile", type=pathlib.Path, help="Input file", nargs="?")
argparser.add_argument(
    "--semver", action="store_true", help="Use semver versioning", default=False
)

args = argparser.parse_args()
if args.infile is None and args.semver is False:
    print("No input file provided. Use --semver to generate a version file.")
    exit(1)

try:
    git_tag = (
        subprocess.check_output(
            ["git", "describe", "--tags"], stderr=subprocess.DEVNULL
        )
        .decode("utf-8")
        .strip()
    )
except subprocess.CalledProcessError:
    git_tag = "0.0.1"
semver = git_tag.lstrip("v").split("-")[0].split(".")

if args.semver:
    print(".".join(semver))
else:
    infile: pathlib.Path = args.infile

    with open(args.infile) as f:
        content = f.read()

    result = content.format(
        major_ver=semver[0],
        minor_ver=semver[1],
        patch_ver=semver[2],
    )

    result_file = infile.parent / infile.stem
    if not result_file.exists() or result_file.read_text() != result:
        print(f"Updating version file: {result_file} as content changed!")
        with open(result_file, "w") as f:
            f.write(result)
