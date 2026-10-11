"""Encode original button PNGs as D2R SpA1 v31 resources for the DLL.

This performs only format conversion and resampling. It never reads a donor
sprite, paints new artwork, or copies an asset from the game installation.
See ../docs/BUTTON-SPRITES.md for the format evidence and validation limits.
"""

from __future__ import annotations

import argparse
import hashlib
import io
import json
from pathlib import Path
import struct
import tempfile

from PIL import Image


STATES = ("normal", "hover", "pressed", "disabled")
HD_SIZE = (144, 136)
LOWEND_SIZE = (72, 68)
HEADER = struct.Struct("<4sHH8I")
RESOURCE_HD = 2101
RESOURCE_LOWEND = 2102


def encode_sprite(frames: list[Image.Image]) -> bytes:
    """Pack equal RGBA frames side by side into the observed uncompressed form."""
    if not frames or len(frames) > 64:
        raise ValueError("a sprite needs 1..64 frames")
    width, height = frames[0].size
    if not (0 < width <= 4096 and 0 < height <= 4096):
        raise ValueError("frame dimensions must be between 1 and 4096")
    if any(frame.mode != "RGBA" or frame.size != (width, height) for frame in frames):
        raise ValueError("all sprite frames must have equal RGBA dimensions")
    strip = Image.new("RGBA", (width * len(frames), height))
    for index, frame in enumerate(frames):
        # No alpha mask: copy all four channels exactly, including translucent pixels.
        strip.paste(frame, (index * width, 0))
    header = HEADER.pack(b"SpA1", 31, width, strip.width, height, 0, len(frames), 0, 0, 0, 0)
    return header + strip.tobytes()


def decode_sprite(data: bytes) -> list[Image.Image]:
    """Strictly decode only the subset emitted by this tool, for round-trip checks."""
    if len(data) < HEADER.size:
        raise ValueError("truncated sprite header")
    magic, version, frame_width, width, height, zero1, count, *zeros = HEADER.unpack_from(data)
    if magic != b"SpA1" or version != 31 or zero1 or any(zeros):
        raise ValueError("unsupported sprite header")
    if not (0 < frame_width <= 4096 and 0 < height <= 4096 and 0 < count <= 64):
        raise ValueError("invalid sprite dimensions or frame count")
    if width != frame_width * count or len(data) != HEADER.size + width * height * 4:
        raise ValueError("sprite dimensions and payload length do not agree")
    strip = Image.frombytes("RGBA", (width, height), data[HEADER.size:])
    return [strip.crop((index * frame_width, 0, (index + 1) * frame_width, height))
            for index in range(count)]


def validate_frames(frames: list[Image.Image], expected_size: tuple[int, int]) -> None:
    if len(frames) != len(STATES) or any(frame.size != expected_size for frame in frames):
        raise ValueError("expected four frames at the requested raster dimensions")
    if len({frame.tobytes() for frame in frames}) != len(STATES):
        raise ValueError("the four button states must have distinct pixels")
    for state, frame in zip(STATES, frames):
        alpha_min, alpha_max = frame.getchannel("A").getextrema()
        if alpha_min != 0 or alpha_max == 0:
            raise ValueError(f"{state} must have transparent background and visible artwork")


def load_art(input_dir: Path) -> list[Image.Image]:
    frames = []
    for state in STATES:
        path = input_dir / f"{state}.png"
        if path.stat().st_size > 16 * 1024 * 1024:
            raise ValueError(f"source image exceeds 16 MiB: {path}")
        with Image.open(path) as image:
            if image.format != "PNG" or image.width != image.height or not (max(HD_SIZE) <= image.width <= 4096):
                raise ValueError(f"expected a square PNG between {max(HD_SIZE)} and 4096 pixels: {path}")
            if "A" not in image.getbands() and "transparency" not in image.info:
                raise ValueError(f"source PNG has no transparency channel: {path}")
            frames.append(image.convert("RGBA"))
    if any(frame.size != frames[0].size for frame in frames):
        raise ValueError("the four source PNGs must have identical canvas dimensions")
    validate_frames(frames, frames[0].size)
    return frames


def write_if_changed(path: Path, data: bytes) -> None:
    if path.exists() and path.read_bytes() == data:
        return
    # The temporary file is on the same volume, so replacing an existing build
    # output cannot expose a half-written resource to the compiler.
    with tempfile.NamedTemporaryFile(dir=path.parent, prefix=path.name + ".", suffix=".tmp", delete=False) as stream:
        temp_path = Path(stream.name)
        stream.write(data)
    try:
        temp_path.replace(path)
    finally:
        temp_path.unlink(missing_ok=True)


