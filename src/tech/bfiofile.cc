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

/* $Header: /CounterStrike/BFIOFILE.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Westwood Library *
 *                                                                                             *
 *                    File Name : RAMFILE.CPP *
 *                                                                                             *
 *                   Programmer : David R. Dettmer *
 *                                                                                             *
 *                   Start Date : November 10, 1995 *
 *                                                                                             *
 *                  Last Update : November 10, 1995  [DRD] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * BufferIOFileClass::BufferIOFileClass -- Filename based
 *constructor for a file object.     * BufferIOFileClass::BufferIOFileClass --
 *default constructor for a file object.            * BufferIOFileClass::Cache
 *-- Load part or all of a file data into RAM.                     *
 *   BufferIOFileClass::Close -- Perform a closure of the file. *
 *   BufferIOFileClass::Commit -- Writes the cache to the file if it has
 *changed.              * BufferIOFileClass::Free -- Frees the allocated buffer.
 ** BufferIOFileClass::IsAvailable -- Checks for existence of file cached or on
 *disk.        * BufferIOFileClass::IsOpen -- Determines if the file is open. *
 *   BufferIOFileClass::Open -- Assigns name and opens file in one operation. *
 *   BufferIOFileClass::Open -- Opens the file object with the rights specified.
 ** BufferIOFileClass::Read -- Reads data from the file cache. *
 *   BufferIOFileClass::Seek -- Moves the current file pointer in the file. *
 *   BufferIOFileClass::SetName -- Checks for name changed for a cached file. *
 *   BufferIOFileClass::Size -- Determines size of file (in bytes). *
 *   BufferIOFileClass::Write -- Writes data to the file cache. *
 *   BufferIOFileClass::~BufferIOFileClass -- Destructor for the file object. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "tech/bfiofile.h"

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <string_view>
#include <cstring>

#include "base/numeric.h"
#include "sdllib/file_access.h"
#include "tech/rawfile.h"
#include "tech/wwfile.h"
/***********************************************************************************************
 * BufferIOFileClass::BufferIOFileClass -- Filename based constructor for a file
 *object.       *
 *                                                                                             *
 *    This constructor is called when a file object is created with a supplied
 *filename, but   * not opened at the same time. In this case, an assumption is
 *made that the supplied       * filename is a constant string. A duplicate of
 *the filename string is not created since   * it would be wasteful in that
 *case.                                                       *
 *                                                                                             *
 * INPUT:   filename -- The filename to assign to this file object. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 11/10/1995 DRD : Created. *
 *=============================================================================================*/
BufferIOFileClass::BufferIOFileClass(const std::string_view filename) {
  BufferIOFileClass::SetName(filename);
}

/***********************************************************************************************
 * BufferIOFileClass::~BufferIOFileClass -- Destructor for the file object. *
 *                                                                                             *
 *    This destructor will free all memory allocated thru using Cache routines.
 **
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 11/10/1995 DRD : Created. *
 *=============================================================================================*/
BufferIOFileClass::~BufferIOFileClass() {
  // Close() commits buffered writes; Free() only discards the buffer, clearing
  // has_unwritten_changes_ without writing it out. ~RawFileClass cannot reach
  // this override
  // -- by the time it runs, the object is no longer a BufferIOFileClass -- so
  // the commit has to happen here or pending writes are lost. Qualified: the
  // derived parts are already gone, so this class's version is the right one.
  BufferIOFileClass::Close();
  Free();
}

/***********************************************************************************************
 * BufferIOFileClass::Cache -- Load part or all of a file data into RAM. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  bool; Was the file load successful?  It could fail if there wasn't
 *enough room     * to allocate the raw data block. *
 *                                                                                             *
 * WARNINGS:   This routine goes to disk for a potentially very long time. *
 *                                                                                             *
 * HISTORY: * 11/10/1995 DRD : Created. *
 *=============================================================================================*/
