"""One-time catalog builder from a reviewed skills table; no table is packaged."""
import csv
import argparse
from pathlib import Path

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('skills_table', type=Path)
source = parser.parse_args().skills_table
rows = list(csv.DictReader(source.open(encoding='utf-8-sig'), delimiter='\t'))
classes = {'ama':'amazon','sor':'sorceress','nec':'necromancer','pal':'paladin',
           'bar':'barbarian','dru':'druid','ass':'assassin','war':'warlock'}
labels = {12:'Multi Shot',85:'Blood Golem',90:'Iron Golem',94:'Fire Golem',
          222:'Poison Creeper',223:'Werewolf',224:'Lycanthropy',228:'Werebear',
          231:'Carrion Vine',234:'Fissure',236:'Heart of Wolverine',237:'Summon Dire Wolf',
          241:'Solar Creeper',251:'Fire Blast',256:'Shock Web',258:'Burst of Speed',
          262:'Wake of Fire',272:'Wake of Inferno',280:'Phoenix Strike',
          393:'Sigil of Lethargy',396:'Sigil of Rancor',400:'Sigil of Death'}
tested = {12,22,51,54,56,59,62,67,132,151}
ground = {54,132,78,75,85,94,32,28,221,222,226,227,231,236,237,241,246,247,
          256,257,261,262,268,271,272,276,279,373,376,377,390,393,396,400}
snap_placement = {234,244,251,256,257,261,262,271,272,276,393,396,400}
curses = {66,71,72,76,77,81,82,86,87,91}
directional = {6,7,11,12,15,16,20,21,22,25,26,27,31,35,
               36,38,39,41,43,45,47,49,53,55,64,67,84,93,101,121,140,
               225,229,230,240,243,245,253,266,273,384,388,391,395,398,399}
placement = {78,234,244}
self_cast = {8,17,40,42,44,46,48,50,52,57,58,60,68,92,117,130,138,146,
             149,154,155,223,228,235,249,250,258,267,277,278}
entries = []
for row in rows:
    if row['charclass'] not in classes:
        continue
    sid = int(row['*Id'])
    label = labels.get(sid, row['skill'])
    snap = sid in snap_placement or sid not in ground and (row['range'] in ('rng','h2h','both') or sid in curses or sid in tested or sid in directional)
    if sid in tested:
        behavior = 'tested snap aim' if snap else 'tested ground aim'
    elif sid == 43:
        behavior = 'native unit/item/object target; untested'
    elif sid == 251:
        behavior = 'snap placement; untested'
    elif row['TargetCorpse'] == '1':
        behavior = 'corpse target; keep native'
    elif row['passive'] == '1':
        behavior = 'passive; keep native'
    elif row['charclass'] == 'pal' and row['aura'] == '1' and row['range'] == 'none':
        behavior = 'aura; keep native'
    elif sid in {44,48,92}:
        behavior = 'self-centered area; keep native'
    elif sid in curses:
        behavior = 'curse/area target; untested'
    elif sid in placement:
        behavior = 'ground placement; untested'
    elif sid in directional:
        behavior = 'directional target; untested'
    elif row['summon']:
        behavior = 'summon/placement; untested'
    elif row['range'] == 'h2h':
        behavior = 'melee/unit target; untested'
    elif sid in self_cast:
        behavior = 'self/buff/automatic; keep native'
    elif row['range'] == 'none':
        behavior = 'native behavior; review before enabling'
    else:
        behavior = 'point/directional target; untested'
    entries.append((sid,label,classes[row['charclass']],behavior,snap,sid in tested))
assert len(entries) == 240 and len({e[0] for e in entries}) == 240
assert all(sum(e[2]==c for e in entries)==30 for c in classes.values())
header = '''#pragma once
#include <array>
#include <cstddef>
namespace Aim {
// Semantic catalog only, not extracted game records. See SKILL-CATALOG.md.
struct CatalogSkill {
    int id; const char* name; const char* className; const char* review;
    bool snap; bool enabledByDefault;
};
inline constexpr std::array<CatalogSkill,240> SkillCatalog{{
'''
for sid,label,cls,behavior,snap,enabled in entries:
    header += f'    {{{sid},"{label}","{cls}","{behavior}",{str(snap).lower()},{str(enabled).lower()}}},\n'
header += '''}};
inline const CatalogSkill* FindCatalogSkill(int id) noexcept {
    for(const auto& skill:SkillCatalog) if(skill.id==id) return &skill;
    return nullptr;
}
inline const char* CatalogSkillName(int id) noexcept {
    const auto* skill=FindCatalogSkill(id);
    return skill?skill->name:"Custom skill";
}
}
'''
(root/'src/aim/skill_catalog.h').write_text(header,encoding='utf-8')
config = (root/'skill-defaults.toml').read_text()
config = config[:config.index('# Class headings')]
config += '# Class headings are readability only; IDs apply on any character.\n'
config += '# true enables aim; false is off but R3-toggleable; "disabled" locks off.\n'
config += '# Passive entries default "disabled"; other untested skills default false.\n'
config += '# Vanilla names below are labels only; mods may replace an ID\'s behavior.\n'
for cls in classes.values():
    config += f'\n[aim.{cls}]\n'
    for sid,label,entry_cls,behavior,snap,enabled in entries:
        if entry_cls == cls:
            value = '"disabled"' if behavior.startswith('passive;') else str(enabled).lower()
            config += f'# {label} — {behavior}\n"{sid}" = {value}\n'
config += '''
# Additional IDs outside the class catalog; use the active mod's skills table.
# true enables the configured targeting mode; false is off; "disabled" locks off. 1024 total IDs including class/discovered entries (1..65534).
[aim.custom]
# Recognized Warp is discovered automatically. Manual fallback in a skill profile:
# "429" = false # verify the active mod ID and set its ground mode below

# Optional targeting overrides for built-in or declared custom IDs.
# Changes targeting only; does not enable a skill.
[aim.targeting]
"136" = "snap" # Reimagined Winter's Gambit
"141" = "snap" # Reimagined Chasm Break
# "429" = "ground" # manual Warp fallback only

'''
from leading_defaults import render_leading_defaults
config += render_leading_defaults()
config += '\n[aim.whirlwind]\n# false snaps onto the enemy without extending past it.\nwhirlwind_pass_through_enabled = true\nwhirlwind_pass_through_distance = 1.5 # world tiles beyond enemy, 0..15\n'
(root/'skill-defaults.toml').write_text(config,encoding='utf-8')
print('Generated 240 class entries and reviewed leading baselines.')
