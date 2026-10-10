// The C++ Blowfish engine that engine_rs's blowfish module replaced
// (src/rust/src/blowfish), kept so that a differential test and kernel_bench
// can hold the Rust one to it.

#ifndef CNC_RED_ALERT_ENGINE_CRYPTO_BLOWFISH_REFERENCE_TEST_UTIL_H_
#define CNC_RED_ALERT_ENGINE_CRYPTO_BLOWFISH_REFERENCE_TEST_UTIL_H_

#include <climits>
#include <cstddef>
#include <cstdint>
#include <span>

namespace blowfish_reference {

// As ::BlowfishEngine (engine/crypto/blowfish.h).
class BlowfishEngine {
 public:
  BlowfishEngine() = default;
  ~BlowfishEngine();

  BlowfishEngine(const BlowfishEngine&) = delete;
  BlowfishEngine& operator=(const BlowfishEngine&) = delete;
  BlowfishEngine(BlowfishEngine&&) = delete;
  BlowfishEngine& operator=(BlowfishEngine&&) = delete;

  void Submit_Key(std::span<const std::byte> key);

  // Encrypts (decrypts) all bytes from the source buffer into the
  // destination buffer and returns how many bytes were processed. Only whole
  // 8-byte blocks are transformed; a trailing partial block is copied as is.
  // The destination must be at least as large as the source. To work in place,
  // pass the same buffer as both arguments. Without a key the data is copied.
  int Encrypt(std::span<const std::byte> plaintext,
              std::span<std::byte> cyphertext);
  int Decrypt(std::span<const std::byte> cyphertext,
              std::span<std::byte> plaintext);

  /*
  **	This is the maximum key length supported.
  */
  static constexpr int kMaxKeyLength = 56;

 private:
  bool IsKeyed = false;

  void Sub_Key_Encrypt(uint32_t& left, uint32_t& right);

  void Process_Block(std::span<const std::byte> plaintext,
                     std::span<std::byte> cyphertext,
                     std::span<const uint32_t> ptable);
  void Initialize_Tables();

  static constexpr int kRounds = 16;
  static constexpr int kBytesPerBlock =
      8;  // Feistal round count (16 is standard).

  /*
  **	Initialization data for sub keys. The initial values are constant and
  **	filled with a number generated from pi. Thus they are not random but
  **	they don't hold a weak pattern either.
  */
  static const uint32_t P_Init[static_cast<int>(kRounds) + 2];
  static const uint32_t S_Init[4][UCHAR_MAX + 1];

  /*
  **	Permutation tables for encryption and decryption.
  */
  uint32_t P_Encrypt[static_cast<int>(kRounds) + 2]{};
  uint32_t P_Decrypt[static_cast<int>(kRounds) + 2]{};

  /*
  **	S-Box tables (four).
  */
  uint32_t bf_S[4][UCHAR_MAX + 1]{};
};

}  // namespace blowfish_reference

#endif  // CNC_RED_ALERT_ENGINE_CRYPTO_BLOWFISH_REFERENCE_TEST_UTIL_H_
