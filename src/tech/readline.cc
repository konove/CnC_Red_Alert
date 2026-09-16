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

#include <cctype>
#include <cstring>

#include "tech/byte_source.h"
#include "tech/file.h"
#include "tech/file_source.h"

void strtrim(char* buffer) {
  if (buffer) {
    /*
    **	Strip leading white space from the string.
    */
    const char* source = buffer;
    while (isspace(*source)) {
      source++;
    }
    if (source != buffer) {
      memmove(buffer, source, strlen(source) + 1);
    }

    /*
    **	Clip trailing white space from the string.
    */
    for (int index = static_cast<int>(strlen(buffer)) - 1; index >= 0; index--) {
      if (isspace(buffer[index])) {
        buffer[index] = '\0';
      } else {
        break;
      }
    }
  }
}

int Read_Line(File& file, char* buffer, int len, bool& eof) {
  FileSource fs(file);
  return Read_Line(fs, buffer, len, eof);
}

int Read_Line(ByteSource& file, char* buffer, int len, bool& eof) {
  if (len == 0 || buffer == nullptr) {
    return 0;
  }

  int count = 0;
  for (;;) {
    char c = 0;
    if (!file.ReadObject(c)) {
      eof = true;
      buffer[0] = '\0';
      break;
    }

    if (c == '\x0A') {
      break;
    }
    if (c != '\x0D' && count + 1 < len) {
      buffer[count++] = c;
    }
  }
  buffer[count] = '\0';

  strtrim(buffer);
  return static_cast<int>(strlen(buffer));
}
