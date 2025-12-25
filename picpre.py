#!/usr/bin/python3
import argparse
import pathlib
import re
import os
import sys


def main():
    regdef_exception_list = {
        "MAINPR",
        "ISRPR",
        "DMA1PR",
        "DMA2PR",
        "DMA3PR",
        "DMA4PR",
        "SCANPR",
        "NVMLOCK",
        "NVMADRL",
        "NVMADRH",
        "NVMADRU",
        "NVMDATL",
        "NVMDATH",
        "WDTTMR",
        "TMR0L",
        "TMR0H",
        "TMR1L",
        "TMR1H",
        "TMR2",
        "PR2",
        "TMR4",
        "PR4",
        "TU16APS",
        "TU16BPS",
        "CWG1ISM",
        "CWG1DBR",
        "ADACC",
        "ADACQ",
        "ADACT",
        "ADACTPPS",
        "ADCNT",
        "ADERR",
        "ADFLTR",
        "ADLTH",
        "ADPRE",
        "ADPREV",
        "ADRES",
        "ADRPT",
        "ADSTAT",
        "ADSTPT",
        "ADUTH",
        "CCP1PPS",
        "CCP2PPS",
        "CLCIN0PPS",
        "CLCIN1PPS",
        "CLCIN2PPS",
        "CLCIN3PPS",
        "CWG1DBF",
        "CWG1INPPS",
        "FSR0H",
        "FSR0L",
        "FSR1H",
        "FSR1L",
        "FSR2H",
        "FSR2L",
        "I2C1CNT",
        "I2C1SCLPPS",
        "I2C1SDAPPS",
        "INDF0",
        "INDF1",
        "INDF2",
        "INT0PPS",
        "INT1PPS",
        "INT2PPS",
        "IVTADH",
        "IVTADL",
        "IVTADU",
        "IVTBASEH",
        "IVTBASEL",
        "IVTBASEU",
        "PLUSW0",
        "PLUSW1",
        "PLUSW2",
        "POSTDEC0",
        "POSTDEC1",
        "POSTDEC2",
        "POSTINC0",
        "POSTINC1",
        "POSTINC2",
        "PREINC0",
        "PREINC1",
        "PREINC2",
        "PRODH",
        "PRODL",
        "PWM1ERSPPS",
        "PWM2ERSPPS",
        "PWMIN0PPS",
        "PWMIN1PPS",
        "SPI1SCKPPS",
        "SPI1SDIPPS",
        "SPI1SSPPS",
        "STKPTR",
        "T0CKIPPS",
        "T1CKIPPS",
        "T1GPPS",
        "T2INPPS",
        "T4INPPS",
        "TABLAT",
        "TBLPTRH",
        "TBLPTRL",
        "TBLPTRU",
        "TOSH",
        "TOSL",
        "TUIN0PPS",
        "TUIN1PPS",
        "U1CTSPPS",
        "U1RXPPS",
        "U2CTSPPS",
        "U2RXPPS",
        "WREG",
    }
    regdef_re = re.compile(
        r"\s*extern\s*(?P<cvqual>volatile|const)\s*unsigned (?P<typename>char|short|long)\s+(?P<regname>\w+)\s*(?:\[(?P<arraysz>\d+)\])?\s+__at\((?P<address>0x[0-9a-fA-F]+)\)\s*;"
    )
    reg_structdef_re = re.compile(
        r"\s*extern\s+(?P<cvqual>volatile|const)\s+(?P<structname>\w+)\s+(?P<regname>\w+)\s+__at\((?P<address>0x[0-9a-fA-F]+)\)\s*;"
    )

    reg_struct_typedef_re = re.compile(r"\s*\}\s*(\w+)bits_t\s*;")

    parser = argparse.ArgumentParser(description="pic header preprocessor")
    parser.add_argument(
        "-o",
        "--output",
        help="Output directory to generate source files",
        default=pathlib.Path("./__picgen"),
        type=pathlib.Path,
    )
    parser.add_argument("input", help="Input file", type=pathlib.Path)
    args = parser.parse_args()
    inputfile: pathlib.Path = args.input
    outdir: pathlib.Path = args.output
    os.makedirs(outdir, exist_ok=True)
    with open(inputfile, "rb") as f:
        lines = f.readlines()
        regdefintions = []
        regmacrodefs = []
        with open(outdir / inputfile.name, "w") as outheader:
            for line in lines:
                decoded = line.decode()
                regdef_match = regdef_re.match(decoded)
                regstructdef_match = reg_structdef_re.match(decoded)
                regstruct_typedef_match = reg_struct_typedef_re.match(decoded)
                if regdef_match:
                    matchd = regdef_match.groupdict()
                    if matchd["regname"] in regdef_exception_list:
                        continue
                    if "arraysz" in matchd and matchd["arraysz"] is not None:
                        outheader.write(
                            f"extern {matchd['cvqual']} {matchd['typename']} {matchd['regname']}[{matchd['arraysz']}];\n"
                        )
                    else:
                        outheader.write(f"#ifdef {matchd['regname']}\n")
                        outheader.write(f"#undef {matchd['regname']}\n")
                        outheader.write(f"#endif // {matchd['regname']}\n")
                        outheader.write(
                            f"#define {matchd['regname']} {matchd['regname']}bits.__value\n"
                        )

                elif regstructdef_match:
                    matchd = regstructdef_match.groupdict()
                    basename = matchd["regname"].removesuffix("bits")
                    outheader.write(
                        f"extern volatile {matchd['structname']} {matchd['regname']};\n"
                    )
                    if basename in regdef_exception_list:
                        regmacrodefs.append(f"#ifdef {basename}\n")
                        regmacrodefs.append(f"#undef {basename}\n")
                        regmacrodefs.append(f"#endif // {basename}\n")
                        regmacrodefs.append(
                            f"#define {basename} {matchd['regname']}.__value\n"
                        )
                    regdefintions.append(
                        f"volatile {matchd['structname']} {matchd['regname']}"
                    )
                elif regstruct_typedef_match:
                    outheader.write(
                        f"unsigned __value;{regstruct_typedef_match.group(0)}\n"
                    )
                else:
                    outheader.write(line.decode())

            for macrodef in regmacrodefs:
                outheader.write(macrodef)

        with open(outdir / f"{inputfile.stem}.c", "w") as outsrc:
            outsrc.write(f"#define _XC_H_\n\n")
            outsrc.write(f"#define __CCI__\n\n")
            outsrc.write(f"#define _LIB_BUILD\n\n")
            outsrc.write(f"typedef unsigned char __bit;\n\n")
            outsrc.write(f"typedef unsigned long __uint24;\n\n")
            outsrc.write(f'#include "{inputfile.name}"\n\n')
            for defini in regdefintions:
                outsrc.write(f"{defini};\n")


if __name__ == "__main__":
    main()
