#pragma once
// Reload-time compilation of Weapons.txt, Armor.txt, Misc.txt, ItemTypes.txt,
// ItemStatCost.txt, and Skills.txt into fast lookup tables. Zero file access or
// allocations on runtime hooks.

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace PyreLootFilter::Tables {

namespace fs = std::filesystem;

[[nodiscard]] constexpr std::uint32_t CanonicalItemCode(std::uint32_t code) noexcept {
    for (int i = 3; i >= 0; --i) {
        const auto byte = static_cast<unsigned char>((code >> (i * 8)) & 0xFFU);
        if (byte != 0 && byte != ' ') break;
        code &= ~(0xFFU << (i * 8));
    }
    return code;
}

[[nodiscard]] constexpr bool PrintableItemCode(std::uint32_t code) noexcept {
    code = CanonicalItemCode(code);
    if (code == 0) return false;
    bool ended = false;
    for (int i = 0; i < 4; ++i) {
        const auto ch = static_cast<unsigned char>((code >> (i * 8)) & 0xFFU);
        if (ch == 0) {
            ended = true;
            continue;
        }
        if (ended || ch < 0x21U || ch > 0x7EU) return false;
    }
    return true;
}

[[nodiscard]] constexpr std::uint32_t PackCode(std::string_view text) noexcept {
    if (text.empty() || text.size() > 4) return 0;
    std::uint32_t result{};
    for (std::size_t i = 0; i < text.size(); ++i) {
        const auto c = static_cast<unsigned char>(text[i]);
        if (c < 0x21U || c > 0x7EU) {
            if (c != ' ' || i < 3 || i + 1 != text.size()) return 0;
        }
        result |= static_cast<std::uint32_t>(c) << (8U * i);
    }
    return CanonicalItemCode(result);
}

inline constexpr std::uint32_t GoldItemCode = PackCode("gld");

[[nodiscard]] constexpr bool IsGoldCode(std::uint32_t code) noexcept {
    return CanonicalItemCode(code) == GoldItemCode;
}

inline void CodeToText(std::uint32_t code, char (&out)[5]) noexcept {
    for (int i = 0; i < 4; ++i) {
        const auto ch = static_cast<unsigned char>((code >> (i * 8)) & 0xFFU);
        out[i] = ch == 0 ? ' ' : (ch >= 32 && ch <= 126 ? static_cast<char>(ch) : '.');
    }
    out[4] = '\0';
}

inline std::vector<std::string_view> SplitTsv(std::string_view row) {
    std::vector<std::string_view> cells;
    for (std::size_t pos = 0; pos <= row.size();) {
        const auto end = row.find('\t', pos);
        if (end == std::string_view::npos) {
            cells.push_back(row.substr(pos));
            break;
        }
        cells.push_back(row.substr(pos, end - pos));
        pos = end + 1;
    }
    return cells;
}

inline std::size_t ColumnIndex(const std::vector<std::string_view>& header, std::string_view name) noexcept {
    for (std::size_t i = 0; i < header.size(); ++i) {
        if (header[i] == name) return i;
    }
    return header.size();
}

inline bool ReadTsvLines(const fs::path& path, std::vector<std::string>& lines,
                         std::vector<std::string_view>& headings, std::string& error) {
    std::error_code ec;
    const auto bytes = fs::file_size(path, ec);
    if (ec || !bytes || bytes > 32U * 1024U * 1024U) {
        error = "table-missing-empty-or-over-32MiB:" + path.filename().string();
        return false;
    }
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        error = "table-open-failed:" + path.filename().string();
        return false;
    }
    std::string line;
    if (!std::getline(file, line)) {
        error = "table-missing-header:" + path.filename().string();
        return false;
    }
    if (line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    lines.push_back(std::move(line));
    while (std::getline(file, line)) {
        if (line.size() > 65536) {
            error = "table-row-over-64KiB";
            return false;
        }
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) lines.push_back(std::move(line));
        if (lines.size() > 16385) {
            error = "table-over-16384-rows";
            return false;
        }
    }
    if (file.bad()) {
        error = "table-read-failed";
        return false;
    }
    headings = SplitTsv(lines[0]);
    return true;
}

inline fs::path DiscoverExcel(const fs::path& configPath) {
    const auto hasPair = [](const fs::path& dir) {
        std::error_code ec;
        if (!fs::is_regular_file(dir / L"weapons.txt", ec) || ec) return false;
        ec.clear();
        return fs::is_regular_file(dir / L"armor.txt", ec) && !ec;
    };
    for (auto root = configPath.parent_path(); !root.empty();) {
        if (hasPair(root)) return root;
        if (hasPair(root / L"data" / L"global" / L"excel"))
            return root / L"data" / L"global" / L"excel";
        const auto ownMpq = root / (root.filename().wstring() + L".mpq") / L"data" / L"global" / L"excel";
        if (hasPair(ownMpq)) return ownMpq;
        std::vector<fs::path> candidates;
        std::error_code ec;
        for (fs::directory_iterator it(root, ec), end; !ec && it != end; it.increment(ec)) {
            if (!it->is_directory(ec)) {
                if (ec) break;
                continue;
            }
            if (it->path().extension() != L".mpq") continue;
            const auto excel = it->path() / L"data" / L"global" / L"excel";
            if (hasPair(excel)) candidates.push_back(excel);
        }
        if (candidates.size() == 1) return candidates.front();
        if (candidates.size() > 1) return {};
        const auto parent = root.parent_path();
        if (parent == root) break;
        root = parent;
    }
    return {};
}

