# VQA Player Classes

The VQA player in `src/winvq/vqa32` has had its naming, type and comment passes, but its shape is
still the 1995 C design: one state struct handed to free functions, bit flags for state machines,
file-scope globals, link-time hooks and a C control block for configuration. This plan turns it into
classes with one job each and redesigns the public API with it.

Measured on the tree at 48ea8af8.

## Findings

- **One god struct passed everywhere.** `VqaPlayerState` (`vqa_player_state.h`) holds the config,
  header and a `VqaMovie` that nests `VqaLoader`, `VqaDrawer`, `VqaFlipper`, `VqaAudio`. Every
  operation is a free function taking `VqaPlayerState*` (`OpenVqa`, `PlayVqa`, `LoadNextFrame`,
  `DrawNextFrame`, `ConfigureDrawer`, `CopyStagedAudio`, …), so any function can touch any state.
- **Bit-flag state machines.** `kMovie*`, `kFrame*`, `kCodebook*`, `kDrawer*`, `kAudio*` flags
  encode states that should be enums/bools; `kMovieLoaderAsleep` + a stored `chunk_header` is a
  hand-rolled coroutine.
- **Intrusive rings.** `vector<unique_ptr<Node>>` plus `next` pointers (`LinkRing`) instead of
  values with indices.
