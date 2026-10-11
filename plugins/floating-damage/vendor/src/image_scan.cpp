#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <D2RLImage/image_scan.h>

#include <D2RLPlugin/api.h>

#include <cstdio>
#include <cstring>

namespace D2RLImage {

namespace {

// One RUNTIME_FUNCTION. Declared here rather than using the Windows header's
// version so the unwind field is not a union that varies by architecture.
struct RuntimeFunction {
	uint32_t begin;
	uint32_t end;
	uint32_t unwind;
};

constexpr uint32_t MaxDumpLength = 8'192;
constexpr uint32_t BytesPerLine  = 64;

// A contiguous run of committed, readable bytes inside a section.
struct Span {
	const uint8_t* start = nullptr;
	size_t         size  = 0;
};

// Splits a section into the readable runs it actually consists of.
//
// This exists because the obvious alternative - asking Readable() before every
// read - is a VirtualQuery per byte, and .text in this game is 22 MB. That is tens
// of millions of syscalls per scanned string, which does not merely take a
// while: it stalls the thread the console command is running on, inside a live
// game. VirtualQuery reports the size of the region it describes, so walking
// regions costs a handful of calls for the whole section, and everything after
// that is a plain memory scan.
auto ReadableSpans(const Section& section) noexcept -> std::vector<Span> {
	std::vector<Span> spans;

	const uint8_t* cursor = section.start;
	const uint8_t* end    = section.start + section.size;

	while (cursor < end) {
		MEMORY_BASIC_INFORMATION info {};
		if (VirtualQuery(cursor, &info, sizeof(info)) == 0) {
			break;
		}

		const auto* regionStart = static_cast<const uint8_t*>(info.BaseAddress);
		const auto* regionEnd   = regionStart + info.RegionSize;
		if (regionEnd <= cursor) {
			break;
		}

		constexpr DWORD readable = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
		const bool      usable   = info.State == MEM_COMMIT && (info.Protect & readable) != 0 && (info.Protect & PAGE_GUARD) == 0;

		if (usable) {
			const uint8_t* from = cursor;
			const uint8_t* to   = regionEnd < end ? regionEnd : end;
			spans.push_back(Span { from, static_cast<size_t>(to - from) });
		}

		cursor = regionEnd;
	}
	return spans;
}

void HexDump(const uint8_t* bytes, size_t count, char* out, size_t outSize) noexcept {
	size_t used = 0;
	for (size_t index = 0; index < count && used + 3 < outSize; ++index) {
		const int written = std::snprintf(out + used, outSize - used, "%02X", bytes[index]);
		if (written < 0) {
			break;
		}
		used += static_cast<size_t>(written);
	}
	out[used < outSize ? used : outSize - 1] = '\0';
}

}

auto Readable(const void* address, size_t size) noexcept -> bool {
	MEMORY_BASIC_INFORMATION info {};
	if (VirtualQuery(address, &info, sizeof(info)) == 0 || info.State != MEM_COMMIT) {
		return false;
	}

	constexpr DWORD readable = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
	if ((info.Protect & readable) == 0 || (info.Protect & PAGE_GUARD) != 0) {
		return false;
	}

	const auto* start = static_cast<const uint8_t*>(info.BaseAddress);
	return static_cast<const uint8_t*>(address) + size <= start + info.RegionSize;
}

auto ReadSections(uintptr_t base, std::vector<Section>& sections) noexcept -> bool {
	auto* const image = reinterpret_cast<uint8_t*>(base);
	if (base == 0 || !Readable(image, sizeof(IMAGE_DOS_HEADER))) {
		return false;
	}

	const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(image);
	if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0 || dos->e_lfanew > 0x1000) {
		return false;
	}

	const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(image + dos->e_lfanew);
	if (!Readable(nt, sizeof(IMAGE_NT_HEADERS64)) || nt->Signature != IMAGE_NT_SIGNATURE) {
		return false;
	}

	const auto* header = IMAGE_FIRST_SECTION(nt);
	const auto  count  = nt->FileHeader.NumberOfSections;
	if (!Readable(header, sizeof(IMAGE_SECTION_HEADER) * count)) {
		return false;
	}

	for (WORD index = 0; index < count; ++index, ++header) {
		char name[IMAGE_SIZEOF_SHORT_NAME + 1] {};
		std::memcpy(name, header->Name, IMAGE_SIZEOF_SHORT_NAME);

		Section entry;
		entry.name  = name;
		entry.rva   = header->VirtualAddress;
		entry.start = image + header->VirtualAddress;
		entry.size  = header->Misc.VirtualSize;

		if (entry.size != 0 && Readable(entry.start, 1)) {
			sections.push_back(entry);
		}
	}
	return !sections.empty();
}