struct BaseNameCatalog {
    std::unordered_map<std::string, std::vector<std::uint32_t>> byName;
    std::size_t weaponRows{}, armorRows{};

    [[nodiscard]] const std::vector<std::uint32_t>* Find(std::string_view name) const {
        const auto it = byName.find(std::string(name));
        return it == byName.end() ? nullptr : &it->second;
    }
};

inline bool ReadBaseNameTable(const fs::path& path, BaseNameCatalog& out,
                              std::size_t& rowCount, std::string& error) {
    std::vector<std::string> lines;
    std::vector<std::string_view> headings;
    if (!ReadTsvLines(path, lines, headings, error)) return false;
    const auto nameIdx = ColumnIndex(headings, "name");
    const auto codeIdx = ColumnIndex(headings, "code");
    if (nameIdx == headings.size() || codeIdx == headings.size()) {
        error = "baseName-table-requires-literal-name-and-code-columns";
        return false;
    }
    for (std::size_t j = 1; j < lines.size(); ++j) {
        const auto row = SplitTsv(lines[j]);
        if (nameIdx >= row.size() || codeIdx >= row.size()) {
            error = "baseName-table-short-row";
            return false;
        }
        const auto name = row[nameIdx];
        const auto code = row[codeIdx];
        if (name.empty() || code.empty()) continue;
        if (name.size() > 127 || name.find('\r') != name.npos || name.find('\n') != name.npos) {
            error = "baseName-table-invalid-name";
            return false;
        }
        const auto packed = PackCode(code);
        if (!packed) {
            error = "baseName-table-invalid-item-code";
            return false;
        }
        out.byName[std::string(name)].push_back(packed);
        if (++rowCount > 4096) {
            error = "baseName-table-over-4096-named-rows";
            return false;
        }
    }
    if (rowCount == 0) {
        error = "baseName-table-empty";
        return false;
    }
    return true;
}

inline bool LoadBaseNames(const fs::path& excel, BaseNameCatalog& out, std::string& error) {
    BaseNameCatalog staged{};
    if (!ReadBaseNameTable(excel / L"weapons.txt", staged, staged.weaponRows, error) ||
        !ReadBaseNameTable(excel / L"armor.txt", staged, staged.armorRows, error)) {
        return false;
    }
    for (auto& [name, codes] : staged.byName) {
        std::sort(codes.begin(), codes.end());
        codes.erase(std::unique(codes.begin(), codes.end()), codes.end());
    }
    out = std::move(staged);
    return true;
}

struct ItemTypeCatalog {
    struct TypeRow {
        std::string parent1, parent2;
    };
    std::unordered_map<std::string, TypeRow> types;
    std::unordered_map<std::string, std::vector<std::string>> names;
    std::unordered_map<std::string, std::vector<std::uint32_t>> itemCodesByType;
    std::size_t typeRows{}, weaponRows{}, armorRows{}, miscRows{};

    [[nodiscard]] const std::vector<std::uint32_t>* Find(std::string_view typeOrName) const {
        if (const auto it = itemCodesByType.find(std::string(typeOrName));
            it != itemCodesByType.end()) {
            return &it->second;
        }
        const auto toLower = [](unsigned char c) noexcept {
            return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A'))
                                          : static_cast<char>(c);
        };
        for (const auto& [key, codes] : itemCodesByType) {
            if (key.size() != typeOrName.size()) continue;
            bool eq = true;
            for (std::size_t i = 0; i < key.size(); ++i) {
                if (toLower(static_cast<unsigned char>(key[i])) !=
                    toLower(static_cast<unsigned char>(typeOrName[i]))) {
                    eq = false;
                    break;
                }
            }
            if (eq) return &codes;
        }
        return nullptr;
    }
};

inline std::string TrimTypeCode(std::string_view cell) {
    while (!cell.empty() && cell.back() == ' ') cell.remove_suffix(1);
    if (!PackCode(cell)) return {};
    return std::string(cell);
}

inline bool HasNonZeroCell(std::string_view cell) noexcept {
    while (!cell.empty() && cell.front() == ' ') cell.remove_prefix(1);
    while (!cell.empty() && cell.back() == ' ') cell.remove_suffix(1);
    return !cell.empty() && cell != "0";
}