- **Globals.** `audio.cc` keeps the sound converter, the playing movie, the _movie clock_,
  `open_movie_count` and the pause flag as file statics; `vqa_movie_loaded` is a global atomic. Two
  `VqaPlayer`s (TD's `Choose_Side` opens two) share one clock.
- **Link-time hooks.** The game supplies `QueueVqaPalette()` as a free function; `PauseVqaAudio()` /
  `ResumeVqaAudio()` are globals.
- **`VqaConfig` is a C "control block".** It bundles four unrelated things: the client's callbacks
  (C function pointers), the audio device (`device_id` + `SDL_AudioSpec*` + a `void (**)(span)`
  slot), where to draw (`image_buffer` into the _game's_ page, size, origin, margins), and tuning.
  The game keeps one forever in `GameState::anim_control_` and pokes fields before each movie. Most
  of it is never changed by the games: they only set callbacks, audio device, image buffer, audio
  on/off, no-skip and slow palette; `frame_rate`, `clock_source`, alt audio, step,
  `kVqaDrawNothing`, origin/margins are defaults or test-only. `image_buffer` makes the player write
  into a game screen page at an offset it computes from origin/margins, then the callback blits that
  page — placement is a presentation concern living in the decoder.
- **C-style results.** `int` modes, `int32_t` codes mixing errors (`kVqaErrorRead`) with scheduling
  states (`kVqaNotTime`, `kVqaSleeping`), `-1` sentinels, an `Open`/`Close` state machine.
- **Copy-paste.** Three `Load*` / `LoadCompressed*` pairs differ only in buffer; three sound loaders
  repeat the "first chunk preloads the ring, else stage it" logic.
- **Latent bugs.** The audio thread clears `kVqaDrawNoSkip` in `config.draw_flags` while the main
  thread reads it unlocked (data race); the global clock is shared between players. The "flipper" is
  dead: `PlayVqa` releases each frame right after drawing, so `kMovieAwaitingRelease` /
  `kMovieDrawerAsleep` / the drawer's `kVqaSleeping` never happen outside tests.

Goal: classes with one job each, RAII for open/close and SDL resources, no globals, interfaces for
the two things the player needs from its host (a screen and a sound device), enum/optional types
that make invalid states unrepresentable, pure logic separated from I/O and SDL for unit tests.
Decided with the user: **the public API is redesigned too** (game callers updated), errors are
**`std::expected<…, VqaError>`**, and **`VqaConfig` is dissolved** into interfaces plus a small
options struct.

## Plan: public API (`vqa_player.h`)

`VqaConfig`'s four concerns become four separate things:

| Concern                           | Was                                                                            | Becomes                                                                                                        |
| --------------------------------- | ------------------------------------------------------------------------------ | -------------------------------------------------------------------------------------------------------------- |
| Where frames go                   | `frame_callback`, `event_handler`, link-time `QueueVqaPalette`                 | `VqaClient` interface                                                                                          |
| Where the image lives / is placed | `image_buffer`, `image_width/height`, origin bits, margins, `kVqaDrawToBuffer` | Nothing: the player owns a movie-sized frame and hands the client a read-only view; the client places/blits it |
| Sound device                      | `audio_device_id`, `audio_spec`, `audio_callback` slot, `kVqaOptionAudio`      | `VqaAudioDevice` interface, passed as a nullable pointer (null = silent, system clock)                         |
| Tuning                            | the rest                                                                       | `VqaOptions`, a few defaulted fields; the games pass `{}`                                                      |

```cpp
enum class VqaError { kOpen, kRead, kSeek, kNotVqa, kNoMemory, kAudio };

// A decoded frame, valid only during the call.
struct VqaFrameView {
  int frame_number;
  int width, height;                 // the movie's frame size
  std::span<const uint8_t> pixels;   // width * height, 8-bit indexed
  std::span<const uint8_t> palette;  // RGB triplets; empty when unchanged (a skipped
                                     // frame's palette is carried to the next drawn one)
};

// Implemented by whoever shows the movie.
class VqaClient {
 public:
  virtual ~VqaClient() = default;
  // Show the frame. Return false to stop the movie.
  virtual bool OnFrame(const VqaFrameView& frame) = 0;
  // A late frame was dropped to catch up. Return false to stop the movie.
  virtual bool OnFrameSkipped(int frame_number) { return true; }
  // Too early for the next frame; present or wait instead of spinning.
  virtual void OnIdle() {}
};

// The sound device a movie plays through. BasicLockable: lock() holds off the audio thread.
class VqaAudioDevice {
 public:
  virtual ~VqaAudioDevice() = default;
  virtual const SDL_AudioSpec& spec() const = 0;
  // Installs the movie's mixer, which the device calls on its audio thread, locked, with a
  // silenced buffer. Returns false if another movie's mixer is installed.
  virtual bool Attach(std::function<void(std::span<std::byte>)> mixer) = 0;
  virtual void Detach() = 0;
  virtual void lock() = 0;
  virtual void unlock() = 0;
};

struct VqaOptions {
  bool skip_late_frames = false;         // was !kVqaDrawNoSkip; turned on by an audio underrun
  int frame_buffers = 6;
  int codebook_buffers = 3;
  std::optional<int> audio_ring_bytes;   // nullopt = 1.5 s of the movie's sound
  int audio_block_bytes = 2048;
};

enum class VqaStepResult { kFrameShown, kWaiting, kEnded };

// An open movie: Open() reads the header and preloads frames; destruction closes it. Movable.
class VqaPlayer {
 public:
  static std::expected<VqaPlayer, VqaError> Open(VqaIo& io, std::string_view name,
                                                 VqaClient& client, VqaAudioDevice* audio,
                                                 const VqaOptions& options = {});
  void Run();            // blocks until the movie ends or the client stops it
  VqaStepResult Step();  // one load + draw turn (the old kVqaModeWalk)
  int last_frame_shown() const;
 private:
  std::unique_ptr<Movie> movie_;
};
```

Dropped outright: `kVqaModePause/Stop` (games never use them; stop = client returns false or the
player is destroyed), `frame_rate` override, `clock_source`, alternate audio track selection (the
loader plays the primary track and skips `SNA*`), `kVqaOptionStep`, `kVqaDrawNothing`,
`kVqaOptionSlowPalette` (the game passes its own setting to `SetPalette`), `SetVqaConfigDefaults`,
all `kVqa*` int constants, `PauseVqaAudio/ResumeVqaAudio`, `QueueVqaPalette`.

**Host side (tech/ and games):**

- `tech/mixer_vqa_audio.{h,cc}`: `MixerVqaAudio final : VqaAudioDevice` over `AudioMixer` — same
  adapter pattern as `GameFileVqaIo`. `AudioMixer`'s `AudioCallback` becomes
  `std::function<void(std::span<std::byte>)>` (set under the device lock) and gains
  `SetExtraPaused(bool)` for focus loss: the movie mixer isn't called → silence → the audio clock
  and frames wait, exactly today's behavior.
- Each game gets a `MovieScreen final : VqaClient` (RA `ra/movie.cc`, TD `td/conquer.cc`, declared
  in `ra/movie.h` / `td/conquer.h`, also used by `td/intro.cc`) replacing `VQ_Call_Back`,
  `VQ_Event_Handler`, `QueueVqaPalette`, `Check_VQ_Palette_Set` and the `VQPalette*` statics. It
  holds the target page (`sys_mem_page` or RA's `vq640`), copies the frame in centered (what the
  default margins did), sets the palette, scales/blits, handles Esc and focus loss. One extra 64–256
  KB copy per frame.
- `GameState::anim_control_` and both `Anim_Init()`s go; call sites build io + screen + player
  locally. TD `Choose_Side`'s two concurrent movies become two `std::optional<VqaPlayer>`s.

## Plan: internal design

```
VqaPlayer                         public handle; std::unique_ptr<Movie>
└─ Movie                          one open movie; ctor = open, dtor = close (RAII)
   ├─ VqaHeader                   on-disk header (vqa_format.h, unchanged)
   ├─ FrameRing                   vector<Frame>, vector<Codebook> by value; load/draw cursors
   ├─ MovieLoader                 ChunkReader, codebook assembly, pending sound chunk
   │    writes FrameRing, AudioRing
   ├─ MovieDrawer                 own frame image, VqDecoder, frame pacing, carried palette
   │    reads FrameRing + MovieClock, calls VqaClient
   ├─ AudioRing                   pure ring buffer + block bookkeeping (no SDL)
   ├─ std::optional<AudioOutput>  SDL_AudioStream (unique_ptr deleter) + VqaAudioDevice&
   └─ MovieClock                  per movie; system clock or AudioOutput's played bytes
```

| Class / type (file)                               | Responsibility                                                                                                                                                                                         | Replaces                                                                                                                       |
| ------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------ |
| `ChunkReader` (`chunk_reader.*`)                  | IFF chunks over `VqaIo`: `Next()` → `expected<Chunk>`, `Skip`, `ReadPayload(chunk, span)`; validates size, byteswap, padding                                                                           | raw `ChunkHeader` reads, `ChunkSize/PadSize/IsValidChunkSize`, ~8 repeated `Seek(PadSize…)`                                    |
| `LcwBuffer` (`lcw_buffer.*`)                      | Fixed-capacity buffer loaded raw, or LCW-compressed at its end; idempotent `Decompress()`; `data()`                                                                                                    | `*_offset` + `k*Compressed` pairs, the 6 `Load`/`LoadCompressed` functions, `DecompressPalette/DecompressFrame`                |
| `Codebook`, `Frame`, `FrameRing` (`frame_ring.*`) | Frame state `enum class {kFree, kLoaded}`, `bool key`, optional palette, codebook index; ring with loader/drawer cursors, `Release()`                                                                  | `VqaFrame/VqaCodebook`, `next`, `LinkRing`, `kFrame*/kCodebook*`, `VqaFlipper`                                                 |
| `MovieLoader` (`movie_loader.*`)                  | `LoadNextFrame()` → `kLoaded/kNoBuffer/kAudioFull/kEnd` or `VqaError`; partial-codebook assembly; `std::optional<Chunk> pending_sound_` to resume; one `StageSound(chunk, SoundCodec)` for PCM/ZAP/IMA | `loader.cc` free functions, `VqaLoader`, `kMovieLoaderAsleep`, `FramePart`, the 3 sound loaders                                |
| `ImaAdpcmDecoder` (`adpcm_decoders.*`)            | Stateful decoder, `Decode(source, dest)`                                                                                                                                                               | `AdpcmStream` + `DecodeAdpcmSound(stream*, …)` (out of `vqm32/soscomp.h`; vqa32 is its only user)                              |
| `AudioFormat` (`audio_ring.h`)                    | rate/channels/bits, `bytes_per_second()`, `FromHeader(header)`                                                                                                                                         | loose `VqaAudio` fields, v1 special case in `AllocateMovie`                                                                    |
| `AudioRing` (`audio_ring.*`)                      | ring, staging, `CopyStaged() → bool`, `CommitPreload`, next-block/advance, atomic `underrun`, `blocks_played`, `movie_loaded`                                                                          | `VqaAudio`, `CopyStagedAudio`, `CommitPreload`, `vqa_movie_loaded` global, the `draw_flags` race                               |
| `AudioOutput` (`audio_output.*`)                  | Converter stream; `Start/Stop` = `Attach/Detach` a closure over the ring (so no open-count); `PlayedBytes()`; locks via `std::scoped_lock(device)`                                                     | `audio.cc` statics, `DeviceLock`, `kAudioOpen/kAudioPlaying`                                                                   |
| `MovieClock` (`movie_clock.*`)                    | `Set(ticks)`, `Now()`; audio source when `AudioOutput` plays, else steady_clock                                                                                                                        | statics `audio_clock`, `clock_offset_ticks`, `SetMovieClock/ReadMovieClock`                                                    |
| `VqDecoder` (`vq_decoder.*`)                      | `enum class BlockShape`, `BlockShapeFor(w,h) → optional`, decode into a `width`-stride frame                                                                                                           | `decode_frame` fn pointer, `DecodeNothing`, `BlockDimensions`                                                                  |
| `MovieDrawer` (`movie_drawer.*`)                  | Owns the frame image; `DrawNextFrame()` → `kDrawn/kNotTime/kNoFrame/kStopped`; `SelectFrame` pacing; carried palette; releases the frame itself                                                        | `drawer.cc`, `ConfigureDrawer` (placement deleted), `VqaDrawer`, `kDrawerPalettePending`, `kMovieAwaitingRelease/DrawerAsleep` |
| `Movie` (`movie.*`)                               | `Open` → `expected<unique_ptr<Movie>, VqaError>`; `Step()` loop body from `PlayVqa`                                                                                                                    | `VqaPlayerState`, `VqaMovie`, `OpenVqa/PrepareMovie/AllocateMovie/PreloadFrames/CloseVqa/PlayVqa`, `kMovie*`                   |

`VqaIo` stays (tech depends on vqa32, so `ByteStream` can't be used without inverting that).

## Phases

Each phase is one to three commits via `/commit`; the build, `vqa32_test` and strict checks stay
green at every commit. Internals first under the old API, public API after.

0. **Plan doc.** Write this plan to `docs/VQA_PLAYER_CLASSES_PLAN.md`, commit alone, add a MEMORY.md
   "Current Work" line; record progress per phase.
1. **Leaf building blocks, each with its own `_test.cc`:** `ChunkReader`, `LcwBuffer`,
   `ImaAdpcmDecoder` (drop `soscomp.h`'s declarations), `BlockShape`. Switch `loader.cc`/`drawer.cc`
   onto them.
2. **`FrameRing`.** `Frame`/`Codebook` by value with enum/bool state and indices; delete `next`,
   `LinkRing`, frame/codebook flags. Drop the flipper: the drawer releases after the callback.
3. **Audio split.** `AudioFormat`, `AudioRing` (the CopyStagedAudio/preload tests move to
   `audio_ring_test.cc`), `AudioOutput`, `MovieClock`; delete the `audio.cc` statics and
   `vqa_movie_loaded`. Introduce `VqaAudioDevice` + `tech/mixer_vqa_audio.*`, `AudioCallback` →
   `std::function`, `AudioMixer::SetExtraPaused`; tests get a `FakeVqaAudioDevice` (no SDL device).
4. **`MovieLoader`.** Class over `ChunkReader`/`FrameRing`/`AudioRing`; `pending_sound_` replaces
   the asleep flag; the three sound loaders unify behind `SoundCodec`.
5. **`MovieDrawer`.** Class with decoder and pacing (`SelectFrame` tested with a fake clock).
6. **Public API + callers.** `Movie`, `VqaPlayer::Open` factory, `VqaClient`, `VqaFrameView`,
   `VqaOptions`, `std::expected`; the player owns the frame image (placement and its tests deleted).
   Games: `MovieScreen` in RA (`ra/movie.{h,cc}`) and TD (`td/conquer.{h,cc}`, used by
   `td/intro.cc`); delete `Anim_Init` (`ra/init.cc`, `td/init.cc`), `anim_control_`
   (`ra/game_state.h`, `td/game_state.h`), the palette hooks in `ra/nondosstub.cc`,
   `td/nondosstub.cc`, `td/winstub.cc` (its `QueueVqaPalette(unsigned char*, long…)` is already a
   stale overload), and switch `PauseVqaAudio/ResumeVqaAudio` in `{ra,td}/winstub.cc`,
   `{ra,td}/sdlstub.cc` to `TheAudio().SetExtraPaused()`. Delete `vqa_player_state.h`, `config.cc`,
   `audio.cc`, `loader.cc`, `drawer.cc`.
7. **Tidy.** `ZapHeader` fields to Google names in `vqa_format.h`; shared test helpers (`FakeVqaIo`,
   `MovieStart`, `AppendChunk`, …) into `vqa_test_util.h`; `vqa_player_test.cc` keeps end-to-end
   tests through `VqaPlayer` with a recording `VqaClient`; update `CLAUDE.md` Key Files and the
   `tech/game_file_vqa_io.h` comment.

Files keep the EA header where they carry EA code; brand-new files (`chunk_reader.*`,
`lcw_buffer.*`, `frame_ring.*`, `audio_ring.*`, `movie_clock.*`, `mixer_vqa_audio.*`) get none.

## Reuse

`base::CopyBytes`, `base::ObjectBytes`, `base::At`, `base::ToSize` (`base/`), `LCW_Uncompress`
(`vqm32/compress.h`), `MakeId`/`ID_FORM` (`vqm32/iff.h`), `DecodeBlocks` (`vq_decoder.cc`),
`port::WriteUnaligned`; game side `Interpolate_2X_Scale`, `PixelView::BlitTo`, `SetPalette`.

## Verification

- Per stage: `tools/strict_tu.py <changed files>`; `cmake --build build --parallel 22` and
  `build/src/winvq/vqa32/vqa32_test`. Every behavior tested in today's `vqa_player_test.cc`
  survives, moved to the class that owns it: open errors, 2 GiB chunks, oversized chunks, partial
  codebook offsets, skipped-frame palette carried to the next frame, ring wrap, preload wrap, walk
  keeps sound, stop, audio conversion failure.
- New tests: two movies have independent clocks; an underrun turns on frame skipping; `Attach`
  failing while another movie holds the device; `FrameRing` cursor handoff; `VqaFrameView` carries
  the movie's size and palette.
- Full strict build (`--parallel 14`) before the last commit of phases 3 and 6.
- Real display (user check; headless `SDL_VIDEODRIVER=dummy` never loads palettes): RA intro
  (`REDINTRO`, the 640x400 path) and a 320x200 briefing with sound, Esc breakout, focus loss pausing
  sound and frames; TD `Choose_Side` (two movies open at once) and a briefing.

## Progress

| Phase                   | Status | Commits |
| ----------------------- | ------ | ------- |
| 0. Plan doc             | todo   |         |
| 1. Leaf building blocks | todo   |         |
| 2. FrameRing            | todo   |         |
| 3. Audio split          | todo   |         |
| 4. MovieLoader          | todo   |         |
| 5. MovieDrawer          | todo   |         |
| 6. Public API + callers | todo   |         |
| 7. Tidy                 | todo   |         |
