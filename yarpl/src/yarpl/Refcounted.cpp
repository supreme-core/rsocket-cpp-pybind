#include "yarpl/Refcounted.h"

#include "folly/Synchronized.h"

#include <algorithm>
#include <iomanip>

namespace yarpl {
namespace detail {

using sync_map_type = folly::Synchronized<refcount_map_type>;
using refcount_pair = std::pair<std::string, int64_t>;

#ifdef YARPL_REFCOUNT_DEBUGGING

// number of objects currently live of this type
sync_map_type live_refcounted_map;

// number of objects ever created of this type
sync_map_type total_refcounted_map;

static void inc_in_map(std::string const& typestring, sync_map_type& the_map) {
  auto map = the_map.wlock();
  auto it = map->find(typestring);
  if (it == map->end()) {
    map->emplace(typestring, 1);
    it = map->find(typestring);
  }
  else {
    it->second = it->second + 1;
  }

  VLOG(6) << "increment " << typestring << " to " << it->second;
}

static void dec_in_map(std::string const& typestring, sync_map_type& the_map) {
  auto map = the_map.wlock();
  auto it = map->find(typestring);
  if (it == map->end()) {
    VLOG(6) << "didn't find " << typestring << " in the map";
    return;
  }
  else {
    if (it->second >= 1) {
      it->second = it->second - 1;
    }
    else {
      VLOG(6) << "deallocating " << typestring << " past zero?";
      return;
    }
  }

  VLOG(6) << "decrement " << typestring << " to " << it->second;
}

void inc_live(std::string const& typestring) { inc_in_map(typestring, live_refcounted_map); }
void dec_live(std::string const& typestring) { dec_in_map(typestring, live_refcounted_map); }

void inc_created(std::string const& typestring) { inc_in_map(typestring, total_refcounted_map); }

template <typename ComparePred>
void debug_refcounts_map(std::ostream& o, sync_map_type const& the_map, ComparePred& pred) {
  // truncate demangled typename
  auto const max_type_len = 50;
  // only print the first 'n' entries
  auto max_entries = 50;

  auto the_map_locked = the_map.rlock();
  std::vector<refcount_pair> entries(the_map_locked->begin(), the_map_locked->end());
  std::sort(entries.begin(), entries.end(), pred);

  for(auto& pair : entries) {
    if (!max_entries--) break;

    auto s = pair.first;
    if (s.size() > max_type_len) {
      s = s.substr(0, max_type_len);
    }
    o << std::left << std::setw (max_type_len) << s << " :: " << pair.second << std::endl;
  }
}

void debug_refcounts(std::ostream& o) {
  struct {
    bool operator()(refcount_pair const& a, refcount_pair const& b) {
      return a.second > b.second;
    }
  } max_refcount_pred;

  o << "===============" << std::endl;
  o << "LIVE REFCOUNTS: " << std::endl;
  debug_refcounts_map(o, live_refcounted_map, max_refcount_pred);
  o << "===============" << std::endl;
  o << "===============" << std::endl;
  o << "TOTAL REFCOUNTS: " << std::endl;
  debug_refcounts_map(o, total_refcounted_map, max_refcount_pred);
  o << "===============" << std::endl;
}

#else /* YARPL_REFCOUNT_DEBUGGING */

void inc_created(std::string const&) { assert(false); }
void inc_live(std::string const&) {  assert(false); }
void dec_live(std::string const&) {  assert(false); }
void debug_refcounts(std::ostream& o) {
  o << "Compile with YARPL_REFCOUNT_DEBUGGING (-DYARPL_REFCOUNT_DEBUGGING=On) to get Refcounted allocation counts" << std::endl;
}

#endif

} }
