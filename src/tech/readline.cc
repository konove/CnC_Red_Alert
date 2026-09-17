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

#include "tech/readline.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iterator>
#include <span>
#include <string_view>

#include "base/array.h"
#include "tech/byte_source.h"
#include "tech/file.h"
#include "tech/file_source.h"

void strtrim(std::span<char> buffer) {
  const auto end = std::ranges::find(buffer, '\0');
  auto text = buffer.first(static_cast<std::size_t>(end - buffer.begin()));
  while (!text.empty() && isspace(static_cast<unsigned char>(text.front()))) {
    text = text.subspan(1);
  }
  while (!text.empty() && isspace(static_cast<unsigned char>(text.back()))) {
    text = text.first(text.size() - 1);
  }
  // Forward copy is safe when removing a prefix from the same buffer.
  for (std::size_t i = 0; i < text.size(); ++i) {
    base::At(buffer, i) = base::At(text, i);
  }
  if (text.size() < buffer.size()) {
    base::At(buffer, static_cast<std::size_t>(text.size())) = '\0';
  }
}

int Read_Line(File& file, std::span<char> buffer, bool& eof) {
  FileSource fs(file);
  return Read_Line(fs, buffer, eof);
}

int Read_Line(ByteSource& file, std::span<char> buffer, bool& eof) {
  if (buffer.empty()) {
    return 0;
  }

  int count = 0;
  for (;;) {
    char c = 0;
    if (!file.ReadObject(c)) {
      eof = true;
      base::At(buffer, static_cast<std::size_t>(0)) = '\0';
      break;
    }

    if (c == '\x0A') {
      break;
    }
    if (c != '\x0D' && count + 1 < std::ssize(buffer)) {
      base::At(buffer, static_cast<std::size_t>(count++)) = c;
    }
  }
  base::At(buffer, static_cast<std::size_t>(count)) = '\0';

  strtrim(buffer);
  return static_cast<int>(std::string_view(buffer.data()).size());
}
