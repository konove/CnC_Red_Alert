// File: vqaplay, a command line player for VQA movies, for watching one movie
// or stepping through it frame by frame outside the games. It plays through
// vqa32 exactly as the games do, and reads the movie through the games' file
// lookup and their sound mixer.
//
//   vqaplay [options] <game-dir> <name>   a movie in an installation of either
//                                         game, loose or in its archives
//   vqaplay [options] <file>              a movie file anywhere
//
//   --paused     stop on the first frame
//   --scale=N    open the window at N times the movie's size (default 2)
//   --mute       play without sound
//   --skip-late  drop late frames to keep up, rather than show every one
//
// Space pauses and resumes; Right or '.' shows the next frame and pauses;
// Esc or Q quits. The window can be resized.

#include <SDL.h>
#include <SDL_error.h>
#include <SDL_events.h>
#include <SDL_keycode.h>
#include <SDL_pixels.h>
#include <SDL_render.h>
#include <SDL_timer.h>
#include <SDL_video.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "absl/base/attributes.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_format.h"
#include "engine/audio/audio_mixer.h"
#include "engine/base/array.h"
#include "engine/base/numeric.h"
#include "engine/video/game_file_vqa_io.h"
#include "engine/video/mixer_vqa_audio.h"
#include "engine/video/vqa/vqa_player.h"
#include "tools/game_data.h"

namespace {

// The command line, parsed.
struct Options {
  std::string game_dir;  // Where the movie is looked up.
  std::string name;      // The movie's file name.
  bool paused = false;
  int scale = 2;
  bool mute = false;
  bool skip_late = false;
};

// What the viewer asked for since the last look.
enum class Command { kNone, kQuit, kTogglePause, kStepFrame };

struct WindowDeleter {
  void operator()(SDL_Window* window) const { SDL_DestroyWindow(window); }
};
struct RendererDeleter {
  void operator()(SDL_Renderer* renderer) const {
    SDL_DestroyRenderer(renderer);
  }
};
struct TextureDeleter {
  void operator()(SDL_Texture* texture) const { SDL_DestroyTexture(texture); }
};

// Shows the frames in a window, opened at the first frame since only the
// frames say how big the movie is, and reads the viewer's keys.
class MovieWindow final : public VqaClient {
 public:
  MovieWindow(std::string_view name, const int scale)
      : name_(name), scale_(scale) {}

  bool OnFrame(const VqaFrameView& frame) override {
    if (!frame.palette.empty()) {
      SetPalette(frame.palette);
    }
    if (!Open(frame.width, frame.height)) {
      return false;
    }
    std::ranges::transform(frame.pixels, pixels_.begin(),
                           [this](const uint8_t index) {
                             return base::At(std::span(palette_), index);
                           });
    SDL_UpdateTexture(texture_.get(), nullptr, pixels_.data(),
                      frame.width * int{sizeof(uint32_t)});
    Present();
    return true;
  }

  // Too early for the next frame, or paused: give the time back.
  void OnIdle() override { SDL_Delay(1); }

  // Reads the window's events, redrawing it when it needs that, and returns
  // the last command among them.
  Command PollCommand() {
    Command command = Command::kNone;
    SDL_Event event{};
    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_QUIT) {
        return Command::kQuit;
      }
      if (event.type == SDL_WINDOWEVENT) {
        Present();
      } else if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
          case SDLK_ESCAPE:
          case SDLK_q:
            return Command::kQuit;
          case SDLK_SPACE:
            command = Command::kTogglePause;
            break;
          case SDLK_RIGHT:
          case SDLK_PERIOD:
            command = Command::kStepFrame;
            break;
          default:
            break;
        }
      }
    }
    return command;
  }

  // Puts the frame shown and whether the movie is paused in the title.
  void ShowStatus(const int frame, const int frame_count, const bool paused) {
    if (window_ == nullptr) {
      return;
    }
    const std::string title =
        absl::StrFormat("%s - frame %d/%d%s", name_, frame + 1, frame_count,
                        paused ? " - paused" : "");
    if (title != title_) {
      title_ = title;
      SDL_SetWindowTitle(window_.get(), title_.c_str());
    }
  }

  // Why the window could not be opened; empty if it was.
  [[nodiscard]] const std::string& error() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return error_;
  }

 private:
  // Opens the window for frames of this size, once.
  bool Open(const int width, const int height) {
    if (window_ != nullptr) {
      return true;
    }
    window_.reset(SDL_CreateWindow(name_.c_str(), SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED, width * scale_,
                                   height * scale_, SDL_WINDOW_RESIZABLE));
    if (window_ != nullptr) {
      renderer_.reset(SDL_CreateRenderer(window_.get(), -1, 0));
    }
    if (renderer_ != nullptr) {
      // Scales the frame to the window, keeping its shape.
      SDL_RenderSetLogicalSize(renderer_.get(), width, height);
      texture_.reset(
          SDL_CreateTexture(renderer_.get(), SDL_PIXELFORMAT_ARGB8888,
                            SDL_TEXTUREACCESS_STREAMING, width, height));
    }
    if (texture_ == nullptr) {
      error_ = SDL_GetError();
      return false;
    }
    pixels_.resize(base::ToSize(width * height));
    return true;
  }

  // The movies' palettes are 6 bits a color, as the VGA took them.
  void SetPalette(const std::span<const uint8_t> palette) {
    const auto colors = std::min(palette.size() / 3, palette_.size());
    for (std::size_t i = 0; i < colors; ++i) {
      const auto expand = [&](const std::size_t channel) {
        const auto value =
            static_cast<uint32_t>(base::At(palette, (i * 3) + channel) & 63U);
        return (value << 2U) | (value >> 4U);
      };
      base::At(std::span(palette_), i) =
          0xFF000000U | (expand(0) << 16U) | (expand(1) << 8U) | expand(2);
    }
  }

  void Present() {
    if (texture_ == nullptr) {
      return;
    }
    SDL_RenderClear(renderer_.get());
    SDL_RenderCopy(renderer_.get(), texture_.get(), nullptr, nullptr);
    SDL_RenderPresent(renderer_.get());
  }

  std::string name_;
  int scale_;
  std::string title_;
  std::string error_;
  std::array<uint32_t, 256> palette_{};  // ARGB8888.
  std::vector<uint32_t> pixels_;         // The frame in ARGB8888.
  std::unique_ptr<SDL_Window, WindowDeleter> window_;
  std::unique_ptr<SDL_Renderer, RendererDeleter> renderer_;
  std::unique_ptr<SDL_Texture, TextureDeleter> texture_;
};