inline bool ReadItemTypesFile(const fs::path& excel, ItemTypeCatalog& out, std::string& error) {
    std::vector<std::string> lines;
    std::vector<std::string_view> headings;
    if (!ReadTsvLines(excel / L"itemtypes.txt", lines, headings, error)) return false;
    const auto codeCol = ColumnIndex(headings, "Code");
    const auto nameCol = ColumnIndex(headings, "ItemType");
    const auto p1Col = ColumnIndex(headings, "Equiv1");
    const auto p2Col = ColumnIndex(headings, "Equiv2");
    if (codeCol == headings.size() || nameCol == headings.size() ||
        p1Col == headings.size() || p2Col == headings.size()) {
        error = "itemType-requires-ItemType-Code-Equiv1-Equiv2-columns";
        return false;
    }
    for (std::size_t j = 1; j < lines.size(); ++j) {
        const auto cells = SplitTsv(lines[j]);
        if (codeCol >= cells.size() || cells[codeCol].empty()) continue;
        const auto id = TrimTypeCode(cells[codeCol]);
        if (id.empty()) {
            error = "itemType-invalid-type-Code";
            return false;
        }
        const auto rawName = nameCol < cells.size() ? cells[nameCol] : std::string_view{};
        if (rawName.size() > 127 || rawName.find('\n') != rawName.npos || rawName.find('\r') != rawName.npos) {
            error = "itemType-invalid-ItemType-name";
            return false;
        }
        const auto parseParent = [&](std::size_t col, std::string& parent) {
            if (col >= cells.size() || cells[col].empty()) return true;
            parent = TrimTypeCode(cells[col]);
            return !parent.empty();
        };
        ItemTypeCatalog::TypeRow row;
        if (!parseParent(p1Col, row.parent1) || !parseParent(p2Col, row.parent2)) {
            error = "itemType-invalid-Equiv-code";
            return false;
        }
        if (!out.types.emplace(id, std::move(row)).second) {
            error = "itemType-duplicate-Code:" + id;
            return false;
        }
        if (!rawName.empty()) out.names[std::string(rawName)].push_back(id);
        ++out.typeRows;
    }
    if (!out.typeRows) {
        error = "itemType-no-types";
        return false;
    }
    return true;
}

inline bool ReadTypedItemsFile(const fs::path& excel, const wchar_t* filename,
                               ItemTypeCatalog& out, std::size_t& count, std::string& error,
                               std::unordered_map<std::uint32_t, std::vector<std::string>>& direct) {
    std::vector<std::string> lines;
    std::vector<std::string_view> headings;
    if (!ReadTsvLines(excel / filename, lines, headings, error)) return false;
    const auto codeCol = ColumnIndex(headings, "code");
    const auto typeCol = ColumnIndex(headings, "type");
    const auto type2Col = ColumnIndex(headings, "type2");
    const auto questCol = ColumnIndex(headings, "quest");
    if (codeCol == headings.size() || typeCol == headings.size()) {
        error = "itemType-item-table-requires-code-and-type-columns:" + (excel / filename).filename().string();
        return false;
    }
    for (std::size_t j = 1; j < lines.size(); ++j) {
        const auto cells = SplitTsv(lines[j]);
        if (codeCol >= cells.size() || typeCol >= cells.size() ||
            cells[codeCol].empty() || cells[typeCol].empty()) {
            continue;
        }
        const auto packed = PackCode(cells[codeCol]);
        const auto primary = TrimTypeCode(cells[typeCol]);
        if (!packed || primary.empty() || !out.types.contains(primary)) {
            error = "itemType-invalid-item-code-or-unknown-primary-type";
            return false;
        }
        direct[packed].push_back(primary);
        if (type2Col < cells.size() && !cells[type2Col].empty()) {
            const auto secondary = TrimTypeCode(cells[type2Col]);
            if (secondary.empty() || !out.types.contains(secondary)) {
                error = "itemType-invalid-or-unknown-type2";
                return false;
            }
            direct[packed].push_back(secondary);
        }
        if (questCol < cells.size() && HasNonZeroCell(cells[questCol]) && out.types.contains("ques")) {
            direct[packed].push_back("ques");
        }
        ++count;
    }
    return true;
}

inline bool ResolveTypeAncestors(const ItemTypeCatalog& out, const std::string& id,
                                 std::unordered_map<std::string, std::vector<std::string>>& memo,
                                 std::unordered_set<std::string>& visiting, std::string& error) {
    if (memo.contains(id)) return true;
    const auto row = out.types.find(id);
    if (row == out.types.end()) {
        error = "itemType-unknown-Equiv:" + id;
        return false;
    }
    if (!visiting.insert(id).second) {
        error = "itemType-cyclic-Equiv:" + id;
        return false;
    }
    std::vector<std::string> result{id};
    for (const auto* parent : {&row->second.parent1, &row->second.parent2}) {
        if (parent->empty()) continue;
        if (!ResolveTypeAncestors(out, *parent, memo, visiting, error)) return false;
        const auto& related = memo.at(*parent);
        result.insert(result.end(), related.begin(), related.end());
    }
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    visiting.erase(id);
    memo.emplace(id, std::move(result));
    return true;
}