bool BufferIOFileClass::Cache(int32_t size, void* buffer) {
  if (buffer_) {
    //
    // if trying to cache again with size or buffer fail
    //
    return size == 0 && buffer == nullptr;
  }

  if (IsAvailable()) {
    file_size_ = Size();
  } else {
    file_size_ = 0;
  }

  if (size) {
    //
    // minimum buffer size for performance
    //
    if (size < kMinimumBufferSize) {
      size = kMinimumBufferSize;

      /*
      **	Specifying a size smaller than the minimum is an error
      **	IF a buffer pointer was also specified. In such a case the
      **	system cannot use the buffer.
      */
      if (buffer) {
        Error(EINVAL);
      }
    }

    buffer_size_ = size;
  } else {
    buffer_size_ = file_size_;
  }

  //
  // if size == 0 and a buffer is specified then that is invalid.
  // if the buffer_size_ is 0 then this must be a new file and no size was
  // specified so exit.
  //
  if ((size == 0 && buffer) || !buffer_size_) {
    return false;
  }

  if (buffer) {
    buffer_ = buffer;
  } else {
    buffer_ = new char[base::ToSize(buffer_size_)];
  }

  if (buffer_) {
    owns_buffer_ = true;
    is_disk_open_ = false;
    buffer_position_ = 0;
    buffer_file_position_ = 0;
    change_begin_ = -1;
    change_end_ = -1;
    file_position_ = 0;
    true_file_start_ = 0;

    //
    // the file was checked for availability then set the file_size_
    //
    if (file_size_) {
      int32_t read_size;
      bool opened_here = false;
      int32_t previous_position = 0;

      if (file_size_ <= buffer_size_) {
        read_size = file_size_;
      } else {
        read_size = buffer_size_;
      }

      if (IsOpen()) {
        //
        // get previous file position
        //
        previous_position = Seek(0);

        //
        // get true file position
        //
        if (RawFileClass::IsOpen()) {
          true_file_start_ = RawFileClass::Seek(0);
        } else {
          true_file_start_ = previous_position;
        }

        if (file_size_ <= buffer_size_) {
          //
          // if previous position is non-zero seek to the beginning
          //
          if (previous_position) {
            Seek(0, SEEK_SET);
          }

          //
          // set the buffer position for future reads/writes
          //
          buffer_position_ = previous_position;
        } else {
          buffer_file_position_ = previous_position;
        }

        file_position_ = previous_position;
      } else {
        if (Open()) {
          true_file_start_ = RawFileClass::Seek(0);
          opened_here = true;
        }
      }

      const int32_t bytes_read = Read(buffer_, read_size);

      if (bytes_read != read_size) {
        Error(EIO);
      }

      if (opened_here) {
        Close();
      } else {
        //
        // seek to the previous position in the file
        //
        Seek(previous_position, SEEK_SET);
      }

      is_buffer_loaded_ = true;
    }

    use_buffer_ = true;
    return true;
  }

  Error(ENOMEM);

  return false;
}

/***********************************************************************************************
 * BufferIOFileClass::Free -- Frees the allocated buffer. *
 *                                                                                             *
 *    This routine will free the buffer. By using this in conjunction with the *
 *    Cache() function, one can maintain tight control of memory usage. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 11/10/1995 DRD : Created. *
 *=============================================================================================*/
void BufferIOFileClass::Free() {
  if (buffer_) {
    if (owns_buffer_) {
      delete[] static_cast<char*>(buffer_);
      owns_buffer_ = false;
    }

    buffer_ = nullptr;
  }

  buffer_size_ = 0;
  is_open_ = false;
  is_buffer_loaded_ = false;
  has_unwritten_changes_ = false;
  use_buffer_ = false;
}

/***********************************************************************************************
 * BufferIOFileClass::Commit -- Writes the cache to the file if it has changed.
 **
 *                                                                                             *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  false, did not need to write the buffer. * true, wrote the buffer. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 11/15/1995 DRD : Created. *
 *=============================================================================================*/
