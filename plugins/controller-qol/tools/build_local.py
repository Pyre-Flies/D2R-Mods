"""Reproducible local MSVC build; optional --source/--build support the QOL tree.
For other machines, use a VS x64 Developer Prompt and the documented CMake commands.
"""
import argparse
import os
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source', type=Path, default=Path(__file__).resolve().parents[1])
parser.add_argument('--build', type=Path)
parser.add_argument('--vc', type=Path, default=Path(r'C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Tools\MSVC\14.44.35207'))
parser.add_argument('--sdk-version', default='10.0.26100.0')
args = parser.parse_args()
source = args.source.resolve()
build = args.build or source/'build-potions'
kit = Path(r'C:\Program Files (x86)\Windows Kits\10')
env = {k.upper(): v for k, v in os.environ.items()}  # Avoid duplicate Path/PATH in MSBuild.
env['PATH'] = str(args.vc/'bin/Hostx64/x64')+';'+str(kit/'bin'/args.sdk_version/'x64')+';'+env.get('PATH','')
env['INCLUDE'] = ';'.join(map(str, [args.vc/'include']+[kit/'Include'/args.sdk_version/p for p in ['ucrt','shared','um','winrt']]))
env['LIB'] = ';'.join(map(str, [args.vc/'lib/x64']+[kit/'Lib'/args.sdk_version/p for p in ['ucrt/x64','um/x64']]))
cmake = r'C:\Program Files\CMake\bin\cmake.exe'
for command in [[cmake,'-S',str(source),'-B',str(build),'-G','NMake Makefiles','-DCMAKE_BUILD_TYPE=Release'],
                [cmake,'--build',str(build)],
                [r'C:\Program Files\CMake\bin\ctest.exe','--test-dir',str(build),'--output-on-failure']]:
    subprocess.run(command, env=env, check=True)