inline bool LoadItemTypes(const fs::path& excel, ItemTypeCatalog& out, std::string& error) {
    ItemTypeCatalog staged{};
    if (!ReadItemTypesFile(excel, staged, error)) return false;
    std::unordered_map<std::uint32_t, std::vector<std::string>> direct;
    if (!ReadTypedItemsFile(excel, L"weapons.txt", staged, staged.weaponRows, error, direct) ||
        !ReadTypedItemsFile(excel, L"armor.txt", staged, staged.armorRows, error, direct) ||
        !ReadTypedItemsFile(excel, L"misc.txt", staged, staged.miscRows, error, direct)) {
        return false;
    }
    std::unordered_map<std::string, std::vector<std::string>> memo;
    std::unordered_set<std::string> visiting;
    for (const auto& [id, row] : staged.types) {
        if (!ResolveTypeAncestors(staged, id, memo, visiting, error)) return false;
    }
    for (const auto& [id, row] : staged.types) staged.itemCodesByType[id];
    for (const auto& [itemCode, types] : direct) {
        for (const auto& id : types) {
            for (const auto& ancestor : memo.at(id)) {
                staged.itemCodesByType[ancestor].push_back(itemCode);
            }
        }
    }
    for (auto& [type, codes] : staged.itemCodesByType) {
        std::sort(codes.begin(), codes.end());
        codes.erase(std::unique(codes.begin(), codes.end()), codes.end());
    }
    for (const auto& [name, types] : staged.names) {
        if (staged.itemCodesByType.contains(name) && staged.types.contains(name)) continue;
        auto& codes = staged.itemCodesByType[name];
        for (const auto& type : types) {
            const auto it = staged.itemCodesByType.find(type);
            if (it != staged.itemCodesByType.end()) {
                codes.insert(codes.end(), it->second.begin(), it->second.end());
            }
        }
        std::sort(codes.begin(), codes.end());
        codes.erase(std::unique(codes.begin(), codes.end()), codes.end());
    }
    out = std::move(staged);
    return true;
}

enum class StatKind : std::uint8_t {
    Standard = 0,
    AllResist = 1,      // min(fire[39], light[41], cold[43], poison[45])
    TotalResist = 2,    // fire[39] + light[41] + cold[43] + poison[45]
    AnyResist = 3,      // max(fire[39], light[41], cold[43], poison[45])
    EnhancedDamage = 4, // max(item_maxdamage_percent[17], item_mindamage_percent[18])
};

[[nodiscard]] inline std::string NormalizeIdentifier(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (const unsigned char ch : text) {
        if (ch >= 'A' && ch <= 'Z') {
            out.push_back(static_cast<char>(ch + ('a' - 'A')));
        } else if ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) {
            out.push_back(static_cast<char>(ch));
        }
    }
    return out;
}

[[nodiscard]] inline bool ParseUnsigned16(std::string_view text, std::uint16_t maxVal,
                                          std::uint16_t& out) noexcept {
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t')) text.remove_prefix(1);
    while (!text.empty() && (text.back() == ' ' || text.back() == '\t')) text.remove_suffix(1);
    if (text.empty()) return false;
    unsigned int val = 0;
    const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), val, 10);
    if (ec != std::errc{} || end != text.data() + text.size() || val > maxVal) return false;
    out = static_cast<std::uint16_t>(val);
    return true;
}

struct StatCostCatalog {
    std::unordered_map<std::string, std::uint16_t> byExact;
    std::unordered_map<std::string, std::uint16_t> byNormalized;
    std::size_t rows{};