bool BufferIOFileClass::Commit() {
  if (use_buffer_) {
    if (has_unwritten_changes_) {
      const int32_t changed_size = change_end_ - change_begin_;

      if (is_disk_open_) {
        RawFileClass::Seek(
            true_file_start_ + buffer_file_position_ + change_begin_, SEEK_SET);
        RawFileClass::Write(buffer_, changed_size);
        RawFileClass::Seek(true_file_start_ + file_position_, SEEK_SET);
      } else {
        RawFileClass::Open();
        RawFileClass::Seek(
            true_file_start_ + buffer_file_position_ + change_begin_, SEEK_SET);
        RawFileClass::Write(buffer_, changed_size);
        RawFileClass::Close();
      }

      has_unwritten_changes_ = false;
      return true;
    }
    return false;
  }
  return false;
}

/***********************************************************************************************
 * BufferIOFileClass::SetName -- Checks for name changed for a cached file. *
 *                                                                                             *
 *    Checks for a previous filename and that it is cached.  If so, then check
 *the             * new filename against the old. If they are the same then
 *return that filename.            * Otherwise, the file object's name is set
 *with just the raw filename as passed            * to this routine. *
 *                                                                                             *
 * INPUT:   filename -- Pointer to the filename to set as the name of this file
 *object.        *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 11/15/1995 DRD : Created. *
 *=============================================================================================*/
void BufferIOFileClass::SetName(const std::string_view filename) {
  if (!FileName().empty() && use_buffer_) {
    if (filename == FileName()) {
      return;
    }
    Commit();
    is_buffer_loaded_ = false;
  }

  RawFileClass::SetName(filename);
}

/***********************************************************************************************
 * BufferIOFileClass::DoIsAvailable -- Checks for existence of file cached or
 * on disk.          *
 *                                                                                             *
 *                                                                                             *
 * INPUT:   mode -- kQuick for fast check, kBlocking for full error recovery. *
 *                                                                                             *
 * OUTPUT:  bool; Is the file available for opening? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 11/16/1995 DRD : Created. *
 *=============================================================================================*/
bool BufferIOFileClass::DoIsAvailable(AvailabilityCheck mode) {
  if (use_buffer_) {
    return true;
  }

  return RawFileClass::DoIsAvailable(mode);
}

/***********************************************************************************************
 * BufferIOFileClass::IsOpen -- Determines if the file is open. *
 *                                                                                             *
 *    If part or all of the file is cached, then return that it is opened. A
 *closed file       * doesn't have a valid pointer. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  bool; Is the file open? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 11/14/1995 DRD : Created. *
 *=============================================================================================*/
bool BufferIOFileClass::IsOpen() const {
  if (is_open_ && use_buffer_) {
    return true;
  }

  return RawFileClass::IsOpen();
}

/***********************************************************************************************
 * BufferIOFileClass::Open -- Assigns name and opens file in one operation. *
 *                                                                                             *
 *    This routine will assign the specified filename to the file object and
 *open it at the    * same time. If the file object was already open, then it
 *will be closed first. If the     * file object was previously assigned a
 *filename, then it will be replaced with the new    * name. Typically, this
 *routine is used when an anonymous file object has been crated and  * now it
 *needs to be assigned a name and opened. *
 *                                                                                             *
 * INPUT:   filename -- The filename to assign to this file object. *
 *                                                                                             *
 *          rights   -- The open file access rights to use. *
 *                                                                                             *
 * OUTPUT:  bool; Was the file opened? The return value of this is moot, since
 *the open file   * is designed to never return unless it succeeded. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 11/14/1995 DRD : Created. *
 *=============================================================================================*/
bool BufferIOFileClass::Open(const std::string_view filename, FileAccess rights) {
  SetName(filename);
  return BufferIOFileClass::Open(rights);
}

