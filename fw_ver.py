#!/usr/bin/python3

import subprocess
import pathlib
import argparse

argparser = argparse.ArgumentParser(description="Generate version file.")
argparser.add_argument("infile", type=pathlib.Path, help="Input file", nargs="?")
argparser.add_argument(
    "--semver", action="store_true", help="Use semver versioning", default=False
)

args = argparser.parse_args()
if args.infile is None and args.semver is False:
    print("No input file provided. Use --semver to generate a version file.")
    exit(1)

git_tag = subprocess.check_output(["git", "describe", "--tags"]).decode("utf-8").strip()
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

    with open(infile.parent / infile.stem, "w") as f:
        f.write(result)