    [[nodiscard]] bool Resolve(std::string_view token, StatKind& outKind,
                               std::uint16_t& outStatId) const {
        outKind = StatKind::Standard;
        outStatId = 0;
        const auto norm = NormalizeIdentifier(token);
        if (norm.empty()) return false;

        if (norm == "allresist" || norm == "allres" || norm == "allresistance" ||
            norm == "allresistances" || norm == "resall" || norm == "resistall") {
            outKind = StatKind::AllResist;
            return true;
        }
        if (norm == "totalresist" || norm == "totalres" || norm == "sumresist" ||
            norm == "anyresistsum" || norm == "elementalresistsum") {
            outKind = StatKind::TotalResist;
            return true;
        }
        if (norm == "anyresist" || norm == "anyres" || norm == "maxsingleresist") {
            outKind = StatKind::AnyResist;
            return true;
        }
        if (norm == "enhanceddamage" || norm == "ed" || norm == "edmg") {
            outKind = StatKind::EnhancedDamage;
            return true;
        }

        struct AliasEntry {
            std::string_view normName;
            std::uint16_t id;
        };
        static constexpr AliasEntry kBuiltinAliases[] = {
            {"strength", 0}, {"str", 0},
            {"energy", 1}, {"ene", 1},
            {"dexterity", 2}, {"dex", 2},
            {"vitality", 3}, {"vit", 3},
            {"hitpoints", 6},
            {"maxhp", 7}, {"maxlife", 7}, {"life", 7}, {"hp", 7},
            {"mana", 8},
            {"maxmana", 9}, {"maxmp", 9}, {"mp", 9},
            {"stamina", 10},
            {"maxstamina", 11},
            {"gold", 14},
            {"itemarmorpercent", 16}, {"enhanceddefense", 16}, {"edef", 16},
            {"itemmaxdamagepercent", 17},
            {"itemmindamagepercent", 18},
            {"tohit", 19}, {"attackrating", 19}, {"ar", 19},
            {"toblock", 20}, {"increasedchanceofblocking", 20}, {"blockchance", 20},
            {"mindamage", 21}, {"mindmg", 21},
            {"maxdamage", 22}, {"maxdmg", 22},
            {"secondarymindamage", 23},
            {"secondarymaxdamage", 24},
            {"damagepercent", 25},
            {"manarecoverybonus", 27}, {"regeneratemana", 27}, {"manaregen", 27},
            {"staminarecoverybonus", 28},
            {"armorclass", 31}, {"defense", 31}, {"def", 31},
            {"armorclassvsmissile", 32},
            {"armorclassvshth", 33},
            {"normaldamagereduction", 34}, {"damagereduced", 34}, {"pdr", 34},
            {"magicdamagereduction", 35}, {"magicdamagereduced", 35}, {"mdr", 35},
            {"damageresist", 36}, {"physicalresist", 36}, {"damagereducedpercent", 36}, {"pdrpercent", 36}, {"drpercent", 36},
            {"magicresist", 37}, {"magicres", 37}, {"mres", 37},
            {"maxmagicresist", 38},
            {"fireresist", 39}, {"fireres", 39}, {"fres", 39},
            {"maxfireresist", 40},
            {"lightresist", 41}, {"lightningresist", 41}, {"lightres", 41}, {"lres", 41},
            {"maxlightresist", 42}, {"maxlightningresist", 42},
            {"coldresist", 43}, {"coldres", 43}, {"cres", 43},
            {"maxcoldresist", 44},
            {"poisonresist", 45}, {"poisonres", 45}, {"pres", 45},
            {"maxpoisonresist", 46},
            {"lifedrainmindam", 60}, {"lifesteal", 60}, {"ll", 60},
            {"manadrainmindam", 62}, {"manasteal", 62}, {"ml", 62},
            {"quantity", 70},
            {"value", 71},
            {"durability", 72},
            {"maxdurability", 73},
            {"hpregen", 74}, {"replenishlife", 74}, {"replife", 74},
            {"itemmaxdurabilitypercent", 75},
            {"itemmaxhppercent", 76}, {"maxlifepercent", 76},
            {"itemmaxmanapercent", 77}, {"maxmanapercent", 77},
            {"itemattackertakesdamage", 78}, {"thorns", 78},
            {"itemgoldbonus", 79}, {"goldfind", 79}, {"gf", 79}, {"extragold", 79},
            {"itemmagicbonus", 80}, {"magicfind", 80}, {"mf", 80},
            {"itemknockback", 81}, {"knockback", 81},
            {"itemaddclassskills", 83}, {"classskills", 83},
            {"itemaddexperience", 85}, {"experiencegained", 85},
            {"itemhealafterkill", 86}, {"lifeafterkill", 86}, {"laek", 86},
            {"itemreducedprices", 87}, {"vendorprices", 87},
            {"itemlightradius", 89}, {"lightradius", 89},
            {"itemreqpercent", 91}, {"requirementspercent", 91}, {"reqpercent", 91},
            {"itemfasterattackrate", 93}, {"increasedattackspeed", 93}, {"ias", 93},
            {"itemfastermovevelocity", 96}, {"fasterrunwalk", 96}, {"frw", 96},
            {"itemnonclassskill", 97}, {"nonclassskill", 97}, {"oskill", 97},
            {"itemfastergethitrate", 99}, {"fasterhitrecovery", 99}, {"fhr", 99},
            {"itemfasterblockrate", 102}, {"fasterblockrate", 102}, {"fbr", 102},
            {"itemfastercastrate", 105}, {"fastercastrate", 105}, {"fcr", 105},
            {"itemsingleskill", 107}, {"singleskill", 107}, {"skill", 107}, {"staffmod", 107},
            {"itemrestinpeace", 108}, {"restinpeace", 108},
            {"itempoisonlengthresist", 110}, {"poisonlengthreduced", 110}, {"plr", 110},
            {"itemnormaldamage", 111}, {"flatdamage", 111},
            {"itemdamagetomana", 114}, {"damagetomana", 114}, {"dtm", 114},
            {"itemignoretargetac", 115}, {"ignoretargetdefense", 115}, {"itd", 115},
            {"itemfractionaltargetac", 116}, {"targetdefense", 116},
            {"itempreventheal", 117}, {"preventmonsterheal", 117}, {"pmh", 117},
            {"itemhalffreezeduration", 118}, {"halffreezeduration", 118},
            {"itemtohitpercent", 119}, {"attackratingpercent", 119}, {"arpercent", 119},
            {"itemdemondamagepercent", 121}, {"damagetodemons", 121},
            {"itemundeaddamagepercent", 122}, {"damagetoundead", 122},
            {"itemelemskill", 126}, {"elemskill", 126}, {"fireskills", 126},
            {"itemallskills", 127}, {"allskills", 127},
            {"itemopenwounds", 135}, {"openwounds", 135}, {"ow", 135},
            {"itemcrushingblow", 136}, {"crushingblow", 136}, {"cb", 136},
            {"itemmanaafterkill", 138}, {"manaafterkill", 138}, {"maek", 138},
            {"itemdeadlystrike", 141}, {"deadlystrike", 141}, {"ds", 141},
            {"itemabsorbfirepercent", 142},
            {"itemabsorbfire", 143}, {"fireabsorb", 143},
            {"itemabsorblightpercent", 144},
            {"itemabsorblight", 145}, {"lightningabsorb", 145}, {"lightabsorb", 145},
            {"itemabsorbmagicpercent", 146},
            {"itemabsorbmagic", 147}, {"magicabsorb", 147},
            {"itemabsorbcoldpercent", 148},
            {"itemabsorbcold", 149}, {"coldabsorb", 149},
            {"itemslow", 150}, {"slowstarget", 150},
            {"itemaura", 151}, {"aura", 151},
            {"itemindesctructible", 152}, {"indestructible", 152},
            {"itemcannotbefrozen", 153}, {"cannotbefrozen", 153}, {"cbf", 153},
            {"itempierce", 156}, {"piercingattack", 156},
            {"itemaddskilltab", 188}, {"skilltab", 188}, {"tabskills", 188},
            {"itemnumsockets", 194}, {"sockets", 194}, {"numsockets", 194},
            {"passivefiremastery", 329}, {"fireskilldamage", 329},
            {"passiveltngmastery", 330}, {"lightskilldamage", 330}, {"lightningskilldamage", 330},
            {"passivecoldmastery", 331}, {"coldskilldamage", 331},
            {"passivepoismastery", 332}, {"poisonskilldamage", 332},
            {"passivefirepierce", 333}, {"piercefire", 333},
            {"passiveltngpierce", 334}, {"piercelight", 334}, {"piercelightning", 334},
            {"passivecoldpierce", 335}, {"piercecold", 335},
            {"passivepoispierce", 336}, {"piercepoison", 336},
        };

        for (const auto& entry : kBuiltinAliases) {
            if (entry.normName == norm) {
                outStatId = entry.id;
                return true;
            }
        }

        if (const auto it = byExact.find(std::string(token)); it != byExact.end()) {
            outStatId = it->second;
            return true;
        }
        if (const auto it = byNormalized.find(norm); it != byNormalized.end()) {
            outStatId = it->second;
            return true;
        }
        return ParseUnsigned16(token, 1023U, outStatId);
    }
};