std::string_view ErrorText(const VqaError error) {
  switch (error) {
    case VqaError::kOpen:
      return "not found";
    case VqaError::kRead:
      return "read error or truncated movie";
    case VqaError::kSeek:
      return "seek error";
    case VqaError::kNotVqa:
      return "not a VQA movie";
    case VqaError::kNoMemory:
      return "no play buffers";
    case VqaError::kAudio:
      return "the sound cannot be played";
    default:
      return "unknown error";
  }
}

// Returns false, having said why, on a command line that makes no sense.
bool ParseOptions(const std::span<char* const> args, Options& options) {
  std::vector<std::string_view> positional;
  for (const char* const arg : args.subspan(1)) {
    const std::string_view text(arg);
    if (text == "--paused") {
      options.paused = true;
    } else if (text == "--mute") {
      options.mute = true;
    } else if (text == "--skip-late") {
      options.skip_late = true;
    } else if (text.starts_with("--scale=")) {
      if (!absl::SimpleAtoi(text.substr(8), &options.scale) ||
          options.scale < 1 || options.scale > 8) {
        absl::FPrintF(stderr, "--scale takes 1 to 8\n");
        return false;
      }
    } else if (text.starts_with("--")) {
      absl::FPrintF(stderr, "unknown option: %s\n", text);
      return false;
    } else {
      positional.push_back(text);
    }
  }

  if (positional.size() == 2) {
    options.game_dir = positional.front();
    options.name = positional.back();
    return true;
  }
  if (positional.size() == 1) {
    const std::filesystem::path path(positional.front());
    options.game_dir =
        path.has_parent_path() ? path.parent_path().string() : ".";
    options.name = path.filename().string();
    return true;
  }
  absl::FPrintF(stderr,
                "usage: vqaplay [options] <game-dir> <name>\n"
                "       vqaplay [options] <file>\n"
                "options: --paused --scale=N --mute --skip-late\n"
                "keys: Space pause, Right or . next frame, Esc or Q quit\n");
  return false;
}

int Play(const Options& options) {
  OpenGameData(options.game_dir);

  // The file, the window and the sound must outlive the player.
  GameFileVqaIo io;
  MovieWindow window(options.name, options.scale);
  AudioMixer mixer;
  MixerVqaAudio movie_audio(mixer);
  // The games' own rate; the movie's sound is converted to it.
  const bool with_sound = !options.mute && mixer.Open(22050, false);

  VqaOptions vqa_options;
  vqa_options.skip_late_frames = options.skip_late;
  auto player =
      VqaPlayer::Open(io, options.name, window,
                      with_sound ? &movie_audio : nullptr, vqa_options);
  if (!player.has_value()) {
    absl::FPrintF(stderr, "%s: %s\n", options.name, ErrorText(player.error()));
    return 1;
  }
  absl::PrintF("%s: %d frames at %d fps%s\n", options.name,
               player->frame_count(), player->frame_rate(),
               with_sound ? "" : ", silent");

  // Stepping lets the movie run until it shows one more frame, then pauses
  // it; --paused is a step to the first frame.
  bool pause_after_frame = options.paused;
  while (true) {
    switch (window.PollCommand()) {
      case Command::kQuit:
        return 0;
      case Command::kTogglePause:
        if (player->paused()) {
          player->Resume();
        } else {
          player->Pause();
        }
        pause_after_frame = false;
        break;
      case Command::kStepFrame:
        player->Resume();
        pause_after_frame = true;
        break;
      case Command::kNone:
      default:
        break;
    }

    const VqaStepResult result = player->Step();
    if (result == VqaStepResult::kEnded) {
      break;
    }
    if (result == VqaStepResult::kFrameShown && pause_after_frame) {
      player->Pause();
      pause_after_frame = false;
    }
    window.ShowStatus(player->last_frame_shown(), player->frame_count(),
                      player->paused());
  }

  if (!window.error().empty()) {
    absl::FPrintF(stderr, "cannot open a window: %s\n", window.error());
    return 1;
  }
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  // argv is a pointer and a count, which is the one place a span has to be
  // built from both; argc counts the entries argv holds.
  // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
  const std::span<char* const> args(argv, base::ToSize(argc));
  Options options;
  if (!ParseOptions(args, options)) {
    return 2;
  }

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
    absl::FPrintF(stderr, "SDL_Init: %s\n", SDL_GetError());
    return 1;
  }
  const int status = Play(options);
  SDL_Quit();
  return status;
}
