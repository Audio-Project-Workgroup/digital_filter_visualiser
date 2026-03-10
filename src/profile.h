struct ProfileSite;
struct ProfiledScope;
struct Profiler
{
  Profiler(void);

  static ProfileSite *sites;
  static u64 siteCount;
  static u64 tscFreq;

  // TODO(ry): indices instead of pointers
  ProfileSite *currentParent;
  //ProfileSite *nextParent[MAX_SITES];

  u64 tscStart;
  u64 tscEnd;
};

/** a static location in code that may be hit mutliple times in different states
 * on different threads
 */
struct ProfileSite
{
  explicit ProfileSite(char const *_label) : label(_label)
  {}

  char const *label;

  /** the cumulative elapsed time stamp count accross all calls */
  u64 tscElapsed;
  /** the cumulative elapsed time stamp count of all children across all calls */
  u64 tscElapsedChildren;
  /** the elapsed time stamp count with this site as root */
  u64 tscElapsedRoot;

  u64 hitCount;

  u64 bytesAllocated;

  Profiler *usedThreadProfiler;
};

/** runtime state of profiled scopes
 */
struct ProfiledScope
{
  explicit ProfiledScope(ProfileSite *_site);
  ~ProfiledScope();

  u64 tscStart;
  u64 tscElapsedRootOld;
  ProfileSite *site;
  ProfileSite *parent;
};

// NOTE(ry): public interface

#if defined(DISABLE_PROFILING)
#  define PROFILE_SCOPE(name)
#  define PROFILE_FUNCTION()
#else
#  define GLUE_(a, b) a##b
#  define GLUE(a, b) GLUE_(a, b)
#  define PROFILE_SCOPE(name)\
  static PROFILE_SECTION ProfileSite GLUE(profile__site__, __LINE__)(name); \
  ProfiledScope GLUE(profile__scope__, __LINE__)(&GLUE(profile__site__, __LINE__));
#  define PROFILE_FUNCTION() PROFILE_SCOPE(__func__)
#endif
