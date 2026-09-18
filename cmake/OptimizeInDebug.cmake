# optimize_in_debug(<source>...)
#
# Compiles the given sources of the current directory at -O2 even in Debug
# builds. For the byte- and pixel-crunching kernels only: at -O0 every
# base::At, span accessor and std::ranges helper in their inner loops is a
# real out-of-line call. Game logic stays at -O0 so it can be stepped through.
#
# Measured 2026-09-17, Red Alert, headless save load: Debug took 0.72 s
# against 0.12 s at RelWithDebInfo, and these kernels were 72% of its CPU
# (RSA bignum 24%, Blowfish 15%, the fading table 15%, SHA-1 14%, LCW 3%,
# the window blit 2%).
function(optimize_in_debug)
    set_property(SOURCE ${ARGN} APPEND PROPERTY COMPILE_OPTIONS
            "$<$<CONFIG:Debug>:-O2>")
endfunction()
