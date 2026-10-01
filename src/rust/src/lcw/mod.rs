//! LCW, Westwood's LZ77-style compression, as used by shapes, key frames,
//! WSA animations, VQA movies, CPS pictures and the map and overlay packs.
//!
//! A stream is a sequence of commands, each an opcode byte and its operands:
//!
//! | Opcode              | Meaning                                                      |
//! | ------------------- | ------------------------------------------------------------ |
//! | `0cccdddd dddddddd` | copy `ccc + 3` bytes from `d` bytes back in the output       |
//! | `10cccccc`          | copy `c` literal bytes from the stream; `0x80` ends it       |
//! | `11cccccc pppp`     | copy `c + 3` bytes from output position `p` (`c` < 62)       |
//! | `0xfe cccc v`       | write `c` copies of byte `v`                                 |
//! | `0xff cccc pppp`    | copy `c` bytes from output position `p`                      |
//!
//! Multi-byte operands are little-endian 16-bit values. Copies run forward one
//! byte at a time, so a copy overlapping its own output repeats a pattern.

mod decode;
mod encode;

pub use decode::{uncompress, uncompress_bounded, uncompress_in_place};
pub use encode::{compress, worst_case_size};