/***********************************************************************************************
 * BufferIOFileClass::Open -- Opens the file object with the rights specified. *
 *                                                                                             *
 *    This routine is used to open the specified file object with the access
 *rights indicated. * This only works if the file has already been assigned a
 *filename. It is guaranteed, by   * the error handler, that this routine will
 *always return with success.                    *
 *                                                                                             *
 * INPUT:   rights   -- The file access rights to use when opening this file.
 *This is a        * combination of READ and/or WRITE bit flags. *
 *                                                                                             *
 * OUTPUT:  bool; Was the file opened successfully? This will always return true
 *by reason of  * the error handler. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 11/14/1995 DRD : Created. *
 *=============================================================================================*/
bool BufferIOFileClass::Open(FileAccess rights) {
  BufferIOFileClass::Close();

  if (use_buffer_) {
    buffer_rights_ = rights;  // save rights requested for checks later

    if (rights != FileAccess::kRead || file_size_ > buffer_size_) {
      if (rights == FileAccess::kWrite) {
        RawFileClass::Open(rights);
        RawFileClass::Close();
        rights = FileAccess::kReadWrite;
        true_file_start_ = 0;  // now writing to single file
      }

      if (true_file_start_) {
        use_buffer_ = false;
        Open(rights);
        use_buffer_ = true;
      } else {
        RawFileClass::Open(rights);
      }

      is_disk_open_ = true;

      if (buffer_rights_ == FileAccess::kWrite) {
        file_size_ = 0;
      }

    } else {
      is_disk_open_ = false;
    }

    buffer_position_ = 0;
    buffer_file_position_ = 0;
    change_begin_ = -1;
    change_end_ = -1;
    file_position_ = 0;
    is_open_ = true;
  } else {
    RawFileClass::Open(rights);
  }

  return true;
}

/***********************************************************************************************
 * BufferIOFileClass::Write -- Writes data to the file cache. *
 *                                                                                             *
 *                                                                                             *
 * INPUT:   buffer   -- Pointer to the buffer that holds the data to be written.
 **
 *                                                                                             *
 *          size     -- The number of bytes to write. *
 *                                                                                             *
 * OUTPUT:  Returns the number of bytes actually written. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 11/15/1995 DRD : Created. *
 *=============================================================================================*/
int32_t BufferIOFileClass::Write(const void* buffer, int32_t size) {
  bool opened_here = false;

  if (!IsOpen()) {
    if (!Open(FileAccess::kWrite)) {
      return 0;
    }
    true_file_start_ = RawFileClass::Seek(0);
    opened_here = true;
  }

  if (use_buffer_) {
    int32_t bytes_written = 0;

    if (buffer_rights_ != FileAccess::kRead) {
      while (size) {
        int32_t chunk_size;

        if (size >= buffer_size_ - buffer_position_) {
          chunk_size = buffer_size_ - buffer_position_;
        } else {
          chunk_size = size;
        }

        if ((chunk_size != buffer_size_) && (!is_buffer_loaded_)) {
          int32_t read_size;

          if (file_size_ < buffer_size_) {
            read_size = file_size_;
            buffer_file_position_ = 0;
          } else {
            read_size = buffer_size_;
            buffer_file_position_ = file_position_;
          }

          if (true_file_start_) {
            use_buffer_ = false;
            Seek(file_position_, SEEK_SET);
            Read(buffer_, buffer_size_);
            Seek(file_position_, SEEK_SET);
            use_buffer_ = true;
          } else {
            RawFileClass::Seek(buffer_file_position_, SEEK_SET);
            RawFileClass::Read(buffer_, read_size);
          }

          buffer_position_ = 0;
          change_begin_ = -1;
          change_end_ = -1;

          is_buffer_loaded_ = true;
        }

        memmove(static_cast<char*>(buffer_) + buffer_position_,
                static_cast<const char*>(buffer) + bytes_written,
                base::ToSize(chunk_size));

        has_unwritten_changes_ = true;
        bytes_written += chunk_size;
        size -= chunk_size;

        if (change_begin_ == -1) {
          change_begin_ = buffer_position_;
          change_end_ = buffer_position_;
        } else {
          change_begin_ = std::min(change_begin_, buffer_position_);
        }

        buffer_position_ += chunk_size;

        change_end_ = std::max(change_end_, buffer_position_);

        file_position_ = buffer_file_position_ + buffer_position_;

        file_size_ = std::max(file_size_, file_position_);

        //
        // end of buffer reached?
        //
        if (buffer_position_ == buffer_size_) {
          Commit();

          buffer_position_ = 0;
          buffer_file_position_ = file_position_;
          change_begin_ = -1;
          change_end_ = -1;

          if (size && file_size_ > file_position_) {
            if (true_file_start_) {
              use_buffer_ = false;
              Seek(file_position_, SEEK_SET);
              Read(buffer_, buffer_size_);
              Seek(file_position_, SEEK_SET);
              use_buffer_ = true;
            } else {
              RawFileClass::Seek(file_position_, SEEK_SET);
              RawFileClass::Read(buffer_, buffer_size_);
            }
          } else {
            is_buffer_loaded_ = false;
          }
        }
      }
    } else {
      Error(EACCES);
    }

    size = bytes_written;
  } else {
    size = RawFileClass::Write(buffer, size);
  }

  if (opened_here) {
    Close();
  }

  return size;
}