inline bool LoadStatCosts(const fs::path& excel, StatCostCatalog& out, std::string& error) {
    StatCostCatalog staged{};
    const auto path = excel / L"itemstatcost.txt";
    std::error_code ec;
    if (!fs::is_regular_file(path, ec) || ec) {
        out = std::move(staged);
        return true;
    }
    std::vector<std::string> lines;
    std::vector<std::string_view> headings;
    if (!ReadTsvLines(path, lines, headings, error)) return false;
    const auto statCol = ColumnIndex(headings, "Stat");
    auto idCol = ColumnIndex(headings, "*ID");
    if (idCol == headings.size()) idCol = ColumnIndex(headings, "ID");
    if (statCol == headings.size() || idCol == headings.size()) {
        error = "itemstatcost-requires-Stat-and-ID-columns";
        return false;
    }
    for (std::size_t j = 1; j < lines.size(); ++j) {
        const auto cells = SplitTsv(lines[j]);
        if (statCol >= cells.size() || idCol >= cells.size()) continue;
        const auto name = cells[statCol];
        const auto idStr = cells[idCol];
        if (name.empty() || idStr.empty()) continue;
        std::uint16_t statId = 0;
        if (!ParseUnsigned16(idStr, 1023U, statId)) continue;
        staged.byExact.try_emplace(std::string(name), statId);
        const auto norm = NormalizeIdentifier(name);
        if (!norm.empty()) staged.byNormalized.try_emplace(norm, statId);
        ++staged.rows;
    }
    out = std::move(staged);
    return true;
}

struct SkillCatalog {
    std::unordered_map<std::string, std::uint16_t> byExact;
    std::unordered_map<std::string, std::uint16_t> byNormalized;
    std::size_t rows{};

