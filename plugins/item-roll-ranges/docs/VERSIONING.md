# Version policy

Use MAJOR.MINOR.PATCH+rev.N: the first three components identify the qualified D2RLoader target; N is the independent revision for this plugin on that target. Increment N for every published build, including packaging fixes. Reset N only when the loader target changes after qualification. Public PluginInfo version and string resources carry the SemVer form. Windows numeric FILEVERSION/PRODUCTVERSION use MAJOR,MINOR,PATCH,N. CMake project VERSION remains numeric.

D2RLoader SDK README requires SemVer2.0.0, at most63 characters; four dotted numeric components displayed as invalid. Reference: https://github.com/D2RLoader/PluginSDK#load-a-plugin (PluginInfo version paragraph). Public releases here are stable; do not use a prerelease suffix solely as a revision counter.

SemVer precedence ignores +build metadata. Our rev.N labels identify releases but do not automatically sort as newer under SemVer comparison. Any future updater must explicitly compare revision numbers within the same target or adopt independent sortable plugin versions. Do not infer loader compatibility from the label alone: existing exact provider and native byte checks stay enforced.

Migration: QOL1.3.1.3 ->1.3.1+rev.4; Item Roll Ranges1.3.1.0 ->1.3.1+rev.1. This release changes metadata/documentation only. Prior archives remain preserved.
