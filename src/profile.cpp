// NOTE(ry): implementation

static ProfileConfig profileConfig{
  ProfileFormat::tui,
  {&std::cerr,
#if OS_WINDOWS
   [](void *data){
	 juce::ignoreUnused(data);
	 OutputDebugStringA(profileGetLog());
	 OutptuDebugStringA('\n');
   }
#else
   [](void *data){
	 auto outStream = reinterpret_cast<std::ostream*>(data);
	 *outStream << std::string_view(profileFlushLog());
   }
#endif
  }
};

static void
profileConfigure(ProfileConfig cfg)
{
  profileConfig = cfg;
}

thread_local Profiler threadProfiler = {};

struct ProfilerLog
{
  size_t logAt;
  char logBuffer[1 << 15];
};

thread_local ProfilerLog threadProfilerLog = {};

static char*
profileFlushLog(void)
{
  threadProfilerLog.logAt = 0;
  return threadProfilerLog.logBuffer;
}

static void
profileLogFormatString(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  size_t bytesRemaining = sizeof(threadProfilerLog.logBuffer) - threadProfilerLog.logAt;
  char *buffer = threadProfilerLog.logBuffer + threadProfilerLog.logAt;
  int bytesWritten = vsnprintf(buffer, bytesRemaining, fmt, args);
  threadProfilerLog.logAt += size_t(bytesWritten);

  va_end(args);
}

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
#define NOMINMAX
#include <windows.h>
static u64
osReadTimer(void)
{
  LARGE_INTEGER result;
  QueryPerformanceCounter(&result);
  return(result.QuadPart);
}

static u64
osGetTimerFreq(void)
{
  LARGE_INTEGER result;
  QueryPerformanceFrequency(&result);
  return(result.QuadPart);
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
  // TODO(ry): we shouldn't have any branches in the profiler
  if(site->usedThreadProfiler == nullptr)
  {
    site->usedThreadProfiler = &threadProfiler;
  }
  else
  {
    // NOTE(ry); we don't support profiled sites being called by mutiple threads
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

	profileLogFormatString("%s elapsed: %.2f us (%.2f us w/ children)\n",
						   site->label, tscElapsedExclusive_us, tscElapsed_us);

    for(u32 siteIdx = 0; siteIdx < Profiler::siteCount; ++siteIdx)
    {
      auto *profSite = Profiler::sites + siteIdx;
      if(profSite->usedThreadProfiler != &threadProfiler ||
		 profSite == this->site)
      { continue; }

      r64 siteTscElapsed_us = 1000000.0 * (r64)profSite->tscElapsed / (r64)Profiler::tscFreq;
	  profileLogFormatString("%s elapsed: %.2f us (hit count = %lu)\n",
							 profSite->label, siteTscElapsed_us, profSite->hitCount);

      profSite->tscElapsedMax = std::max(profSite->tscElapsedMax, profSite->tscElapsed);
      profSite->tscElapsed = 0;
      profSite->tscElapsedChildren = 0;
      profSite->tscElapsedRoot = 0;
      profSite->hitCount = 0;
      profSite->bytesAllocated = 0;
      profSite->usedThreadProfiler = nullptr;
    }

	profileConfig.dst.onTopScopeExit(profileConfig.dst.userData);

    site->tscElapsedMax = std::max(site->tscElapsedMax, site->tscElapsed);
    site->tscElapsed = 0;
    site->tscElapsedChildren = 0;
    site->tscElapsedRoot = 0;
    site->hitCount = 0;
    site->bytesAllocated = 0;
    site->usedThreadProfiler = nullptr;
  }
}