auto FindSection(const std::vector<Section>& sections, const char* name) noexcept -> const Section* {
	for (const Section& section : sections) {
		if (section.name == name) {
			return &section;
		}
	}
	return nullptr;
}

void FindString(const Section& section, uint32_t base, const char* needle, std::vector<uint32_t>& rvas) noexcept {
	(void)base;

	const size_t length = std::strlen(needle);
	if (length == 0 || section.size <= length) {
		return;
	}

	for (const Span& span : ReadableSpans(section)) {
		if (span.size <= length) {
			continue;
		}

		// A literal could straddle the end of a span, but the next span starts
		// where this one ends, so stopping short only misses a match spanning a
		// protection boundary - which a linker does not put a string across.
		for (size_t at = 0; at + length + 1 <= span.size; ++at) {
			if (span.start[at] != static_cast<uint8_t>(needle[0])) {
				continue;
			}
			// The terminator is required, so this matches a whole literal rather
			// than the front of a longer one.
			if (std::memcmp(span.start + at, needle, length) == 0 && span.start[at + length] == '\0') {
				rvas.push_back(section.rva + static_cast<uint32_t>(span.start + at - section.start));
			}
		}
	}
}

void FindLeaReferences(const Section& code, uint32_t targetRva, size_t maxHits, std::vector<uint32_t>& rvas) noexcept {
	constexpr size_t InstructionSize = 7;

	if (code.size < InstructionSize) {
		return;
	}

	for (const Span& span : ReadableSpans(code)) {
		if (span.size < InstructionSize) {
			continue;
		}

		for (size_t at = 0; at + InstructionSize <= span.size; ++at) {
			const uint8_t rex   = span.start[at];
			const uint8_t op    = span.start[at + 1];
			const uint8_t modrm = span.start[at + 2];

			// 48/4C 8D /r with mod=00 and rm=101 is lea reg,[rip+disp32]. The
			// REX.R bit in 4C is what makes r8-r15 a separate opcode byte.
			if ((rex != 0x48 && rex != 0x4C) || op != 0x8D || (modrm & 0xC7) != 0x05) {
				continue;
			}

			int32_t displacement = 0;
			std::memcpy(&displacement, span.start + at + 3, sizeof(displacement));

			const auto site = static_cast<uint32_t>(code.rva + (span.start + at - code.start));

			// The displacement is relative to the instruction *after* this one.
			const auto next    = site + static_cast<uint32_t>(InstructionSize);
			const auto reached = static_cast<uint32_t>(static_cast<int64_t>(next) + displacement);
			if (reached != targetRva) {
				continue;
			}

			rvas.push_back(site);
			if (rvas.size() >= maxHits) {
				return;
			}
		}
	}
}

void FindPattern(const Section& code, const uint8_t* pattern, const uint8_t* mask, size_t length, size_t maxHits, std::vector<uint32_t>& rvas) noexcept {
	if (pattern == nullptr || mask == nullptr || length == 0 || code.size < length) {
		return;
	}

	for (const Span& span : ReadableSpans(code)) {
		if (span.size < length) {
			continue;
		}

		for (size_t at = 0; at + length <= span.size; ++at) {
			bool matched = true;
			for (size_t index = 0; index < length; ++index) {
				if (mask[index] != 0 && span.start[at + index] != pattern[index]) {
					matched = false;
					break;
				}
			}
			if (!matched) {
				continue;
			}

			rvas.push_back(static_cast<uint32_t>(code.rva + (span.start + at - code.start)));
			if (rvas.size() >= maxHits) {
				return;
			}
		}
	}
}

auto FollowTrailingJump(uintptr_t base, uint32_t rangeEnd, uint32_t& target) noexcept -> bool {
	if (base == 0 || rangeEnd < 5) {
		return false;
	}

	// EB rel8 - two bytes, so it starts two before the end.
	const auto* shortJump = reinterpret_cast<const uint8_t*>(base + rangeEnd - 2);
	if (Readable(shortJump, 2) && shortJump[0] == 0xEB) {
		const auto displacement = static_cast<int8_t>(shortJump[1]);
		target                  = static_cast<uint32_t>(static_cast<int64_t>(rangeEnd) + displacement);
		return true;
	}

	// E9 rel32 - five bytes.
	const auto* nearJump = reinterpret_cast<const uint8_t*>(base + rangeEnd - 5);
	if (Readable(nearJump, 5) && nearJump[0] == 0xE9) {
		int32_t displacement = 0;
		std::memcpy(&displacement, nearJump + 1, sizeof(displacement));
		target = static_cast<uint32_t>(static_cast<int64_t>(rangeEnd) + displacement);
		return true;
	}

	return false;
}

