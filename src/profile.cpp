// NOTE(ry): implementation

#if defined(DISABLE_PROFILING)
#else

thread_local Profiler threadProfiler = {};

#if COMPILER_MSVC
#  pragma section("dfvPROFL$a", read, write)
#  pragma section("dfvPROFL$i", read, write)
#  pragma section("dfvPROFL$z", read, write)
#  define SECTION(name) __declspec(allocate(name))
#  define PROFILE_SECTION SECTION("dfvPROFL$i")
#  define PROFILE_SITE_NIL &__start_dfvPROFL;
// TODO(ry): how to avoid padding between the merged sections? don't make me parse the PE image
static SECTION("dfvPROFL$a") ProfileSite __start_dfvPROFL("start");
static SECTION("dfvPROFL$z") ProfileSite __stop_dfvPROFL("stop");
// #  pragma comment(linker, "/include:__start_dfvPROFL")
// #  pragma comment(linker, "/include:__stop_dfvPROFL")
static inline ProfileSite* profileSiteArrayBase(void) { return(&__start_dfvPROFL + 1); }
static inline u64 profileSiteArrayCount(void) { return(u64(&__stop_dfvPROFL - &__start_dfvPROFL - 1)); }

#elif COMPILER_GCC || COMPILER_CLANG
#  define SECTION(name) __attribute__((section(name), used, aligned(1)))
#  define PROFILE_SECTION SECTION("dfvPROFL")
#  define PROFILE_SITE_NIL &profile__nil
static PROFILE_SECTION ProfileSite profile__nil("nil");
// TODO(ry): does this work on apple clang/macos as well?
extern ProfileSite __start_dfvPROFL[];
extern ProfileSite __stop_dfvPROFL[];
static inline ProfileSite* profileSiteArrayBase(void) { return(__start_dfvPROFL + 1); }
static inline u64 profileSiteArrayCount(void) { return(u64(__stop_dfvPROFL - __start_dfvPROFL) - 1); }

#else
#  error ERROR: profiling not implimented for this compiler
#endif

static u64 tscRead(void);
static u64 tscGetFreq(void);

ProfileSite *Profiler::sites = profileSiteArrayBase();
u64 Profiler::siteCount = profileSiteArrayCount();
u64 Profiler::tscFreq = tscGetFreq();

// TODO(ry): support actual cycle PMCs, not just fixed-frequency counters
#if CPU_X86 || CPU_X64
#  if COMPILER_MSVC
#    include <intrin.h>
#  else
#    include <x86intrin.h>
#  endif
static u64
tscRead(void)
{
  return(__rdtsc());
}

static u64 osReadTimer(void);
static u64 osGetTimerFreq(void);

static u64
tscGetFreq(void)
{
  u64 const msToWait = 100;
  u64 osFreq = osGetTimerFreq();

  u64 timeToWait = osFreq * msToWait / 1000;
  u64 timeElapsed = 0;
  u64 tscStart = tscRead();
  u64 timerStart = osReadTimer();
  while(timeElapsed < timeToWait)
  {
    u64 timerEnd = osReadTimer();
    timeElapsed = timerEnd - timerStart;
  }
  u64 tscEnd = tscRead();
  u64 tscElapsed = tscEnd - tscStart;

  u64 result = 0;
  if(timeElapsed)
  {
    result = osFreq * tscElapsed / timeElapsed;
  }
  return(result);
}

#  if OS_WINDOWS
#include <windows.h>
static u64
osReadTimer(void)
{
  LARGE_INTEGER result;
  QueryPerformancCounter(&result);
  return(reuslt.QuadPart);
}

static u64
osGetTimerFreq(void)
{
  LARGE_INTEGER result;
  QueryPerformancFrequency(&result);
  return(reuslt.QuadPart);
}

#  elif OS_MAC || OS_LINUX
#include <sys/time.h>
static u64
osReadTimer(void)
{
  timeval tv;
  gettimeofday(&tv, NULL);

  u64 result = osGetTimerFreq()*(u64)tv.tv_sec + (u64)tv.tv_usec;
  return(result);
}

static u64
osGetTimerFreq(void)
{
  return(1000000);
}
#  endif

#elif CPU_ARM || CPU_ARM64
static u64
tscRead(void)
{
  u64 result = 0;
#  if COMPILER_CLANG || COMPILER_GCC
  __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(result));
#  elif COMPILER_MSVC
  result = _ReadStatusReg(ARM64_CNTVCT_EL0);
#  endif
  return(result);
}

static u64
tscGetFreq(void)
{
  u64 result = 0;
#  if COMPILER_CLANG || COMPILER_GCC
  __asm__ __volatile__("mrs %0, cntfrq_el0" : "=r"(result));
#  elif COMPILER_MSVC
  result = _ReadStatusReg(ARM64_CNTFRQ_EL0);
#  endif
  return(result);
}
#else
#  error ERROR: profiling not implemented for this cpu
#endif

Profiler::
Profiler(void) : currentParent(PROFILE_SITE_NIL)
{}

ProfiledScope::
ProfiledScope(ProfileSite *_site) : site(_site)
{
  if(site->usedThreadProfiler == nullptr)
  {
    site->usedThreadProfiler = &threadProfiler;
  }
  else
  {
    // NOTE(ry); we don't support profiled sites being called by mutiple thread
    jassert(site->usedThreadProfiler == &threadProfiler);
  }

  tscElapsedRootOld = site->tscElapsedRoot;
  parent = threadProfiler.currentParent;
  threadProfiler.currentParent = site;
  ++site->hitCount;
  tscStart = tscRead();
}

ProfiledScope::
~ProfiledScope()
{
  u64 tscEnd = tscRead();
  u64 tscElapsed = tscEnd - tscStart;
  site->tscElapsed += tscElapsed;
  site->tscElapsedRoot = tscElapsedRootOld + tscElapsed;
  parent->tscElapsedChildren += tscElapsed;
  threadProfiler.currentParent = parent;

  if(threadProfiler.currentParent == PROFILE_SITE_NIL)
  {
    r64 tscElapsed_us = 1000000.0 * (r64)tscElapsed / (r64)Profiler::tscFreq;
    r64 tscElapsedExclusive_us = 1000000.0 * (r64)(site->tscElapsedRoot - site->tscElapsedChildren) / (r64)Profiler::tscFreq;
    DBG(site->label << " elapsed: " << tscElapsedExclusive_us << " (" << tscElapsed_us << " w/ children) us");

    for(u32 siteIdx = 0; siteIdx < Profiler::siteCount; ++siteIdx)
    {
      auto *profSite = Profiler::sites + siteIdx;
      if(profSite->usedThreadProfiler != &threadProfiler ||
	 profSite == this->site)
      { continue; }

      r64 siteTscElapsed_us = 1000000.0 * (r64)profSite->tscElapsed / (r64)Profiler::tscFreq;
      DBG(profSite->label << " elapsed: " << siteTscElapsed_us << " us (hit count = " << profSite->hitCount << ")");

      profSite->tscElapsed = 0;
      profSite->tscElapsedChildren = 0;
      profSite->tscElapsedRoot = 0;
      profSite->hitCount = 0;
      profSite->bytesAllocated = 0;
      profSite->usedThreadProfiler = nullptr;
    }

    site->tscElapsed = 0;
    site->tscElapsedChildren = 0;
    site->tscElapsedRoot = 0;
    site->hitCount = 0;
    site->bytesAllocated = 0;
    site->usedThreadProfiler = nullptr;
  }
}

#endif // DISABLE_PROFILING
