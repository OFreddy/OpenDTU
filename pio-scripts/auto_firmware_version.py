# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (C) 2022 Thomas Basler and others
#
import pkg_resources

Import("env")

required_pkgs = {'dulwich'}
installed_pkgs = {pkg.key for pkg in pkg_resources.working_set}
missing_pkgs = required_pkgs - installed_pkgs

if missing_pkgs:
    env.Execute('"$PYTHONEXE" -m pip install dulwich')

from dulwich import porcelain

def get_firmware_specifier_build_flag():
    try:
        build_version = porcelain.describe('.')  # '.' refers to the repository root dir
    except:
        build_version = "g0000000"

    print ("###########################################")
    print ("Firmware Revision retuned from git: " + build_version)

    # filter out release tag from build info git describe --tags
    gitindex = build_version.find("-g")
    if gitindex >= 0:
      print ("contains build info at index %i" % gitindex)
      build_version = build_version[gitindex+1:]
      print ("filtered git hash: " + build_version)

    print ("###########################################")

    build_flag = "-D AUTO_GIT_HASH=\\\"" + build_version + "\\\""
    return (build_flag)

env.Append(
    BUILD_FLAGS=[get_firmware_specifier_build_flag()]
)