auto FunctionRange(const std::vector<Section>& sections, uint32_t rva, uint32_t& begin, uint32_t& end) noexcept -> bool {
	const Section* pdata = FindSection(sections, ".pdata");
	if (pdata == nullptr) {
		return false;
	}

	const size_t count   = pdata->size / sizeof(RuntimeFunction);
	const auto*  entries = reinterpret_cast<const RuntimeFunction*>(pdata->start);
	if (count == 0 || !Readable(entries, count * sizeof(RuntimeFunction))) {
		return false;
	}

	// .pdata is sorted by BeginAddress, so this is a binary search.
	size_t low  = 0;
	size_t high = count;
	while (low < high) {
		const size_t middle = low + (high - low) / 2;
		if (rva < entries[middle].begin) {
			high = middle;
		} else if (rva >= entries[middle].end) {
			low = middle + 1;
		} else {
			begin = entries[middle].begin;
			end   = entries[middle].end;
			return true;
		}
	}
	return false;
}

auto PrimaryFunctionRange(const std::vector<Section>& sections, uintptr_t base, uint32_t rva, uint32_t& begin, uint32_t& end, uint32_t* chainDepth) noexcept -> bool {
	if (chainDepth != nullptr) {
		*chainDepth = 0;
	}
	if (!FunctionRange(sections, rva, begin, end)) {
		return false;
	}

	// UNWIND_INFO:
	//   byte 0  version (bits 0-2) and flags (bits 3-7)
	//   byte 1  size of prolog
	//   byte 2  count of unwind codes
	//   byte 3  frame register and offset
	//   then CountOfCodes 2-byte codes, padded to an even count,
	//   then, if UNW_FLAG_CHAININFO, the parent RUNTIME_FUNCTION.
	constexpr uint8_t  ChainInfoFlag = 0x4;
	constexpr uint32_t MaxLinks      = 8;

	for (uint32_t link = 0; link < MaxLinks; ++link) {
		// Re-find the entry to read its unwind pointer, which FunctionRange
		// does not return.
		const Section* pdata = FindSection(sections, ".pdata");
		if (pdata == nullptr) {
			return true;
		}

		const size_t count   = pdata->size / sizeof(RuntimeFunction);
		const auto*  entries = reinterpret_cast<const RuntimeFunction*>(pdata->start);
		if (count == 0 || !Readable(entries, count * sizeof(RuntimeFunction))) {
			return true;
		}

		const RuntimeFunction* entry = nullptr;
		size_t                 low   = 0;
		size_t                 high  = count;
		while (low < high) {
			const size_t middle = low + (high - low) / 2;
			if (begin < entries[middle].begin) {
				high = middle;
			} else if (begin >= entries[middle].end) {
				low = middle + 1;
			} else {
				entry = &entries[middle];
				break;
			}
		}
		if (entry == nullptr) {
			return true;
		}

		const auto* unwind = reinterpret_cast<const uint8_t*>(base + entry->unwind);
		if (!Readable(unwind, 4)) {
			return true;
		}
		// Version is the low three bits, flags the top five.
		const uint8_t flags = static_cast<uint8_t>(unwind[0] >> 3);
		if ((flags & ChainInfoFlag) == 0) {
			// Not chained: this is the primary entry, so `begin` is the real
			// function start.
			return true;
		}

		const uint8_t  codes  = unwind[2];
		const uint32_t padded = (codes + 1U) & ~1U;
		const auto*    parent = reinterpret_cast<const RuntimeFunction*>(unwind + 4 + padded * 2);
		if (!Readable(parent, sizeof(RuntimeFunction))) {
			return true;
		}

		// A chain that does not move is a malformed table, not a loop worth
		// following.
		if (parent->begin == begin || parent->begin >= parent->end) {
			return true;
		}

		begin = parent->begin;
		end   = parent->end;
		if (chainDepth != nullptr) {
			*chainDepth = link + 1;
		}
	}
	return true;
}