/***********************************************************************************************
 * BufferIOFileClass::Read -- Reads data from the file cache. *
 *                                                                                             *
 *                                                                                             *
 * INPUT:   buffer   -- Pointer to the buffer to place the read data. *
 *                                                                                             *
 *          size     -- The number of bytes to read. *
 *                                                                                             *
 * OUTPUT:  Returns the actual number of bytes read (this could be less than
 *requested).       *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 11/15/1995 DRD : Created. *
 *=============================================================================================*/
int32_t BufferIOFileClass::Read(void* buffer, int32_t size) {
  bool opened_here = false;

  if ((!IsOpen()) && Open()) {
    true_file_start_ = RawFileClass::Seek(0);
    opened_here = true;
  }

  if (use_buffer_) {
    int32_t bytes_read = 0;

    if (buffer_rights_ != FileAccess::kWrite) {
      while (size) {
        int32_t chunk_size;

        if (size >= buffer_size_ - buffer_position_) {
          chunk_size = buffer_size_ - buffer_position_;
        } else {
          chunk_size = size;
        }

        if (!is_buffer_loaded_) {
          int32_t read_size;

          if (file_size_ < buffer_size_) {
            read_size = file_size_;
            buffer_file_position_ = 0;
          } else {
            read_size = buffer_size_;
            buffer_file_position_ = file_position_;
          }

          if (true_file_start_) {
            use_buffer_ = false;
            Seek(file_position_, SEEK_SET);
            Read(buffer_, buffer_size_);
            Seek(file_position_, SEEK_SET);
            use_buffer_ = true;
          } else {
            RawFileClass::Seek(buffer_file_position_, SEEK_SET);
            RawFileClass::Read(buffer_, read_size);
          }

          buffer_position_ = 0;
          change_begin_ = -1;
          change_end_ = -1;

          is_buffer_loaded_ = true;
        }

        memmove(static_cast<char*>(buffer) + bytes_read,
                static_cast<char*>(buffer_) + buffer_position_,
                base::ToSize(chunk_size));

        bytes_read += chunk_size;
        size -= chunk_size;
        buffer_position_ += chunk_size;
        file_position_ = buffer_file_position_ + buffer_position_;

        //
        // end of buffer reached?
        //
        if (buffer_position_ == buffer_size_) {
          Commit();

          buffer_position_ = 0;
          buffer_file_position_ = file_position_;
          change_begin_ = -1;
          change_end_ = -1;

          if (size && file_size_ > file_position_) {
            if (true_file_start_) {
              use_buffer_ = false;
              Seek(file_position_, SEEK_SET);
              Read(buffer_, buffer_size_);
              Seek(file_position_, SEEK_SET);
              use_buffer_ = true;
            } else {
              RawFileClass::Seek(file_position_, SEEK_SET);
              RawFileClass::Read(buffer_, buffer_size_);
            }
          } else {
            is_buffer_loaded_ = false;
          }
        }
      }
    } else {
      Error(EACCES);
    }

    size = bytes_read;
  } else {
    size = RawFileClass::Read(buffer, size);
  }

  if (opened_here) {
    Close();
  }

  return size;
}