    [[nodiscard]] bool Resolve(std::string_view token, std::uint16_t& outSkillId) const {
        outSkillId = 0;
        if (ParseUnsigned16(token, 1023U, outSkillId)) return true;
        if (const auto it = byExact.find(std::string(token)); it != byExact.end()) {
            outSkillId = it->second;
            return true;
        }
        const auto norm = NormalizeIdentifier(token);
        if (norm.empty()) return false;
        if (const auto it = byNormalized.find(norm); it != byNormalized.end()) {
            outSkillId = it->second;
            return true;
        }

        struct BuiltinSkill {
            std::string_view normName;
            std::uint16_t id;
        };
        static constexpr BuiltinSkill kBuiltinSkills[] = {
            {"magicarrow", 6}, {"firearrow", 7}, {"innersight", 8}, {"criticalstrike", 9},
            {"jab", 10}, {"coldarrow", 11}, {"multipleshot", 12}, {"dodge", 13},
            {"poisonjavelin", 15}, {"explodingarrow", 16}, {"slowmissiles", 17}, {"avoid", 18},
            {"impale", 19}, {"lightningbolt", 20}, {"freezingarrow", 21}, {"guidedarrow", 22},
            {"penetrate", 23}, {"chargedstrike", 24}, {"plaguejavelin", 25}, {"strafe", 26},
            {"immolationarrow", 27}, {"evade", 29}, {"fend", 30}, {"valkyrie", 32},
            {"pierce", 33}, {"lightningstrike", 34}, {"lightningfury", 35},
            {"firebolt", 36}, {"warmth", 37}, {"chargedbolt", 38}, {"icebolt", 39},
            {"frozenarmor", 40}, {"staticfield", 42}, {"telekinesis", 43}, {"frostnova", 44},
            {"iceblast", 45}, {"blaze", 46}, {"fireball", 47}, {"nova", 48},
            {"lightning", 49}, {"shiverarmor", 50}, {"firewall", 51}, {"enchant", 52},
            {"chainlightning", 53}, {"teleport", 54}, {"glacialspike", 55}, {"meteor", 56},
            {"thunderstorm", 57}, {"energyshield", 58}, {"blizzard", 59}, {"chillingarmor", 60},
            {"firemastery", 61}, {"hydra", 62}, {"lightningmastery", 63}, {"frozenorb", 64},
            {"coldmastery", 65},
            {"amplifydamage", 66}, {"teeth", 67}, {"bonearmor", 68}, {"skeletonmastery", 69},
            {"raiseskeleton", 70}, {"dimvision", 71}, {"weaken", 72}, {"corpseexplosion", 74},
            {"claygolem", 75}, {"ironmaiden", 76}, {"bonewall", 78}, {"lifetap", 82},
            {"bonespear", 84}, {"decrepify", 87}, {"lowerresist", 91}, {"poisonnova", 92},
            {"bonespirit", 93}, {"revive", 95},
            {"smite", 97}, {"might", 98}, {"holyfire", 101}, {"zeal", 106},
            {"charge", 107}, {"vengeance", 111}, {"blessedhammer", 112}, {"concentration", 113},
            {"holyfreeze", 114}, {"vigor", 115}, {"holyshield", 117}, {"holyshock", 118},
            {"sanctuary", 119}, {"meditation", 120}, {"fistoftheheavens", 121}, {"foh", 121},
            {"fanaticism", 122}, {"conviction", 123}, {"redemption", 124}, {"salvation", 125},
            {"findpotion", 131}, {"howl", 130}, {"shout", 138}, {"finditem", 142},
            {"taunt", 137}, {"ironskin", 145}, {"battlecry", 146},
            {"frenzy", 147}, {"naturalresistance", 153}, {"battleorders", 149}, {"bo", 149},
            {"grimward", 150}, {"whirlwind", 151}, {"berserk", 152}, {"warcry", 154},
            {"battlecommand", 155},
            {"werewolf", 223}, {"lycanthropy", 224}, {"firestorm", 225}, {"oaksage", 226},
            {"werebear", 228}, {"fissure", 234}, {"fury", 248}, {"tornado", 245},
            {"grizzly", 247}, {"armageddon", 249}, {"hurricane", 250},
            {"tigerstrike", 254}, {"dragontalon", 255}, {"fistsoffire", 259},
            {"burstofspeed", 258}, {"wakeoffire", 262}, {"weaponblock", 263}, {"fade", 267},
            {"shadowwarrior", 268}, {"clawsofthunder", 269}, {"lightningsentry", 271},
            {"mindblast", 273}, {"bladesofice", 274}, {"dragonflight", 275}, {"deathsentry", 276},
            {"venom", 278}, {"shadowmaster", 279}, {"phoenixstrike", 280},
        };
        for (const auto& entry : kBuiltinSkills) {
            if (entry.normName == norm) {
                outSkillId = entry.id;
                return true;
            }
        }
        return false;
    }
};

