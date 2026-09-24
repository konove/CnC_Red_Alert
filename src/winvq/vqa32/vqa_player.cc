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

// File: VqaPlayer, the public face of Movie.

#include "winvq/vqa32/vqa_player.h"

#include <expected>
#include <memory>
#include <string_view>
#include <utility>

#include "winvq/vqa32/movie.h"
#include "winvq/vqa32/vqa_audio_device.h"
#include "winvq/vqa32/vqaio.h"

std::expected<VqaPlayer, VqaError> VqaPlayer::Open(VqaIo& io,
                                                   const std::string_view name,
                                                   VqaClient& client,
                                                   VqaAudioDevice* const audio,
                                                   const VqaOptions& options) {
  auto movie = Movie::Open(io, name, client, audio, options);
  if (!movie.has_value()) {
    return std::unexpected(movie.error());
  }
  return VqaPlayer(std::move(*movie));
}

VqaPlayer::VqaPlayer(std::unique_ptr<Movie> movie) : movie_(std::move(movie)) {}

VqaPlayer::~VqaPlayer() = default;
VqaPlayer::VqaPlayer(VqaPlayer&& other) noexcept = default;
VqaPlayer& VqaPlayer::operator=(VqaPlayer&& other) noexcept = default;

void VqaPlayer::Run() { movie_->Run(); }

VqaStepResult VqaPlayer::Step() { return movie_->Step(); }

void VqaPlayer::Pause() { movie_->Pause(); }

void VqaPlayer::Resume() { movie_->Resume(); }

bool VqaPlayer::paused() const { return movie_->paused(); }

int VqaPlayer::last_frame_shown() const { return movie_->last_frame_shown(); }

int VqaPlayer::frame_count() const { return movie_->frame_count(); }

int VqaPlayer::frame_rate() const { return movie_->frame_rate(); }