/***********************************************************************************************
 * BufferIOFileClass::Seek -- Moves the current file pointer in the file. *
 *                                                                                             *
 *    This routine will change the current file pointer to the position
 *specified. It follows  * the same rules the a normal Seek() does, but if the
 *file is part of the mixfile system,  * then only the position value needs to
 *be updated.                                        *
 *                                                                                             *
 * INPUT:   offset      -- The position to move the file to relative to the
 *position indicated    * by the "origin" parameter. *
 *                                                                                             *
 *          origin      -- The direction to affect the position change against.
 *This can be       * either SEEK_CUR, SEEK_END, or SEEK_SET. *
 *                                                                                             *
 * OUTPUT:  Returns with the position of the new location. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 11/15/1995 DRD : Created. *
 *=============================================================================================*/
int32_t BufferIOFileClass::Seek(int32_t offset, int origin) {
  if (use_buffer_) {
    bool offset_was_absolute = false;

    switch (origin) {
      case SEEK_END:
        file_position_ = file_size_;
        break;

      case SEEK_SET:
        file_position_ = 0;
        break;

      case SEEK_CUR:
      default:
        break;
    }

    if (true_file_start_ && (offset >= true_file_start_)) {
      offset -= true_file_start_;
      offset_was_absolute = true;
    }

    file_position_ += offset;

    file_position_ = std::max<int32_t>(file_position_, 0);
    file_position_ = std::min(file_position_, file_size_);

    if (file_size_ <= buffer_size_) {
      buffer_position_ = file_position_;
    } else {
      if (file_position_ >= buffer_file_position_ &&
          file_position_ < buffer_file_position_ + buffer_size_) {
        buffer_position_ = file_position_ - buffer_file_position_;
      } else {
        Commit();
        // check!!
        if (true_file_start_) {
          use_buffer_ = false;
          Seek(file_position_, SEEK_SET);
          use_buffer_ = true;
        } else {
          RawFileClass::Seek(file_position_, SEEK_SET);
        }

        is_buffer_loaded_ = false;
      }
    }

    if (true_file_start_ && offset_was_absolute) {
      return file_position_ + true_file_start_;
    }

    return file_position_;
  }

  return RawFileClass::Seek(offset, origin);
}

/***********************************************************************************************
 * BufferIOFileClass::Size -- Determines size of file (in bytes). *
 *                                                                                             *
 *    If part or all of the file is cached, then the size of the file is already
 ** determined and available. Otherwise, go to the low level system to find the
 *file         * size. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the number of bytes in the file. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 11/14/1995 DRD : Created. *
 *=============================================================================================*/
int32_t BufferIOFileClass::Size() {
  if (is_open_ && use_buffer_) {
    return file_size_;
  }

  return RawFileClass::Size();
}

/***********************************************************************************************
 * BufferIOFileClass::Close -- Perform a closure of the file. *
 *                                                                                             *
 *    Call Commit() to write the buffer if the file is cached and the buffer has
 *changed,      * then call lower level Close(). *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 11/14/1995 DRD : Created. *
 *=============================================================================================*/
void BufferIOFileClass::Close() {
  if (use_buffer_) {
    Commit();

    if (is_disk_open_) {
      if (true_file_start_) {
        // Deliberately this class's Close with buffering switched off, not a
        // derived override, which would re-enter its own logic instead.
        use_buffer_ = false;
        BufferIOFileClass::Close();
        use_buffer_ = true;
      } else {
        RawFileClass::Close();
      }

      is_disk_open_ = false;
    }

    is_open_ = false;
  } else {
    RawFileClass::Close();
  }
}