auto DumpRegion(const D2RL::PluginContext* context, const char* logPrefix, uintptr_t base, uint32_t rva, uint32_t length) noexcept -> bool {
	if (context == nullptr || base == 0) {
		return false;
	}

	std::vector<Section> sections;
	if (!ReadSections(base, sections)) {
		D2RL::LogErrorF(context, "%simage headers could not be read.", logPrefix);
		return false;
	}

	uint32_t begin = 0;
	uint32_t end   = 0;
	if (FunctionRange(sections, rva, begin, end)) {
		D2RL::LogInfoF(context, "%srva 0x%08X is in function 0x%08X..0x%08X (0x%X bytes)", logPrefix, rva, begin, end, end - begin);
		if (length == 0) {
			rva    = begin;
			length = end - begin;
		}
	} else {
		D2RL::LogWarnF(context, "%sno .pdata entry covers rva 0x%08X.", logPrefix, rva);
		if (length == 0) {
			length = 256;
		}
	}

	if (length > MaxDumpLength) {
		D2RL::LogWarnF(context, "%sclamping length 0x%X to 0x%X.", logPrefix, length, MaxDumpLength);
		length = MaxDumpLength;
	}

	const auto* start = reinterpret_cast<const uint8_t*>(base + rva);
	if (!Readable(start, 1)) {
		D2RL::LogErrorF(context, "%srva 0x%08X is not readable.", logPrefix, rva);
		return false;
	}

	char line[BytesPerLine * 2 + 4] {};
	for (uint32_t offset = 0; offset < length; offset += BytesPerLine) {
		const uint32_t take = (length - offset) < BytesPerLine ? (length - offset) : BytesPerLine;
		if (!Readable(start + offset, take)) {
			D2RL::LogWarnF(context, "%srva 0x%08X became unreadable.", logPrefix, rva + offset);
			break;
		}
		HexDump(start + offset, take, line, sizeof(line));
		D2RL::LogInfoF(context, "%sdump %08X %s", logPrefix, rva + offset, line);
	}
	return true;
}

auto ReportReferences(
	const D2RL::PluginContext* context,
	const char*                logPrefix,
	uintptr_t                  base,
	const char*                needle,
	const char*                stringSectionName,
	const char*                codeSectionName,
	size_t                     maxHits,
	bool                       dumpFunctions) noexcept -> size_t {
	if (context == nullptr || base == 0 || needle == nullptr) {
		return 0;
	}

	std::vector<Section> sections;
	if (!ReadSections(base, sections)) {
		D2RL::LogErrorF(context, "%simage headers could not be read.", logPrefix);
		return 0;
	}

	const Section* strings = FindSection(sections, stringSectionName);
	const Section* code    = FindSection(sections, codeSectionName);
	if (strings == nullptr || code == nullptr) {
		D2RL::LogErrorF(context, "%s%s or %s is missing from the image.", logPrefix, stringSectionName, codeSectionName);
		return 0;
	}

	std::vector<uint32_t> found;
	FindString(*strings, strings->rva, needle, found);
	if (found.empty()) {
		D2RL::LogWarnF(context, "%s\"%s\" is not in %s.", logPrefix, needle, stringSectionName);
		return 0;
	}

	size_t total = 0;
	for (const uint32_t stringRva : found) {
		D2RL::LogInfoF(context, "%s\"%s\" at rva 0x%08X", logPrefix, needle, stringRva);

		std::vector<uint32_t> references;
		FindLeaReferences(*code, stringRva, maxHits, references);
		if (references.empty()) {
			D2RL::LogWarnF(context, "%s  no lea reaches it. If this is D2R.exe, the code may not be decrypted yet.", logPrefix);
			continue;
		}

		for (const uint32_t site : references) {
			uint32_t begin = 0;
			uint32_t end   = 0;
			if (FunctionRange(sections, site, begin, end)) {
				D2RL::LogInfoF(context, "%s  lea at 0x%08X in function 0x%08X..0x%08X (0x%X bytes)", logPrefix, site, begin, end, end - begin);
				if (dumpFunctions) {
					(void)DumpRegion(context, logPrefix, base, begin, end - begin);
					// Error strings often live in a cold shard. Include the
					// primary range as well so the dump contains its entry point.
					uint32_t primaryBegin = 0;
					uint32_t primaryEnd = 0;
					if (PrimaryFunctionRange(sections, base, site, primaryBegin, primaryEnd)
						&& primaryBegin != begin) {
						D2RL::LogInfoF(context, "%s  primary range 0x%08X..0x%08X", logPrefix, primaryBegin, primaryEnd);
						(void)DumpRegion(context, logPrefix, base, primaryBegin, primaryEnd - primaryBegin);
					}
				}
			} else {
				D2RL::LogInfoF(context, "%s  lea at 0x%08X, no .pdata entry", logPrefix, site);
			}
			++total;
		}
	}
	return total;
}

}