def build(input_dir: Path, output_dir: Path) -> dict:
    originals = load_art(input_dir)
    outputs: dict[str, bytes] = {}
    records = []
    for name, size, resource_id in (("deposit-button.sprite", HD_SIZE, RESOURCE_HD),
                                    ("deposit-button.lowend.sprite", LOWEND_SIZE, RESOURCE_LOWEND)):
        frames = [frame.resize(size, Image.Resampling.LANCZOS) for frame in originals]
        validate_frames(frames, size)
        data = encode_sprite(frames)
        decoded = decode_sprite(data)
        if any(actual.tobytes() != expected.tobytes() for actual, expected in zip(decoded, frames)):
            raise ValueError(f"pixel round trip failed for {name}")
        outputs[name] = data
        records.append({"file": name, "resource_id": resource_id,
                        "frame_width": size[0], "frame_height": size[1], "frame_count": len(STATES),
                        "strip_width": size[0] * len(STATES), "bytes": len(data),
                        "sha256": hashlib.sha256(data).hexdigest()})
    manifest = {"format": "SpA1", "version": 31, "channels": "RGBA",
                "logical_width": HD_SIZE[0], "logical_height": HD_SIZE[1],
                "frames": dict(zip(STATES, range(len(STATES)))),
                "inputs": {f"{state}.png": hashlib.sha256((input_dir / f"{state}.png").read_bytes()).hexdigest()
                           for state in STATES},
                "resources": records}
    output_dir.mkdir(parents=True, exist_ok=True)
    for name, data in outputs.items():
        write_if_changed(output_dir / name, data)
    # This preview is decoded from the bytes that will be embedded, so reviewing
    # it verifies the packaged pixels rather than a separate rendering of art.
    preview = Image.new("RGBA", (HD_SIZE[0] * len(STATES), HD_SIZE[1]))
    for index, frame in enumerate(decode_sprite(outputs["deposit-button.sprite"])):
        preview.paste(frame, (HD_SIZE[0] * index, 0))
    preview_bytes = io.BytesIO()
    preview.save(preview_bytes, format="PNG")
    write_if_changed(output_dir / "deposit-button-preview.png", preview_bytes.getvalue())
    # RC resource paths are absolute forward-slash paths. No shell expansion or
    # dependence on the compiler's current working directory is required.
    paths = [(output_dir / name).resolve().as_posix() for name in outputs]
    if any('"' in path or "\n" in path or "\r" in path for path in paths):
        raise ValueError("unsupported character in generated resource path")
    rc = ("// Generated by build_button_sprites.py; do not edit.\n"
          f'{RESOURCE_HD} RCDATA "{paths[0]}"\n'
          f'{RESOURCE_LOWEND} RCDATA "{paths[1]}"\n')
    write_if_changed(output_dir / "button_art.rc", rc.encode("utf-8"))
    write_if_changed(output_dir / "button-sprites-manifest.json", (json.dumps(manifest, indent=2) + "\n").encode())
    return manifest


def self_test() -> None:
    # Asymmetric rectangular pixels exercise independent width/height, row order,
    # channel order, alpha and frame boundaries without depending on the artwork.
    frames = []
    for index in range(4):
        frame = Image.new("RGBA", (2, 3))
        frame.putdata([(index + 1, 12, 23, 255), (34, index + 2, 56, 127),
                       (67, 78, index + 3, 1), (90, 101, 112, 0),
                       (123, index + 4, 145, 64), (156, 167, index + 5, 254)])
        frames.append(frame)
    encoded = encode_sprite(frames)
    if encoded[:40] != struct.pack("<4sHH8I", b"SpA1", 31, 2, 8, 3, 0, 4, 0, 0, 0, 0):
        raise AssertionError("header does not match the independently expected values")
    expected = b"".join(bytes(frames[index].getpixel((x, y))) for y in range(3) for index in range(4) for x in range(2))
    if encoded[40:] != expected:
        raise AssertionError("frame, row or RGBA channel order is wrong")
    if [frame.tobytes() for frame in decode_sprite(encoded)] != [frame.tobytes() for frame in frames]:
        raise AssertionError("synthetic pixel round trip failed")
    validate_frames(frames, (2, 3))
    for bad in (encoded[:39], encoded[:-1], encoded + b"x", b"Bad!" + encoded[4:]):
        try:
            decode_sprite(bad)
        except ValueError:
            pass
        else:
            raise AssertionError("malformed sprite was accepted")
    for bad in ([frames[0]] * 4,
                [Image.new("RGBA", (2, 3), (index, 0, 0, 0)) for index in range(4)],
                [Image.new("RGBA", (2, 3), (index, 0, 0, 255)) for index in range(4)]):
        try:
            validate_frames(bad, (2, 3))
        except ValueError:
            pass
        else:
            raise AssertionError("invalid state artwork was accepted")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input-dir", type=Path, help="directory containing normal/hover/pressed/disabled.png")
    parser.add_argument("--output-dir", type=Path, help="generated build output directory")
    parser.add_argument("--self-test", action="store_true", help="check the codec without writing files")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        print("SpA1 codec checks passed")
    elif args.input_dir is None or args.output_dir is None:
        parser.error("--input-dir and --output-dir are required unless --self-test is used")
    else:
        self_test()
        manifest = build(args.input_dir.resolve(), args.output_dir.resolve())
        print("Built and validated " + ", ".join(resource["file"] for resource in manifest["resources"]))


if __name__ == "__main__":
    main()
