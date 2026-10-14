// Reading the game's own code, at runtime, because there is no other way.
//
// **D2R.exe ships with an encrypted .text.** On disk its byte distribution is
// uniform - Shannon entropy 8.000 bits per byte, 0.39% zeroes, which is 1/256
// and therefore noise - so it disassembles to garbage and contains not one
// RIP-relative reference to any string in .rdata. Every static approach fails,
// and fails silently: a disassembler desynchronises at the first byte and keeps
// printing plausible-looking instructions, and a reference scan simply returns
// nothing.
//
// What is *not* encrypted is .rdata, .pdata and the section table. So:
//
//   - .rdata gives string literals, which is how the chat command schema was
//     read without ever disassembling anything.
//   - .pdata is the exception directory, a sorted table of
//     RUNTIME_FUNCTION { BeginAddress, EndAddress, UnwindData } covering ~105k
//     functions, which turns any address inside a function into that function's
//     exact bounds.
//
// And the code itself is readable in memory, after the loader has decrypted it.
// A plugin runs inside the game process, so it is the one place that analysis
// can happen at all.
//
// This library is the generic half of that: find a string, find the code that
// references it, find the function containing that code, and dump it for
// offline disassembly. It only ever *reads* this process's own memory. Nothing
// here patches or hooks anything.
//
// Dumped bytes go to the log as hex, and are meant to be disassembled somewhere
// else:
//
//   objdump -D -b binary -m i386:x86-64 --adjust-vma=0x<rva> dump.bin
//
// Prior art, and a debt: plugins/hardcore-only/src/image_scan.cpp did all of
// this first, for the character-create checkbox names, and still carries its own
// copy. It should move onto this library.

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace D2RL {
struct PluginContext;
}

namespace D2RLImage {

struct Section {
	std::string name;
	uint8_t*    start = nullptr;
	size_t      size  = 0;
	uint32_t    rva   = 0;
};

// True when a range is committed and readable in this process. Every read below
// is guarded by it: an image is a moving target and a page that was mapped a
// moment ago may not be now.
auto Readable(const void* address, size_t size) noexcept -> bool;

// Reads the section table of a module loaded at `base` - use
// PluginContext::exeBase for the game image.
auto ReadSections(uintptr_t base, std::vector<Section>& sections) noexcept -> bool;

auto FindSection(const std::vector<Section>& sections, const char* name) noexcept -> const Section*;

// Every rva at which the null-terminated `needle` appears in `section`. The
// terminator is required, so this finds whole literals rather than prefixes of
// longer ones.
void FindString(const Section& section, uint32_t base, const char* needle, std::vector<uint32_t>& rvas) noexcept;

// Every rva in `code` holding a `lea reg,[rip+disp32]` that resolves to
// `targetRva`.
//
// The encoding is 48/4C 8D <modrm mod=00 rm=101> disp32 - seven bytes - and the
// address reached is the rva of the following instruction plus the
// displacement. That is enough to find who loads a string without decoding
// anything around it, which matters when the surrounding bytes cannot be
// trusted to disassemble.
void FindLeaReferences(const Section& code, uint32_t targetRva, size_t maxHits, std::vector<uint32_t>& rvas) noexcept;

// Every rva in `code` matching `pattern`, where a zero byte in `mask` makes the
// corresponding pattern byte a wildcard. Useful for finding an instruction shape
// rather than an exact address - `call qword ptr [reg+disp32]`, say, where the
// register is not known in advance.
void FindPattern(const Section& code, const uint8_t* pattern, const uint8_t* mask, size_t length, size_t maxHits, std::vector<uint32_t>& rvas) noexcept;

// The .pdata bounds of the range containing `rva`. False when nothing covers
// it, which means a leaf function without unwind data, or not code at all.
//
// Note this is a *range*, not necessarily a whole function - see
// PrimaryFunctionRange.
auto FunctionRange(const std::vector<Section>& sections, uint32_t rva, uint32_t& begin, uint32_t& end) noexcept -> bool;

// The bounds of the *whole* function containing `rva`, following chained unwind
// info to the primary entry.
//
// A compiler splits a function into several ranges - hot, cold, and outlined
// error paths - and gives each its own .pdata entry. Only the first describes
// the real prologue; the rest set UNW_FLAG_CHAININFO and end with a
// RUNTIME_FUNCTION pointing at their parent. Without following that chain, a
// lookup lands on a 12-byte shard and reports it as a function, which is a very
// convincing wrong answer: it has a plausible size and a real address.
//
// `chainDepth`, when not null, receives how many links were followed - zero
// meaning the address was already in the primary range.
auto PrimaryFunctionRange(const std::vector<Section>& sections, uintptr_t base, uint32_t rva, uint32_t& begin, uint32_t& end, uint32_t* chainDepth = nullptr) noexcept -> bool;

// Follows a `jmp` at the very end of a range back to its target.
//
// MSVC splits a function into hot and cold parts and gives each its own .pdata
// entry, so an error path that logs and returns often lives in a fragment whose
// last instruction jumps back into the parent. That makes the parent findable
// from a string only the error path mentions: find the fragment, read its
// trailing jump, and the target is inside the real function.
//
// Handles `EB rel8` and `E9 rel32`, which is what a tail jump is. False if the
// range does not end in one.
auto FollowTrailingJump(uintptr_t base, uint32_t rangeEnd, uint32_t& target) noexcept -> bool;

// Logs `length` bytes at `rva` as hex, 64 per line. A length of 0 dumps the
// whole enclosing function, which is what makes the output disassemblable.
auto DumpRegion(const D2RL::PluginContext* context, const char* logPrefix, uintptr_t base, uint32_t rva, uint32_t length) noexcept -> bool;

// The whole sequence, for one string: find it in `stringSection`, find every
// reference in `codeSection`, report the enclosing function of each, and
// optionally dump it.
//
// Returns the number of references found.
auto ReportReferences(
	const D2RL::PluginContext* context,
	const char*                logPrefix,
	uintptr_t                  base,
	const char*                needle,
	const char*                stringSectionName,
	const char*                codeSectionName,
	size_t                     maxHits,
	bool                       dumpFunctions) noexcept -> size_t;

}
