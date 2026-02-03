# Include What You Use (IWYU) Integration

## Overview

This project uses [Include-What-You-Use (IWYU)](https://include-what-you-use.org/)
to analyze C++ include directives and ensure proper header dependencies.

## Installation

### Linux (Ubuntu/Debian)
```bash
sudo apt install iwyu
```

### Linux (Fedora)
```bash
sudo dnf install iwyu
```

### macOS
```bash
brew install include-what-you-use
```

### Windows
IWYU is not officially supported on Windows/MSVC. It is automatically disabled
for Windows builds.

## Usage

### Building with IWYU (Default)
```bash
cmake -Bbuild -G Ninja
JOBS=$(($(getconf _NPROCESSORS_ONLN) / 2)) && cmake --build build --parallel $JOBS
```

IWYU warnings will appear during compilation but won't fail the build.

### Disabling IWYU
For faster builds without IWYU analysis:
```bash
cmake -Bbuild -G Ninja -DENABLE_IWYU=OFF
JOBS=$(($(getconf _NPROCESSORS_ONLN) / 2)) && cmake --build build --parallel $JOBS
```

### Custom IWYU Path
If IWYU is installed in a non-standard location:
```bash
cmake -Bbuild -G Ninja -DIWYU_PATH=/path/to/include-what-you-use
JOBS=$(($(getconf _NPROCESSORS_ONLN) / 2)) && cmake --build build --parallel $JOBS
```

## Understanding IWYU Output

IWYU will produce warnings like:
```
/path/to/file.cpp:42:1: warning: #includes are not sorted properly
```

These are informational and do not block compilation.

### Common Messages

- **"should add these lines"** - Suggests adding missing includes
- **"should remove these lines"** - Suggests removing unnecessary includes
- **"#includes are not sorted"** - Suggests include order changes

## IWYU Pragmas

You can control IWYU behavior with special comments:

```cpp
// Keep a specific include
#include "something.h"  // IWYU pragma: keep

// Export an include (make it "public")
#include "internal.h"  // IWYU pragma: export

// Skip IWYU analysis for a file
// IWYU pragma: private, include "public_header.h"
```

## Configuration

### Mappings File
Project-specific IWYU mappings are in `.iwyu_mappings` at the repository root.
This file tells IWYU about:
- SDL2 header relationships
- Standard library header mappings
- Project-specific header patterns

### Customizing IWYU Options
Edit `cmake/IWYU.cmake` and modify the `IWYU_COMMAND` variable.

Common options:
- `--max_line_length=N` - Maximum suggested line length
- `--no_fwd_decls` - Don't suggest forward declarations
- `--cxx17ns` - Use C++17 nested namespace syntax

## CI/CD Integration

IWYU is automatically enabled in GitHub Actions for Linux/macOS builds.
The CI build will show IWYU warnings but won't fail on them.

## Troubleshooting

### "include-what-you-use not found"
Install IWYU using the instructions above.

### Too many false positives
1. Add project-specific mappings to `.iwyu_mappings`
2. Use `// IWYU pragma: keep` comments
3. Consider disabling for specific legacy files

### IWYU causes build errors
IWYU should only produce warnings. If builds fail:
1. Check CMake output for actual compilation errors
2. Try `cmake -DENABLE_IWYU=OFF` to isolate the issue
3. Report the issue with IWYU output

## References

- [IWYU Documentation](https://include-what-you-use.org/)
- [IWYU Pragmas](https://github.com/include-what-you-use/include-what-you-use/blob/master/docs/IWYUPragmas.md)
- [IWYU Mappings](https://github.com/include-what-you-use/include-what-you-use/blob/master/docs/IWYUMappings.md)