inline bool LoadSkills(const fs::path& excel, SkillCatalog& out, std::string& error) {
    SkillCatalog staged{};
    const auto path = excel / L"skills.txt";
    std::error_code ec;
    if (!fs::is_regular_file(path, ec) || ec) {
        out = std::move(staged);
        return true;
    }
    std::vector<std::string> lines;
    std::vector<std::string_view> headings;
    if (!ReadTsvLines(path, lines, headings, error)) return false;
    const auto skillCol = ColumnIndex(headings, "skill");
    auto idCol = ColumnIndex(headings, "*Id");
    if (idCol == headings.size()) idCol = ColumnIndex(headings, "Id");
    if (idCol == headings.size()) idCol = ColumnIndex(headings, "*ID");
    if (skillCol == headings.size() || idCol == headings.size()) {
        error = "skills-requires-skill-and-Id-columns";
        return false;
    }
    for (std::size_t j = 1; j < lines.size(); ++j) {
        const auto cells = SplitTsv(lines[j]);
        if (skillCol >= cells.size() || idCol >= cells.size()) continue;
        const auto name = cells[skillCol];
        const auto idStr = cells[idCol];
        if (name.empty() || idStr.empty()) continue;
        std::uint16_t skillId = 0;
        if (!ParseUnsigned16(idStr, 1023U, skillId)) continue;
        staged.byExact.try_emplace(std::string(name), skillId);
        const auto norm = NormalizeIdentifier(name);
        if (!norm.empty()) staged.byNormalized.try_emplace(norm, skillId);
        ++staged.rows;
    }
    out = std::move(staged);
    return true;
}

[[nodiscard]] inline bool ResolveClassIdentifier(std::string_view token,
                                                 std::uint16_t& outClassId) noexcept {
    if (ParseUnsigned16(token, 7U, outClassId)) return true;
    const auto norm = NormalizeIdentifier(token);
    if (norm == "amazon" || norm == "ama" || norm == "zon") { outClassId = 0; return true; }
    if (norm == "sorceress" || norm == "sor" || norm == "sorc") { outClassId = 1; return true; }
    if (norm == "necromancer" || norm == "nec" || norm == "necro") { outClassId = 2; return true; }
    if (norm == "paladin" || norm == "pal" || norm == "pally") { outClassId = 3; return true; }
    if (norm == "barbarian" || norm == "bar" || norm == "barb") { outClassId = 4; return true; }
    if (norm == "druid" || norm == "dru") { outClassId = 5; return true; }
    if (norm == "assassin" || norm == "ass" || norm == "asn" || norm == "sin") { outClassId = 6; return true; }
    if (norm == "warlock" || norm == "war") { outClassId = 7; return true; }
    return false;
}

[[nodiscard]] inline bool ResolveSkillTabIdentifier(
    std::string_view token, std::vector<std::uint16_t>& outTabIds) {
    std::uint16_t num = 0;
    if (ParseUnsigned16(token, 63U, num)) {
        outTabIds.push_back(num);
        return true;
    }
    const auto norm = NormalizeIdentifier(token);
    auto pushTab = [&](std::uint16_t cls, std::uint16_t tree) {
        outTabIds.push_back(static_cast<std::uint16_t>(cls * 8U + tree));
        outTabIds.push_back(static_cast<std::uint16_t>(cls * 3U + tree));
    };
    if (norm == "bow" || norm == "bowandcrossbow" || norm == "bowandcrossbowskills") { pushTab(0, 0); return true; }
    if (norm == "passive" || norm == "passiveandmagic" || norm == "passiveandmagicskills") { pushTab(0, 1); return true; }
    if (norm == "javelin" || norm == "jav" || norm == "javelinandspear" || norm == "javelinandspearskills") { pushTab(0, 2); return true; }
    if (norm == "firespells" || norm == "sorcfire" || norm == "firetab") { pushTab(1, 0); return true; }
    if (norm == "lightningspells" || norm == "sorclightning" || norm == "sorclight" || norm == "lightningtab") { pushTab(1, 1); return true; }
    if (norm == "coldspells" || norm == "sorccold" || norm == "coldtab") { pushTab(1, 2); return true; }
    if (norm == "curses") { pushTab(2, 0); return true; }
    if (norm == "poisonandbone" || norm == "poisonandbonespells" || norm == "pnb") { pushTab(2, 1); return true; }
    if (norm == "necrosummoning" || norm == "necromancersummoning") { pushTab(2, 2); return true; }
    if (norm == "paladincombat" || norm == "paladincombatskills" || norm == "pcomb") { pushTab(3, 0); return true; }
    if (norm == "offensiveauras") { pushTab(3, 1); return true; }
    if (norm == "defensiveauras") { pushTab(3, 2); return true; }
    if (norm == "barbariancombat" || norm == "barbariancombatskills" || norm == "bcomb") { pushTab(4, 0); return true; }
    if (norm == "combatmasteries" || norm == "masteries") { pushTab(4, 1); return true; }
    if (norm == "warcries") { pushTab(4, 2); return true; }
    if (norm == "druidsummoning" || norm == "summoningskillsdruid") { pushTab(5, 0); return true; }
    if (norm == "shapeshifting" || norm == "shapeshiftingskills") { pushTab(5, 1); return true; }
    if (norm == "elemental" || norm == "elementalskills") { pushTab(5, 2); return true; }
    if (norm == "traps") { pushTab(6, 0); return true; }
    if (norm == "shadowdisciplines" || norm == "shadow") { pushTab(6, 1); return true; }
    if (norm == "martialarts") { pushTab(6, 2); return true; }
    return false;
}

} // namespace PyreLootFilter::Tables
