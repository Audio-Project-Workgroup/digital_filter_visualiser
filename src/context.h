/** this file provides a uniform set of preprocessor constants for detecting
 * compiler/operating system/cpu architecture triples.
 * supported compilers: clang, msvc, gcc.
 * supported operating systems: linux, macos, windows
 * supported cpu architectures: x86, x86_64, arm aarch32, arm aarch64
 */

// NOTE(ry): clang defines
#if defined(__clang__)
#  define COMPILER_CLANG 1

// NOTE(ry): clang  os
#  if defined(__gnu_linux__)
#    define OS_LINUX 1
#  elif defined(__APPLE__) && defined(__MACH__)
#    define OS_MAC 1
#  elif defined(_WIN32)
#    define OS_WINDOWS 1
#  else
#    error ERROR: os undetected
#  endif

// NOTE(ry): clang cpu arch
#  if defined(__amd64__)
#    define CPU_X64 1
#  elif defined(__i386__)
#    define CPU_X86 1
#  elif defined(__arm__)
#    define CPU_ARM 1
#  elif defined(__aarch64__)
#    define CPU_ARM64 1
#  else
#    error ERROR: cpu architecture undetected
#  endif

// NOTE(ry): msvc defines
#elif defined(_MSC_VER)
#  define COMPILER_MSVC 1


// NOTE(ry): msvc  os
#  if defined(_WIN32)
#    define OS_WINDOWS 1
#  else
#    error ERROR: os undetected
#  endif

// NOTE(ry): msvc cpu arch
#  if defined(_M_AMD64)
#    define CPU_X64 1
#  elif defined(_M_iX86)
#    define CPU_X86 1
#  elif defined(_M_ARM)
#    define CPU_ARM 1
#  elif defined(_M_ARM64)
#    define CPU_ARM64 1
#  else
#    error ERROR: cpu architecture undetected
#  endif

// NOTE(ry): gcc defines
#elif defined(__GNUC__)
#  define COMPILER_GCC 1

// NOTE(ry): gcc  os
#  if defined(__gnu_linux__)
#    define OS_LINUX 1
#  elif defined(__APPLE__) && defined(__MACH__)
#    define OS_MAC 1
#  elif defined(_WIN32)
#    define OS_WINDOWS 1
#  else
#    error ERROR: os undetected
#  endif

// NOTE(ry): gcc cpu arch
#  if defined(__amd64__)
#    define CPU_X64 1
#  elif defined(__i386__)
#    define CPU_X86 1
#  elif defined(__arm__)
#    define CPU_ARM 1
#  elif defined(__aarch64__)
#    define CPU_ARM64 1
#  else
#    error ERROR: cpu architecture undetected
#  endif

#else
#  error unsupported compiler
#endif

#if !defined(COMPILER_CLANG)
#  define COMPILER_CLANG 0
#endif

#if !defined(COMPILER_MSVC)
#  define COMPILER_MSVC 0
#endif

#if !defined(COMPILER_GCC)
#  define COMPILER_GCC 0
#endif

#if !defined(OS_LINUX)
#  define OS_LINUX 0
#endif

#if !defined(OS_MAC)
#  define OS_MAC 0
#endif

#if !defined(OS_WINDOWS)
#  define OS_WINDOWS 0
#endif

#if !defined(CPU_X64)
#  define CPU_X64 0
#endif

#if !defined(CPU_X86)
#  define CPU_X86 0
#endif

#if !defined(CPU_ARM)
#  define CPU_ARM 0
#endif

#if !defined(CPU_ARM64)
#  define CPU_ARM64 0
#endif

#if CPU_X64 || CPU_ARM64
#  define ARCH_64BIT 1
#  define ARCH_32BIT 0
#else
#  define ARCH_32BIT 1
#  define ARCH_64BIT 0
#endif
