#if TEST_NUM == 203
  #include "playlist.h"
  #include "playlist.h"

  #include <cstddef>

  // To jest inny szablon niż ten w zadaniu.
  template <typename T> class playlist {
  public:
    T t;

    playlist(T t) : t(t) {};
  };

  ::cxx::playlist<char, char> b;

  size_t f203() {
    return b.size();
  }
#endif
