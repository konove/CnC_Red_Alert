// File: Validation shared by the block-based compression pipes and straws.

#ifndef CNC_RED_ALERT_TECH_CODEC_BLOCK_H_
#define CNC_RED_ALERT_TECH_CODEC_BLOCK_H_

// Returns whether a block header read from a compressed stream describes a
// block that fits buffers of `capacity` bytes. Encoders never emit empty
// blocks, so a zero count also marks the stream as corrupt.
constexpr bool BlockHeaderFits(int comp_count, int uncomp_count,
                               int capacity) {
  return comp_count > 0 && comp_count <= capacity && uncomp_count > 0 &&
         uncomp_count <= capacity;
}

#endif  // CNC_RED_ALERT_TECH_CODEC_BLOCK_H_
