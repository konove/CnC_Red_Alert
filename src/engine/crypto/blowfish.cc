/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* $Header: /CounterStrike/BLOWFISH.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : BLOWFISH.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 04/14/96 *
 *                                                                                             *
 *                  Last Update : July 8, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * BlowfishEngine::Decrypt -- Decrypts data using blowfish
 *algorithm.                        * BlowfishEngine::Encrypt -- Encrypt an
 *arbitrary block of data.                            *
 *   BlowfishEngine::Process_Block -- Process a block of data using Blowfish
 *algorithm.        * BlowfishEngine::Sub_Key_Encrypt -- Encrypts a block for
 *use in S-Box processing.          * BlowfishEngine::Submit_Key -- Submit a key
 *that will allow data processing.               *
 *   BlowfishEngine::~BlowfishEngine -- Destructor for the Blowfish engine. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "engine/crypto/blowfish.h"

#include <cstddef>
#include <memory>
#include <span>

#include "absl/log/check.h"
#include "base/buffer.h"
#include "engine/stream/rust_bytes.h"
#include "engine_rs_cxx/lib.h"
#include "rust/cxx.h"

// The cipher is engine_rs's blowfish module (src/rust/src/blowfish); this
// class holds the key it expanded and adds the engine's behavior without one.

namespace {
constexpr int kBytesPerBlock = 8;
}  // namespace

struct BlowfishEngine::Key {
  rust::Box<engine_rs::Blowfish> cipher;
};

BlowfishEngine::BlowfishEngine() = default;

// Dropping the key overwrites its tables.
BlowfishEngine::~BlowfishEngine() = default;

void BlowfishEngine::Submit_Key(const std::span<const std::byte> key) {
  CHECK_LE(key.size(), std::size_t{kMaxKeyLength});
  if (key.empty()) {
    key_ = nullptr;
  } else if (key_ != nullptr) {
    // Re-keyed where they are, the tables need no new allocation and no wipe.
    engine_rs::blowfish_set_key(*key_->cipher, AsRustSlice(key));
  } else {
    key_ = std::make_unique<Key>(engine_rs::blowfish_new(AsRustSlice(key)));
  }
}

// Encrypt() and Decrypt() could be const now that the tables are behind a
// pointer, and would then have to be [[nodiscard]]; they keep the signatures
// their callers were written against.
// NOLINTNEXTLINE(readability-make-member-function-const)
int BlowfishEngine::Encrypt(const std::span<const std::byte> plaintext,
                            const std::span<std::byte> cyphertext) {
  return Process(plaintext, cyphertext, CipherMode::kEncrypt);
}

// NOLINTNEXTLINE(readability-make-member-function-const)
int BlowfishEngine::Decrypt(const std::span<const std::byte> cyphertext,
                            const std::span<std::byte> plaintext) {
  return Process(cyphertext, plaintext, CipherMode::kDecrypt);
}

int BlowfishEngine::Process(const std::span<const std::byte> source,
                            const std::span<std::byte> dest,
                            const CipherMode mode) const {
  CHECK_GE(dest.size(), source.size());
  const int length = static_cast<int>(source.size());
  if (key_ == nullptr) {
    base::MoveBytes(dest, source, length);
    return length;
  }
  const engine_rs::Blowfish& cipher = *key_->cipher;
  const bool encrypt = mode == CipherMode::kEncrypt;
  const auto out = dest.first(source.size());
  if (!Overlaps(source, out)) {
    const auto rust_source = AsRustSlice(source);
    if (encrypt) {
      engine_rs::blowfish_encrypt(cipher, rust_source, AsRustSlice(out));
    } else {
      engine_rs::blowfish_decrypt(cipher, rust_source, AsRustSlice(out));
    }
  } else {
    // Rust may not see one buffer as a shared and a mutable slice at once, so
    // working in place it crosses once. That is the only overlap there is a
    // use for.
    CHECK(source.data() == out.data())
        << "Blowfish source and destination overlap without being one buffer";
    if (encrypt) {
      engine_rs::blowfish_encrypt_in_place(cipher, AsRustSlice(out));
    } else {
      engine_rs::blowfish_decrypt_in_place(cipher, AsRustSlice(out));
    }
  }
  return (length / kBytesPerBlock) * kBytesPerBlock;
}
