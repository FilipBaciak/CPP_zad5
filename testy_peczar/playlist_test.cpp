// Kompilowanie i uruchamianie:
// g++ -Wall -Wextra -O2 -std=c++23 -DTEST_NUM=... playlist_test*.cpp -o playlist_test && ./playlist_test && valgrind --error-exitcode=123 --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all --run-cxx-freeres=yes -q ./playlist_test

#include "playlist.h"

#ifdef NDEBUG
  #undef NDEBUG
#endif

#include <cassert>
#include <cinttypes>
#include <cstddef>
#include <cstdlib>
#include <array>
#include <iterator>
#include <iostream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

// using std::advance;
using std::align_val_t;
using std::array;
using std::as_const;
using std::bad_alloc;
using std::cerr;
using std::clog;
using std::cout;
using std::exception;
using std::invalid_argument;
using std::make_unique;
using std::move;
using std::ostream;
using std::ostringstream;
using std::out_of_range;
using std::pair;
using std::string_view;
using std::size_t;
using std::swap;
using std::vector;

#if TEST_NUM == 203
  size_t f203();
#else
  using cxx::playlist;
#endif

int main();

namespace {
  template <class It>
  void advance(It &it, size_t n) {
    while (n > 0) {
      --n;
      ++it;
    }
  }

#if TEST_NUM == 101 || TEST_NUM == 102 || TEST_NUM == 103
  [[maybe_unused]] const unsigned BIG_VALUE = 100000;

  // Plejlista pewnego radia zawiera nazwy utworów oraz liczone od początku
  // utworu czasy w sekundach rozpoczęcia i zakończenia odtwarzania.
  using params_t = pair<unsigned, unsigned>;
  using radio_t = playlist<string_view, params_t>;
  using play_return_t = std::pair<string_view const &, params_t const &>;
  using pay_return_t = std::pair<string_view const &, size_t>;

  [[maybe_unused]] ostream & operator<<(ostream &os, play_return_t const &p) {
    os << p.first << ' ' << p.second.first << ':' << p.second.second << '\n';
    return os;
  }

  [[maybe_unused]] ostream & operator<<(ostream &os, pay_return_t const &p) {
    os << p.first << ' ' << p.second << '\n';
    return os;
  }

  [[maybe_unused]] array<string_view, 4> tracks = {"zerowe", "pierwsze",
                                                   "drugie", "trzecie"};
  [[maybe_unused]] array<params_t, 7> params = {{{0, 0}, {0, 1}, {0, 2}, {0, 3},
                                                 {0, 4}, {0, 5}, {0, 6}}};

  // Symulujemy odtwarzanie plajlisty.
  template<typename T, typename P>
  void play(playlist<T, P> const &pl) {
    using pit_t = playlist<T, P>::play_iterator;
    for (pit_t pit = pl.play_begin(); pit != pl.play_end(); ++pit)
      clog << pl.play(pit);
  }

  // Plejlistę można też odtwarzać w inny sposób.
  template<typename T, typename P>
  void lay(playlist<T, P> &pl) {
    while (pl.size() > 0) {
      clog << pl.front();
      pl.pop_front();
    }
  }

  // Wypisujemy, ile komu zapłacić za odtwarzanie jego utworu.
  template<typename T, typename P>
  void pay(playlist<T, P> const &pl) {
    typename playlist<T, P>::sorted_iterator pit = pl.sorted_begin();
    while (pit != pl.sorted_end())
      clog << pl.pay(pit++);
  }
#endif

#if (TEST_NUM > 400 && TEST_NUM <= 599) || TEST_NUM == 606
  // Sprawdzamy równość plejlist.
  template <typename T, typename P>
  bool operator==(playlist<T, P> const &pl1, playlist<T, P> const &pl2) {
    size_t pl1_size = pl1.size();
    size_t pl2_size = pl2.size();

    if (pl1_size != pl2_size)
      return false;

    typename playlist<T, P>::play_iterator
      it11 = pl1.play_begin(), end11 = pl1.play_end(),
      it12 = pl2.play_begin(), end12 = pl2.play_end();
    for (; it11 != end11 && it12 != end12; ++it11, ++it12)
      if (pl1.play(it11) != pl2.play(it12))
        return false;
    if (it11 != end11 || it12 != end12)
      return false;

    typename playlist<T, P>::sorted_iterator
      it21 = pl1.sorted_begin(), end21 = pl1.sorted_end(),
      it22 = pl2.sorted_begin(), end22 = pl2.sorted_end();
    for (; it21 != end21 && it22 != end22; ++it21, ++it22)
      if (pl1.pay(it21) != pl2.pay(it22))
        return false;
    if (it21 != end21 || it22 != end22)
      return false;

    return true;
  }
#endif

#if TEST_NUM > 300
  int throw_countdown;
  bool throw_checking = false;

  void ThisCanThrow(int x) {
    if (throw_checking && --throw_countdown <= 0)
      throw x;
  }

  class Track {
    friend int ::main();
    friend void log(char const *);

  public:
    Track(Track const &other) {
      ThisCanThrow(10);
      value = move(other.value);
      ++instance_count;
      ++operation_count;
      ++copy_count;
    }

    Track(Track &&other) {
      value = move(other.value);
      ++operation_count;
    }

    Track & operator=(Track other) {
      ThisCanThrow(11);
      value = move(other.value);
      ++operation_count;
      return *this;
    }

    ~Track() {
      --instance_count;
      ++operation_count;
    }

    auto operator<=>(Track const &other) const {
      ThisCanThrow(12);
      ++operation_count;
      return value <=> other.value;
    }

    bool operator==(Track const &other) const {
      ThisCanThrow(13);
      ++operation_count;
      return value == other.value;
    }

  private:
    inline static size_t instance_count = 0;
    inline static size_t operation_count = 0;
    inline static size_t copy_count = 0;
    size_t value;

    explicit Track(size_t value) : value(value) {
      ThisCanThrow(14);
      ++instance_count;
      ++operation_count;
    }
  };

  class Params {
    friend int ::main();
    friend void log(char const *);

  public:
    Params(Params const &other) {
      ThisCanThrow(20);
      value = other.value;
      ++instance_count;
      ++operation_count;
      ++copy_count;
    }

    ~Params() {
      --instance_count;
      ++operation_count;
    }

    Params() = delete;
    Params(Params &&other) = delete;
    Params & operator=(Params const &other) = delete;
    Params & operator=(Params &&other) = delete;

  private:
    inline static size_t instance_count = 0;
    inline static size_t operation_count = 0;
    inline static size_t copy_count = 0;
    size_t value;

    explicit Params(size_t value) : value(value) {
      ++instance_count;
      ++operation_count;
    }

  #if TEST_NUM >= 403 && TEST_NUM <= 599
  public:
    auto operator<=>(Params const &other) const {
      ++operation_count;
      return value <=> other.value;
    }

    bool operator==(Params const &other) const {
      ++operation_count;
      return value == other.value;
    }
  #endif
  };

  [[maybe_unused]] void log(char const *title) {
    clog << title << '\n';
    clog << "Liczba przechowywanych utworów:    " << Track::instance_count << '\n';
    clog << "Liczba operacji na utworach:       " << Track::operation_count << '\n';
    clog << "Liczba kopiowań utworów:           " << Track::copy_count << '\n';
    clog << "Liczba przechowywanych parametrów: " << Params::instance_count << '\n';
    clog << "Liczba operacji na parametrach:    " << Params::operation_count << '\n';
    clog << "Liczba kopiowań parametrów:        " << Params::copy_count << '\n';
  }
#endif

#if TEST_NUM == 401
  template <typename Exception, typename Playlist, typename Operation>
  void ThrowCheck(Playlist &b, Operation const &op, char const *name) {
    try {
      op(b);
    }
    catch (Exception const &exception) {
      clog << exception.what() << '\n';
      return;
    }
    catch (...) {
      clog << "Operacja " << name << " zgłosiła nieprawidłowy wyjątek.\n";
      exit(2);
    }
    clog << "Operacja " << name << " nie zgłosiła wyjątku.\n";
    exit(2);
  }
#endif

#if TEST_NUM >= 402 && TEST_NUM <= 499
  template <typename Playlist, typename Operation>
  void NoThrowCheck(Playlist &b, Operation const &op, char const *name) {
    try {
      throw_countdown = 0;
      throw_checking = true;
      op(b);
      throw_checking = false;
    }
    catch (exception const &e) {
      throw_checking = false;
      clog << "Operacja " << name << " zgłosiła wyjątek " << e.what() <<".\n";
      exit(3);
    }
    catch (int e) {
      throw_checking = false;
      clog << "Operacja " << name << " zgłosiłą wyjątek " << e << ".\n";
      exit(3);
    }
    catch (...) {
      throw_checking = false;
      clog << "Operacja " << name << " zgłosiła nieoczekiwany wyjątek.\n";
      exit(3);
    }
  }
#endif

#if TEST_NUM > 500 && TEST_NUM <= 599
  template <typename Playlist, typename Operation>
  bool StrongCheck(Playlist &b, Playlist const &d, Operation const &op, char const *name) {
    bool succeeded = false;

    try {
      throw_checking = true;
      op(b);
      throw_checking = false;
      succeeded = true;
    }
    catch (...) {
      throw_checking = false;
      bool unchanged = d == b;
      if (!unchanged) {
        clog << "Operacja " << name << " nie daje silnej gwarancji\n.";
        exit(4);
      }
    }

    return succeeded;
  }
#endif
} // koniec anonimowej przestrzeni nazw

// Operatory new nie mogą być deklarowane w anonimowej przestrzeni nazw.
#if TEST_NUM > 400 && TEST_NUM <= 599
void* operator new(size_t size) {
  try {
    ThisCanThrow(30);
    void* p = malloc(size);
    if (!p)
      throw "malloc";
    return p;
  }
  catch (...) {
    throw bad_alloc();
  }
}

void* operator new(size_t size, align_val_t al) {
  try {
    ThisCanThrow(31);
    void* p = aligned_alloc(static_cast<size_t>(al), size);
    if (!p)
      throw "aligned_alloc";
    return p;
  }
  catch (...) {
    throw bad_alloc();
  }
}

void* operator new[](size_t size) {
  try {
    ThisCanThrow(32);
    void* p = malloc(size);
    if (!p)
      throw "malloc";
    return p;
  }
  catch (...) {
    throw bad_alloc();
  }
}

void* operator new[](size_t size, align_val_t al) {
  try {
    ThisCanThrow(33);
    void* p = aligned_alloc(static_cast<size_t>(al), size);
    if (!p)
      throw "aligned_alloc";
    return p;
  }
  catch (...) {
    throw bad_alloc();
  }
}
#endif

int main() {
// Sprawdzamy przykład dołączony do treści zadania z pominięciem wyjątków.
#if TEST_NUM == 101
  ostringstream pout;
  ostringstream nout;
  auto cerr_buffer = cerr.rdbuf();
  auto clog_buffer = clog.rdbuf();
  auto cout_buffer = cout.rdbuf();
  cerr.rdbuf(nout.rdbuf()); // Od teraz cerr wysyła wszystko do nout.
  clog.rdbuf(pout.rdbuf()); // Od teraz clog wysyła wszystko do pout.
  cout.rdbuf(nout.rdbuf()); // Od teraz cout wysyła wszystko do nout.

  radio_t playlist1;

  assert(playlist1.size() == 0);

  for (size_t i = 0; i < tracks.size(); ++i)
     playlist1.push_back(tracks[i], params[i]);
  playlist1.push_back(tracks[1], params[4]);
  playlist1.push_back(tracks[1], params[5]);
  playlist1.push_back(tracks[0], params[6]);

  assert(playlist1.size() == tracks.size() + 3);

  clog << "# Odtwarzamy pierwszy raz.\n";
  play(playlist1);
  clog << "# Płacimy.\n";
  pay(playlist1);
  clog << "# Odtwarzamy drugi raz, usuwając utwory.\n";
  lay(playlist1);

  assert(playlist1.size() == 0);

  clog << "# Dodajemy utwory i odtwarzamy trzy początkowe.\n";
  playlist1.push_back(tracks[3], params[0]);
  playlist1.push_back(tracks[2], params[1]);
  playlist1.push_back(tracks[3], params[2]);
  playlist1.push_back(tracks[2], params[3]);
  playlist1.push_back(tracks[1], params[4]);
  auto it1 = playlist1.play_begin();
  clog << playlist1.play(it1++);
  clog << playlist1.play(it1++);
  clog << playlist1.play(it1);

  clog << "# Zmieniamy parametry i odtwarzamy całość.\n";
  auto &p1 = playlist1.params(it1);
  p1 = {17, 52};
  play(playlist1);

  clog << "# Musimy zapłacić.\n";
  auto it2 = playlist1.sorted_begin();
  clog << playlist1.pay(++it2);
  clog << playlist1.pay(++it2);

  clog << "# Usuwamy jeden utwór i odtwarzamy.\n";
  playlist1.remove(tracks[3]);
  play(playlist1);

  clog << "# Płacimy za ostatnie odtworzenia.\n";
  pay(playlist1);

  radio_t playlist2;
  vector<radio_t> vec;
  for (unsigned i = 0; i < BIG_VALUE; i++)
    playlist2.push_back(tracks[0], {0, i});
  assert(playlist2.size() == BIG_VALUE);
  for (unsigned i = 0; i < 10 * BIG_VALUE; i++)
    vec.push_back(playlist2); // Wszystkie oplekty w vec współdzielą dane.

  cerr.rdbuf(cerr_buffer); // Przywraca normale działanie cerr.
  clog.rdbuf(clog_buffer); // Przywraca normale działanie clog.
  cout.rdbuf(cout_buffer); // Przywraca normale działanie cout.

  assert(pout.str().compare(
    "# Odtwarzamy pierwszy raz.\n"
    "zerowe 0:0\n"
    "pierwsze 0:1\n"
    "drugie 0:2\n"
    "trzecie 0:3\n"
    "pierwsze 0:4\n"
    "pierwsze 0:5\n"
    "zerowe 0:6\n"
    "# Płacimy.\n"
    "drugie 1\n"
    "pierwsze 3\n"
    "trzecie 1\n"
    "zerowe 2\n"
    "# Odtwarzamy drugi raz, usuwając utwory.\n"
    "zerowe 0:0\n"
    "pierwsze 0:1\n"
    "drugie 0:2\n"
    "trzecie 0:3\n"
    "pierwsze 0:4\n"
    "pierwsze 0:5\n"
    "zerowe 0:6\n"
    "# Dodajemy utwory i odtwarzamy trzy początkowe.\n"
    "trzecie 0:0\n"
    "drugie 0:1\n"
    "trzecie 0:2\n"
    "# Zmieniamy parametry i odtwarzamy całość.\n"
    "trzecie 0:0\n"
    "drugie 0:1\n"
    "trzecie 17:52\n"
    "drugie 0:3\n"
    "pierwsze 0:4\n"
    "# Musimy zapłacić.\n"
    "pierwsze 1\n"
    "trzecie 2\n"
    "# Usuwamy jeden utwór i odtwarzamy.\n"
    "drugie 0:1\n"
    "drugie 0:3\n"
    "pierwsze 0:4\n"
    "# Płacimy za ostatnie odtworzenia.\n"
    "drugie 2\n"
    "pierwsze 1\n"
  ) == 0);

  assert(nout.str().empty());
#endif

// Sprawdzamy, czy iteratory osiągają end.
#if TEST_NUM == 102
  radio_t playlist1;

  for (size_t i = 0; i < tracks.size(); ++i)
     playlist1.push_back(tracks[i], params[i]);
   playlist1.push_back(tracks[0], params[0]);

  auto ip1 = playlist1.play_begin();
  auto ip2 = playlist1.play_end();
  auto is1 = playlist1.sorted_begin();
  auto is2 = playlist1.sorted_end();

  advance(ip1, 5);
  assert(ip1 == ip2);

  advance(is1, 4);
  assert(is1 == is2);
#endif

// Sprawdzamy z przykładu użycia fragment, który ma się nie kompilować.
#if TEST_NUM == 103
  radio_t playlist1;

  playlist1.push_back(tracks[3], params[0]);
  playlist1.push_back(tracks[2], params[1]);
  playlist1.push_back(tracks[3], params[2]);
  playlist1.push_back(tracks[2], params[3]);
  playlist1.push_back(tracks[1], params[4]);

  auto it1 = playlist1.play_begin();

  [[maybe_unused]] auto &p2 = as_const(playlist1).params(it1);
  p2 = {121, 177}; // Nie kompiluje się.
#endif

// Testujemy kopiowanie, przenoszenie i czyszczenie.
#if TEST_NUM == 201
  using playlist_t = playlist<int8_t, int8_t>;

  static const int8_t tracks[] = {1, -4, 6, -4, 7};
  static const int8_t params[] = {9, 7, -5, 10, 7};
  static const int8_t sorted[] = {-4, 1, 6, 7};
  static const size_t counts[] = {2, 1, 1, 1};
  size_t play_size = 5, pay_size = 4;

  auto push_back = [&](playlist_t &p) -> void {
    for (size_t i = 0; i < play_size; ++i)
      p.push_back(tracks[i], params[i]);
  };

  auto check_filed = [&](playlist_t &p) -> void {
    assert(p.size() == play_size);
    size_t i = 0;
    for (auto it = p.play_begin(); it != p.play_end(); ++it) {
      auto r = p.play(it);
      assert(r.first == tracks[i] && r.second == params[i]);
      ++i;
    }
    assert(i == play_size);
    i = 0;
    for (auto it = p.sorted_begin(); it != p.sorted_end(); ++it) {
      auto r = p.pay(it);
      assert(r.first == sorted[i] && r.second == counts[i]);
      ++i;
    }
    assert(i == pay_size);
  };

  auto check_empty = [](playlist_t &p) -> void {
    assert(p.size() == 0);
  };

  playlist_t pl1;
  push_back(pl1);
  playlist_t pl2(pl1);
  check_filed(pl1);
  check_filed(pl2);

  playlist_t pl3;
  push_back(pl3);
  playlist_t pl4(move(pl3));
  check_empty(pl3);
  check_filed(pl4);

  pl3 = pl1;
  check_filed(pl1);
  check_filed(pl3);

  pl4.clear();
  check_empty(pl4);
  push_back(pl4);
  pl4.clear();
  check_empty(pl4);

  pl4 = move(pl2);
  check_empty(pl2);
  check_filed(pl4);

  swap(pl2, pl4);
  check_filed(pl2);
  check_empty(pl4);
#endif

// Testujemy, czy metody są const.
#if TEST_NUM == 202
  playlist<int, int> pl;
  pl.push_back(0, 0);

  [[maybe_unused]] auto a = as_const(pl).size();
  [[maybe_unused]] auto b = as_const(pl).front();
  [[maybe_unused]] auto c = as_const(pl).play_begin();
  [[maybe_unused]] auto d = as_const(pl).play_end();
  [[maybe_unused]] auto e = as_const(pl).sorted_begin();
  [[maybe_unused]] auto f = as_const(pl).sorted_end();
  [[maybe_unused]] auto g = as_const(pl).play(c);
  [[maybe_unused]] auto h = as_const(pl).pay(e);
  [[maybe_unused]] auto i = as_const(pl).params(c);
#endif

// Druga część testu 203 jest w pliku playlist_test_extern.cpp.
#if TEST_NUM == 203
  cxx::playlist<char, char> b;
  assert(f203() == b.size());
#endif

// Testowanie złożoności pamięciowej.
// Czy rozwiązanie nie przechowuje zbyt wielu kopii utworów lub parametrów?
// Czy rozwiązanie nie wykonuje zbyt wiele kopiowań utworów?
#if TEST_NUM == 301
  playlist<Track, Params> pl;

  for (size_t i = 0; i < 100; i++)
    pl.push_back(Track(i), Params(10 * i));
  for (size_t i = 0; i < 100; i++)
    for (size_t j = 1; j < 10; j++)
      pl.push_back(Track(i), Params(10 * i + j));
  log("push_back");

  assert(pl.size() == 1000);
  assert(Track::instance_count == 100);
  assert(Track::copy_count <= 1000);
  assert(Params::instance_count == 1000);

  Track::copy_count = 0;
  Params::copy_count = 0;

  pl.remove(Track(37));
  log("remove");

  assert(pl.size() == 990);
  assert(Track::instance_count == 99);
  assert(Track::copy_count <= 1);
  assert(Params::instance_count == 990);
  assert(Params::copy_count == 0);

  Track::copy_count = 0;
  Params::copy_count = 0;

  pl.clear();
  log("clear");

  assert(pl.size() == 0);
  assert(Track::instance_count == 0);
  assert(Track::copy_count == 0);
  assert(Params::instance_count == 0);
  assert(Params::copy_count == 0);
#endif

#if TEST_NUM == 302
  playlist<Track, Params> pl;

  for (size_t i = 0; i < 100; i++)
    pl.push_back(Track(i), Params(10 * i));
  for (size_t i = 0; i < 100; i++)
    for (size_t j = 1; j < 10; j++)
      pl.push_back(Track(i), Params(10 * i + j));

  Track::copy_count = 0;
  Params::copy_count = 0;

  playlist<Track, Params> lp(pl);
  log("copy");

  assert(pl.size() == 1000);
  assert(lp.size() == 1000);
  assert(Track::instance_count == 100);
  assert(Track::copy_count == 0);
  assert(Params::instance_count == 1000);
  assert(Params::copy_count == 0);
#endif

#if TEST_NUM == 303
  playlist<Track, Params> pl;

  for (size_t i = 0; i < 100; i++)
    pl.push_back(Track(i), Params(10 * i));
  for (size_t i = 0; i < 100; i++)
    for (size_t j = 1; j < 10; j++)
      pl.push_back(Track(i), Params(10 * i + j));

  Track::copy_count = 0;
  Params::copy_count = 0;

  playlist<Track, Params> lp(move(pl));
  log("move");

  assert(pl.size() == 0);
  assert(lp.size() == 1000);
  assert(Track::instance_count == 100);
  assert(Track::copy_count == 0);
  assert(Params::instance_count == 1000);
  assert(Params::copy_count == 0);
#endif

// Czy wyjątki są zgłaszane zgodnie ze specyfikacją?
#if TEST_NUM == 401
  auto f_front     = [](auto &p)->void { [[maybe_unused]] auto r = p.front(); };
  auto f_pop_front = [](auto &p)->void { p.pop_front(); };
  auto f_remove_1  = [](auto &p)->void { p.remove(1); };
  auto f_remove_2  = [](auto &p)->void { p.remove(2); };

  playlist<int, int> playlist1;

  ThrowCheck<out_of_range>(playlist1, f_front, "front");
  ThrowCheck<out_of_range>(playlist1, f_pop_front, "pop_front");
  ThrowCheck<invalid_argument>(playlist1, f_remove_1, "remove(1)");

  playlist1.push_back(1, 1);

  ThrowCheck<invalid_argument>(playlist1, f_remove_2, "remove(2)");

  playlist1.clear();

  ThrowCheck<out_of_range>(playlist1, f_front, "front");
  ThrowCheck<out_of_range>(playlist1, f_pop_front, "pop_front");
  ThrowCheck<invalid_argument>(playlist1, f_remove_1, "remove(1)");

  playlist1.push_back(2, 2);
  playlist<int, int> playlist2(move(playlist1));

  ThrowCheck<out_of_range>(playlist1, f_front, "front");
  ThrowCheck<out_of_range>(playlist1, f_pop_front, "pop_front");
  ThrowCheck<invalid_argument>(playlist1, f_remove_1, "remove(1)");
#endif

// Czy metoda size zgłasza wyjątki?
#if TEST_NUM == 402
  playlist<Track, Params> pl;

  pl.push_back(Track(1), Params(11));
  pl.push_back(Track(2), Params(12));
  pl.push_back(Track(3), Params(13));
  pl.push_back(Track(1), Params(21));
  pl.push_back(Track(2), Params(22));
  pl.push_back(Track(3), Params(23));

  size_t size;

  NoThrowCheck(pl, [&](auto const &p) { size = p.size(); }, "size");
  assert(size == 6);
#endif

// Czy konstruktor przenoszący zgłasza wyjątki?
#if TEST_NUM == 403
  playlist<Track, Params> pl0, pl1, pl2;

  for (size_t i = 1; i <= 7; i++) {
    pl0.push_back(Track(20 * i), Params(i));
    pl1.push_back(Track(20 * i), Params(i));
  }
  for (size_t i = 1; i <= 7; i++) {
    for (size_t j = 8; j <= 21; j++) {
      pl0.push_back(Track(20 * i), Params(j));
      pl1.push_back(Track(20 * i), Params(j));
    }
  }

  size_t size = pl1.size();

  NoThrowCheck(pl1, [&pl2](auto &p)->void { pl2 = move(p); }, "move");
  assert(pl0 == pl2);
  assert(pl2.size() == size);
#endif

// Czy operacja swap zgłasza wyjątki?
#if TEST_NUM == 404
  playlist<Track, Params> pl0, pl1, pl2, pl3;

  for (size_t i = 1; i <= 8; i++) {
    pl0.push_back(Track(17 * i), Params(i));
    pl1.push_back(Track(17 * i), Params(i));
  }
  for (size_t j = 9; j <= 19; j++) {
    for (size_t i = 1; i <= 8; i++) {
      pl0.push_back(Track(17 * i), Params(j));
      pl1.push_back(Track(17 * i), Params(j));
    }
  }

  size_t size = pl1.size();

  auto f = [&pl2](auto &p)->void { swap(p, pl2); };

  NoThrowCheck(pl1, f, "swap");
  assert(pl0 == pl2);
  assert(pl3 == pl1);
  assert(pl1.size() == 0);
  assert(pl2.size() == size);

  NoThrowCheck(pl1, f, "swap");
  assert(pl0 == pl1);
  assert(pl3 == pl2);
  assert(pl1.size() == size);
  assert(pl2.size() == 0);
#endif

// Czy destruktor zgłasza wyjątki?
#if TEST_NUM == 405
  char buf[sizeof (playlist<Track, Params>)];
  playlist<Track, Params> *pl = new (buf) playlist<Track, Params>();

  for (int i = 1; i <= 9; i++)
    pl->push_back(Track(30 * i), Params(i));
  for (int i = 1; i <= 9; i++)
    for (int j = 11; j <= 19; j++)
      pl->push_back(Track(30 * i), Params(j));

  NoThrowCheck(*pl, [](auto &p)->void { p.~playlist<Track, Params>(); }, "destruktor");
#endif

// Czy metoda clear zgłasza wyjątki?
#if TEST_NUM == 406
  playlist<Track, Params> pl;

  pl.push_back(Track(1), Params(11));
  pl.push_back(Track(2), Params(12));
  pl.push_back(Track(3), Params(13));
  pl.push_back(Track(1), Params(21));
  pl.push_back(Track(2), Params(22));
  pl.push_back(Track(3), Params(23));

  NoThrowCheck(pl, [](auto &p)->void { p.clear(); }, "clear");
  assert(pl.size() == 0);

  playlist<Track, Params> pl1;

  pl1.push_back(Track(1), Params(11));
  pl1.push_back(Track(2), Params(12));
  pl1.push_back(Track(3), Params(13));
  pl1.push_back(Track(1), Params(21));
  pl1.push_back(Track(2), Params(22));
  pl1.push_back(Track(3), Params(23));

  playlist<Track, Params> pl2(pl1);

  NoThrowCheck(pl1, [](auto &p)->void { p.clear(); }, "clear");
  assert(pl1.size() == 0);
  assert(pl2.size() == 6);
#endif

// Czy metoda front na niepustej plejliście zgłasza wyjątki?
// Czy metoda pop_front na niepustej plejliście zgłasza wyjątki?
#if TEST_NUM == 407
  playlist<Track, Params> pl;

  pl.push_back(Track(1), Params(1));

  NoThrowCheck(pl, [](auto &p)->void { [[maybe_unused]] auto r = p.front(); }, "front");
  assert(pl.size() == 1);

  NoThrowCheck(pl, [](auto &p)->void { p.pop_front(); }, "pop_front");
  assert(pl.size() == 0);
#endif

// Czy metoda play zgłasza wyjątki?
// Czy metoda pay zgłasza wyjątki?
// Czy metoda params const zgłasza wyjątki?
#if TEST_NUM == 408
  playlist<Track, Params> pl;

  pl.push_back(Track(1), Params(1));

  playlist<Track, Params> pl2(pl);

  auto it = pl.play_begin();
  auto is = pl.sorted_begin();

  NoThrowCheck(pl, [&](auto &p)->void { [[maybe_unused]] auto r = p.play(it); }, "play");
  NoThrowCheck(pl, [&](auto &p)->void { [[maybe_unused]] auto r = p.pay(is); }, "pay");
  NoThrowCheck(pl, [&](auto &p)->void { [[maybe_unused]] auto const &r = as_const(p).params(it); }, "params const");
#endif

// Czy operator= zgłasza wyjątki?
#if TEST_NUM == 409
  playlist<Track, Params> src, dst;

  src.push_back(Track(1), Params(1));
  src.push_back(Track(2), Params(2));
  dst.push_back(Track(3), Params(3));
  assert(dst.size() == 1);

  NoThrowCheck(src, [&](auto &p)->void { dst = p; }, "operator przypisania");
  assert(dst.size() == 2);
#endif

// Czy iteratory zgłaszają wyjątki?
#if TEST_NUM == 410
  playlist<Track, Params> pl;

  for (size_t i = 1; i <= 4; i++)
    pl.push_back(Track(16 * i), Params(7*i));
  for (size_t j = 1; j <= 15; j++)
    for (size_t i = 1; i <= 4; i++)
      pl.push_back(Track(16 * i), Params(12*i));

  size_t sum1 = 0;
  auto f1 = [&](auto &p)->void { for (auto it = p.play_begin(); it != p.play_end(); ++it) ++sum1; };
  NoThrowCheck(pl, f1, "++play_iterator");
  assert(sum1 == 64);

  size_t sum2 = 0;
  auto f2 = [&](auto &p)->void { for (auto it = p.play_begin(); it != p.play_end(); it++) ++sum2; };
  NoThrowCheck(pl, f2, "play_iterator++");
  assert(sum2 == 64);

  NoThrowCheck(pl, [](auto &p)->void { [[maybe_unused]] auto r = p.sorted_begin(); }, "sorted_begin");
  NoThrowCheck(pl, [](auto &p)->void { [[maybe_unused]] auto r = p.sorted_end(); }, "sorted_end");

  auto it = pl.sorted_begin();
  ++it;
  NoThrowCheck(it, [](auto &i)->void { ++i; }, "++sorted_iterator");
  NoThrowCheck(it, [](auto &i)->void { i++; }, "sorted_iterator++");
#endif

// Czy metoda params nie-const zgłasza wyjątki, gdy nie robi kopii?
#if TEST_NUM == 411
  playlist<Track, Params> pl;

  pl.push_back(Track(1), Params(1));

  auto it = pl.play_begin();

  NoThrowCheck(pl, [&](auto &p)->void { [[maybe_unused]] auto const &r = p.params(it); }, "params nie-const");
#endif

// Testujemy silne gwarancje i przezroczystość na wyjątki.
// push_back
#if TEST_NUM == 501
  bool success = false;
  int trials;

  for (trials = 1; !success; trials++) {
    success = true;

    playlist<Track, Params> b, d;

    Track k1(99), k2(0);
    Params v1(9999), v2(102);
    b.push_back(k1, v1);
    d.push_back(k1, v1);

    throw_countdown = trials;
    bool result = StrongCheck(b, d, [&](auto &p)->void { p.push_back(k2, v2); }, "push_back");
    if (result) {
      d.push_back(k2, v2);
      assert(b == d);
    }
    success &= result;

    for (size_t i = 2; i < 10; i++) {
      Track k3(77 + i/2);
      Params v3(103);

      throw_countdown = trials;
      result = StrongCheck(b, d, [&](auto &p)->void { p.push_back(k3, v3); }, "push_back");
      if (result) {
        d.push_back(k3, v3);
        assert(b == d);
      }
      success &= result;
    }

    for (size_t i = 0; i < 10; i++) {
      Track k4((48271 * i) % 31 + 200);
      Params v4(103);
      b.push_back(k4, v4);
      d.push_back(k4, v4);
    }

    for (size_t i = 0; i < 10; i++) {
      Track k5((16807 * i) % 31 + 300);
      Params v5(200 + i);

      throw_countdown = trials;
      result = StrongCheck(b, d, [&](auto &p) { p.push_back(k5, v5); }, "push_back");
      if (result) {
        d.push_back(k5, v5);
        assert(b == d);
      }
      success &= result;
    }

    assert(b == d);
  }

  clog << "Liczba prób wykonanych dla push_back: " << trials << "\n";
#endif

// pop_front
#if TEST_NUM == 502
  bool success = false;
  int trials;

  for (trials = 1; !success; trials++) {
    success = true;

    playlist<Track, Params> b, d;

    for (size_t i = 2; i <= 10; ++i) {
      b.push_back(Track(i % 3), Params(i));
      d.push_back(Track(i % 3), Params(i));
    }

    throw_countdown = trials;
    bool result = StrongCheck(b, d, [](auto &p)->void { p.pop_front(); }, "pop_front");
    if (result) {
      d.pop_front();
      assert(b == d);
    }
    success &= result;

    b.pop_front();
    d.pop_front();
    b.pop_front();
    d.pop_front();
    b.pop_front();
    d.pop_front();

    throw_countdown = trials;
    result = StrongCheck(b, d, [](auto &p)->void { p.pop_front(); }, "pop_front");
    if (result) {
      d.pop_front();
      assert(b == d);
    }
    success &= result;

    assert(b == d);
  }

  clog << "Liczba prób wykonanych dla pop_front: " << trials << "\n";
#endif

// remove
#if TEST_NUM == 503
  bool success = false;
  int trials;

  for (trials = 1; !success; trials++) {
    success = true;

    playlist<Track, Params> b, d;

    Track  tracks[3] = {Track(0), Track(1), Track(2)};
    Params params[8] = {Params(10), Params(11), Params(12), Params(13),
                        Params(14), Params(15), Params(15), Params(17)};

    b.push_back(tracks[0], params[0]);
    d.push_back(tracks[0], params[0]);
    b.push_back(tracks[1], params[1]);
    d.push_back(tracks[1], params[1]);
    b.push_back(tracks[2], params[2]);
    d.push_back(tracks[2], params[2]);
    b.push_back(tracks[0], params[3]);
    d.push_back(tracks[0], params[3]);

    throw_countdown = trials;
    bool result = StrongCheck(b, d, [&](auto &p)->void { p.remove(tracks[1]); }, "remove");
    if (result) {
      d.remove(tracks[1]);
      assert(b == d);
    }
    success &= result;

    b.push_back(tracks[1], params[4]);
    d.push_back(tracks[1], params[4]);
    b.push_back(tracks[2], params[5]);
    d.push_back(tracks[2], params[5]);

    throw_countdown = trials;
    result = StrongCheck(b, d, [&](auto &p)->void { p.remove(tracks[2]); }, "remove");
    if (result) {
      d.remove(tracks[2]);
      assert(b == d);
    }
    success &= result;

    b.push_back(tracks[0], params[0]);
    d.push_back(tracks[0], params[0]);

    throw_countdown = trials;
    result = StrongCheck(b, d, [&](auto &p)->void { p.remove(tracks[1]); }, "remove");
    if (result) {
      d.remove(tracks[1]);
      assert(b == d);
    }
    success &= result;

    throw_countdown = trials;
    result = StrongCheck(b, d, [&](auto &p)->void { p.remove(tracks[0]); }, "remove");
    if (result) {
      d.remove(tracks[0]);
      assert(b == d);
    }
    success &= result;

    assert(b == d);
  }

  clog << "Liczba prób wykonanych dla remove: " << trials << "\n";
#endif

// clear
#if TEST_NUM == 504
  bool success = false;
  int trials;

  for (trials = 1; !success; trials++) {
    success = true;

    playlist<Track, Params> b, d, e;

    Track k1(1), k2(2), k3(3);
    Params v1(41), v2(42);

    b.push_back(k1, v1);
    b.push_back(k1, v2);
    b.push_back(k3, v1);
    b.push_back(k2, v2);
    b.push_back(k3, v2);

    d.push_back(k1, v1);
    d.push_back(k1, v2);
    d.push_back(k3, v1);
    d.push_back(k2, v2);
    d.push_back(k3, v2);

    throw_countdown = trials;
    bool result = StrongCheck(b, e, [](auto &p)->void { p.clear(); }, "clear");
    if (!result)
      assert(b == d);
    success &= result;
  }

  clog << "Liczba prób wykonanych dla clear: " << trials << "\n";
#endif

// operator=
#if TEST_NUM == 505
  bool success = false;
  int trials;

  for (trials = 1; !success; trials++) {
    success = true;

    playlist<Track, Params> b, d, u;

    Track k1(1), k2(2), k3(3), k4(4), k5(5);
    Params v1(41), v2(42);

    b.push_back(k1, v1);
    b.push_back(k1, v2);
    b.push_back(k3, v1);
    b.push_back(k2, v2);
    b.push_back(k3, v2);
    // Wymuszamy wykonanie kopii.
    [[maybe_unused]] auto &r = b.params(b.play_begin());

    d.push_back(k1, v1);
    d.push_back(k1, v2);
    d.push_back(k3, v1);
    d.push_back(k2, v2);
    d.push_back(k3, v2);

    u.push_back(Track(42), Params(666));

    throw_countdown = trials;
    bool result = StrongCheck(b, d, [&](auto &p)->void { u = p; }, "operator przypisania");
    if (result)
      assert(u == d);
    success &= result;
  }

  clog << "Liczba prób wykonanych dla operatora przypisania: " << trials << "\n";
#endif

// konstruktor kopiujący
#if TEST_NUM == 506
  bool success = false;
  int trials;

  for (trials = 1; !success; trials++) {
    playlist<Track, Params> b, d;

    Track k1(1), k2(2), k3(3), k4(4), k5(5);
    vector<Track> kv = {k2, k5, k4, k1, k3};
    Params v1(41), v2(42);

    Params::instance_count = 0;

    b.push_back(k1, v1);
    b.push_back(k2, v2);
    b.push_back(k1, v1);
    b.push_back(k2, v2);
    b.push_back(k2, v2);
    // Wymuszamy wykonanie kopii.
    [[maybe_unused]] auto &r = b.params(b.play_begin());

    size_t count = Params::instance_count;

    d.push_back(k1, v1);
    d.push_back(k2, v2);
    d.push_back(k1, v1);
    d.push_back(k2, v2);
    d.push_back(k2, v2);

    throw_countdown = trials;
    try {
      Params::instance_count = 0;
      throw_checking = true;
      playlist<Track, Params> u(b);
      throw_checking = false;
      success = true;
      assert(Params::instance_count == count);
      assert(u == d && u == d);
    }
    catch (...) {
      throw_checking = false;
      bool unchanged = d == b;
      if (!unchanged) {
        clog << "Konstruktor kopiujący nie daje silnej gwarancji\n.";
        assert(unchanged);
      }
    }
  }

  clog << "Liczba prób wykonanych dla konstruktora kopiującego: " << trials << "\n";
#endif

// Testujemy przywracanie stanu po zgłoszeniu wyjątku w pop_front.
#if TEST_NUM == 507
  bool success = false;
  int trials;

  playlist<Track, Params> pl1;
  pl1.push_back(Track(1), Params(1));
  pl1.push_back(Track(2), Params(2));

  playlist<Track, Params> pl2(pl1);

  size_t tract_count = Track::instance_count;
  size_t params_count = Params::instance_count;

  auto pit = pl2.play_begin();
  auto sit = pl2.sorted_begin();

  for (trials = 1; !success; trials++) {
    throw_countdown = trials;

    try {
      throw_checking = true;
      pl2.pop_front();
      throw_checking = false;
      success = true;
    }
    catch (...) {
      throw_checking = false;
      success = false;
      assert(pit == pl2.play_begin());
      assert(sit == pl2.sorted_begin());
      assert(Track::instance_count == 2);
      assert(Params::instance_count == 2);
      assert(tract_count == Track::instance_count);
      assert(params_count == Params::instance_count);
    }
  }

  clog << "Liczba prób wykonanych dla pop_front: " << trials << "\n";
#endif

// Testujemy przywracanie stanu po zgłoszeniu wyjątku w push_back.
#if TEST_NUM == 508
  bool success = false;
  int trials;

  playlist<Track, Params> pl1;
  pl1.push_back(Track(3), Params(1));
  pl1.push_back(Track(2), Params(2));
  pl1.push_back(Track(1), Params(2));
  pl1.push_back(Track(3), Params(3));
  pl1.push_back(Track(2), Params(5));
  pl1.push_back(Track(1), Params(6));

  playlist<Track, Params> pl2(pl1);

  size_t tract_count = Track::instance_count;
  size_t params_count = Params::instance_count;

  auto pit0 = pl1.play_begin();
  auto sit0 = pl1.sorted_begin();
  array<playlist<Track, Params>::play_iterator, 6> pit{pit0, pit0, pit0,
                                                       pit0, pit0, pit0};
  array<playlist<Track, Params>::sorted_iterator, 3> sit{sit0, sit0, sit0};

  size_t idx = 0;
  for (auto it = pl2.play_begin(); it != pl2.play_end(); ++it)
    pit[idx++] = it;
  assert(idx == pit.size());

  idx = 0;
  for (auto it = pl2.sorted_begin(); it != pl2.sorted_end(); ++it)
    sit[idx++] = it;
  assert(idx == sit.size());

  for (trials = 1; !success; trials++) {
    throw_countdown = trials;

    try {
      throw_checking = true;
      pl2.push_back(Track(2), Params(9));
      throw_checking = false;
      success = true;
    }
    catch (...) {
      throw_checking = false;
      success = false;
      idx = 0;
      for (auto it = pl2.play_begin(); it != pl2.play_end(); ++it)
        assert(pit[idx++] == it);
      assert(idx == pit.size());
      idx = 0;
      for (auto it = pl2.sorted_begin(); it != pl2.sorted_end(); ++it)
        assert(sit[idx++] == it);
      assert(idx == sit.size());
      assert(tract_count == Track::instance_count);
      assert(params_count == Params::instance_count);
    }
  }

  clog << "Liczba prób wykonanych dla push_back: " << trials << "\n";
#endif

// Testujemy przywracanie stanu po zgłoszeniu wyjątku w remove.
#if TEST_NUM == 509
  bool success = false;
  int trials;

  playlist<Track, Params> pl1;
  pl1.push_back(Track(3), Params(1));
  pl1.push_back(Track(2), Params(2));
  pl1.push_back(Track(1), Params(2));
  pl1.push_back(Track(3), Params(3));
  pl1.push_back(Track(2), Params(5));
  pl1.push_back(Track(1), Params(6));

  playlist<Track, Params> pl2(pl1);

  size_t tract_count = Track::instance_count;
  size_t params_count = Params::instance_count;

  auto pit0 = pl1.play_begin();
  auto sit0 = pl1.sorted_begin();
  array<playlist<Track, Params>::play_iterator, 6> pit{pit0, pit0, pit0,
                                                       pit0, pit0, pit0};
  array<playlist<Track, Params>::sorted_iterator, 3> sit{sit0, sit0, sit0};

  size_t idx = 0;
  for (auto it = pl2.play_begin(); it != pl2.play_end(); ++it)
    pit[idx++] = it;
  assert(idx == pit.size());

  idx = 0;
  for (auto it = pl2.sorted_begin(); it != pl2.sorted_end(); ++it)
    sit[idx++] = it;
  assert(idx == sit.size());

  for (trials = 1; !success; trials++) {
    throw_countdown = trials;

    try {
      throw_checking = true;
      pl2.remove(Track(2));
      throw_checking = false;
      success = true;
    }
    catch (...) {
      throw_checking = false;
      success = false;
      idx = 0;
      for (auto it = pl2.play_begin(); it != pl2.play_end(); ++it)
        assert(pit[idx++] == it);
      assert(idx == pit.size());
      idx = 0;
      for (auto it = pl2.sorted_begin(); it != pl2.sorted_end(); ++it)
        assert(sit[idx++] == it);
      assert(idx == sit.size());
      assert(tract_count == Track::instance_count);
      assert(params_count == Params::instance_count);
    }
  }

  clog << "Liczba prób wykonanych dla remove: " << trials << "\n";
#endif

// params nie-const
#if TEST_NUM == 510
  bool success = false;
  int trials;

  for (trials = 1; !success; trials++) {
    success = true;

    playlist<Track, Params> b, d;

    Track t1(99);
    Params p1(9999);
    b.push_back(t1, p1);
    d.push_back(t1, p1);

    playlist<Track, Params> b2(b);
    playlist<Track, Params> d2(d);

    auto itb = b.play_begin();

    throw_countdown = trials;
    bool result = StrongCheck(b, d, [&](auto &p)->void { [[maybe_unused]] auto &r = p.params(itb); }, "params nie-const");
    if (!result) {
      auto &p0 = b.params(itb);
      assert(p0 == p1);
    }

    success &= result;

    assert(b == d);
  }

  clog << "Liczba prób wykonanych dla params nie-const: " << trials << "\n";
#endif

// Testujemy kopiowanie przy modyfikowaniu.
#if TEST_NUM == 601
  playlist<Track, Params> pl1;

  for (int i = 1; i <= 7; i++)
    pl1.push_back(Track(i), Params(i));
  for (int j = 1; j <= 9; j++)
    for (int i = 1; i <= 7; i++)
      pl1.push_back(Track(i), Params(i + j));

  log("1");
  assert(pl1.size() == 70);
  assert(Track::instance_count == 7);
  assert(Params::instance_count == 70);

  // Tu nic się nie kopiuje. Wszystko jest współdzielone.
  playlist<Track, Params> pl2(pl1);
  playlist<Track, Params> pl3;
  playlist<Track, Params> pl4;
  pl3 = pl1;
  pl4 = move(pl2);

  log("2");
  assert(pl1.size() == 70);
  assert(pl2.size() == 0);
  assert(pl3.size() == 70);
  assert(pl4.size() == 70);
  assert(Track::instance_count == 7);
  assert(Params::instance_count == 70);

  auto it = pl3.play_begin();
  advance(it, 17);
  // Wymuszamy skopiowanie współdzielonej plejlisty.
  [[maybe_unused]] auto &t = pl3.params(it);

  log("3");
  assert(pl3.size() == 70);
  // Utwory nie muszą być kopiowane. Parametry zostały skopiowane.
  assert(Track::instance_count == 7 || Track::instance_count == 14);
  assert(Params::instance_count == 140);

  size_t track_instance_count = Track::instance_count;
  it = pl3.play_begin();
  advance(it, 13);
  // Nie kopiujemy plejlisty, która nie jest współdzielona.
  [[maybe_unused]] auto &v = pl3.params(it);

  log("4");
  assert(pl3.size() == 70);
  // Utwory nie zostały skopiowane. Parametry nie zostały skopiowane.
  assert(Track::instance_count == track_instance_count);
  assert(Params::instance_count == 140);

  // Trzeba skopiować, bo udzielono referencji do pl3 i nie może współdzielić.
  playlist<Track, Params> pl5(pl3);

  log("5");
  assert(pl5.size() == 70);
  // Utwory nie muszą być kopiowane. Parametry zostały skopiowane.
  assert(Track::instance_count == 7 || Track::instance_count == 21);
  assert(Params::instance_count == 210);

  track_instance_count = Track::instance_count;
  // Modyfikujemy plejlistę i unieważniamy udzielone referencje.
  pl3.pop_front();
  // Nie trzeba kopiować, bo pl3 może znów współdzielić.
  playlist<Track, Params> pl6(pl3);

  log("6");
  assert(pl6.size() == 69);
  assert(Track::instance_count == track_instance_count);
  assert(Params::instance_count == 209);
#endif

// Testujemy rozdzielanie plejlist przy modyfikowaniu parametrów.
#if TEST_NUM == 602
  playlist<int, int> pl1;

  pl1.push_back(0, 0);
  pl1.push_back(1, 1);
  pl1.push_back(2, 2);

  playlist<int, int> pl2(pl1);
  playlist<int, int> pl3(pl1);

  auto it1 = pl1.play_begin();
  int &p1 = pl1.params(it1);
  p1 = 42;

  playlist<int, int> pl4(pl3);

  auto it3 = pl3.play_begin();
  (++it3)++;
  int &p3 = pl3.params(it3);
  p3 = 77;

  int i = 0;
  for (auto it = pl1.play_begin(); it != pl1.play_end(); ++it, ++i)
    assert(pl2.play(it).second == (i == 0 ? 42 : i));
  i = 0;
  for (auto it = pl2.play_begin(); it != pl2.play_end(); ++it, ++i)
    assert(pl2.play(it).second == i);
  i = 0;
  for (auto it = pl3.play_begin(); it != pl3.play_end(); ++it, ++i)
    assert(pl3.play(it).second == (i == 2 ? 77 : i));
  i = 0;
  for (auto it = pl4.play_begin(); it != pl4.play_end(); ++it, ++i)
    assert(pl4.play(it).second == i);
#endif

// Testujemy kopiowanie przy move, clear, remove, push_back, pop_front.
#if TEST_NUM == 603
  playlist<Track, Params> pl1;

  pl1.push_back(Track(1), Params(1));
  pl1.push_back(Track(3), Params(2));
  pl1.push_back(Track(2), Params(3));
  pl1.push_back(Track(5), Params(4));
  pl1.push_back(Track(4), Params(5));
  pl1.push_back(Track(3), Params(6));
  pl1.push_back(Track(3), Params(7));
  pl1.push_back(Track(2), Params(8));
  pl1.push_back(Track(5), Params(9));

  log("1");
  assert(pl1.size() == 9);
  assert(Track::instance_count == 5);
  assert(Params::instance_count == 9);

  Track::copy_count = 0;
  Params::copy_count = 0;

  playlist<Track, Params> pl2(move(pl1));

  log("2");
  assert(pl2.size() == 9);
  assert(Track::instance_count == 5);
  assert(Params::instance_count == 9);
  assert(Track::copy_count == 0);
  assert(Params::copy_count == 0);

  pl2.clear();

  log("3");
  assert(pl2.size() == 0);
  assert(Track::instance_count == 0);
  assert(Params::instance_count == 0);
  assert(Track::copy_count == 0);
  assert(Params::copy_count == 0);

  pl2.push_back(Track(11), Params(1));
  pl2.push_back(Track(33), Params(2));
  pl2.push_back(Track(22), Params(3));
  pl2.push_back(Track(15), Params(4));
  pl2.push_back(Track(15), Params(5));
  pl2.push_back(Track(33), Params(6));
  pl2.push_back(Track(33), Params(7));
  pl2.push_back(Track(22), Params(8));

  log("4");
  assert(pl2.size() == 8);
  assert(Track::instance_count == 4);
  assert(Params::instance_count == 8);

  Track::copy_count = 0;
  Params::copy_count = 0;

  playlist<Track, Params> pl3(pl2), pl4(pl2);

  log("5");
  assert(pl2.size() == 8);
  assert(Track::instance_count == 4);
  assert(Params::instance_count == 8);
  assert(Track::copy_count == 0);
  assert(Params::copy_count == 0);

  pl2.remove(Track(15));
  pl3.push_back(Track(42), Params(9));
  pl4.pop_front();

  log("6");
  assert(pl2.size() == 6);
  assert(pl3.size() == 9);
  assert(pl4.size() == 7);
  assert(Track::instance_count == 5 || Track::instance_count == 11);
  assert(Params::instance_count == 22);
#endif

// Testujemy unieważnianie udzielonych referencji.
#if TEST_NUM == 604
  playlist<Track, Params> pl1;

  for (int i = 1; i <= 3; i++) {
    pl1.push_back(Track(4 - i), Params(0));
    for (int j = 1; j <= 3; j++) {
      pl1.push_back(Track(4 - i), Params(j));
      pl1.push_back(Track(j + 10 * i + 5), Params(i));
    }
  }

  log("1");
  assert(Track::instance_count == 12);
  assert(Params::instance_count == 21);

  [[maybe_unused]] auto &r1 = pl1.params(pl1.play_begin());
  pl1.remove(Track(38));

  log("2");
  assert(Track::instance_count == 11);
  assert(Params::instance_count == 20);

  playlist<Track, Params> pl2(pl1);
  playlist<Track, Params> pl3;
  playlist<Track, Params> pl4;
  pl3 = pl1;
  pl4 = move(pl2);

  log("3");
  assert(pl1.size() == 20);
  assert(pl3.size() == 20);
  assert(pl4.size() == 20);
  assert(Track::instance_count == 11);
  assert(Params::instance_count == 20);

  [[maybe_unused]] auto &r2 = pl1.params(pl1.play_begin());

  log("4");
  assert(Track::instance_count == 11 || Track::instance_count == 22);
  assert(Params::instance_count == 40);

  pl1.remove(Track(2));

  log("5");
  assert(pl1.size() == 16);
  assert(Track::instance_count == 11 || Track::instance_count == 21);
  assert(Params::instance_count == 36);

  playlist<Track, Params> pl5(pl1);
  playlist<Track, Params> pl6;
  playlist<Track, Params> pl7;
  playlist<Track, Params> pl8;
  pl6 = pl1;
  pl7 = move(pl5);
  pl8 = pl4;

  log("6");
  assert(pl6.size() == 16);
  assert(pl7.size() == 16);
  assert(pl8.size() == 20);
  assert(Track::instance_count == 11 || Track::instance_count == 21);
  assert(Params::instance_count == 36);
#endif

// Testujemy rozdzielanie plejlist przy modyfikowaniu parametru.
#if TEST_NUM == 605
  size_t t_count;

  do {
    playlist<Track, long> pl1;

    pl1.push_back(Track(101), 111);
    pl1.push_back(Track(102), 122);
    pl1.push_back(Track(101), 121);
    pl1.push_back(Track(103), 113);
    pl1.push_back(Track(103), 123);
    pl1.push_back(Track(102), 112);
    pl1.push_back(Track(203), 133);

    log("1");
    t_count = Track::instance_count;
    assert(t_count == 4);

    playlist<Track, long> pl2(pl1);

    log("2");
    assert(Track::instance_count == t_count);

    long &v1 = pl1.params(pl1.play_begin());
    v1 = 233;

    log("3");
    assert(Track::instance_count == 2 * t_count || Track::instance_count == t_count);
    assert(pl1.params(pl1.play_begin()) == 233);
    assert(pl2.params(pl2.play_begin()) == 111);
  } while (0);

  log("4");
  assert(Track::instance_count == 0);
#endif

// Nie wiadomo, co spawdza ten test, ale jak jest, to czemu nie sprawdzić.
#if TEST_NUM == 606
  auto pl = make_unique<playlist<int, int>>();
  vector<playlist<int, int>> vec;
  for (int i = 0; i < 66666; i++)
    pl->push_back(i, i);
  for (int i = 0; i < 77777; i++)
    vec.push_back(*pl);
  pl.reset();
  for (int i = 0; i < 10; i++)
    vec[i].push_back(i + 55555, i);
  for (int i = 0; i < 10; i++)
    for (int j = 0; j < 11; j++)
      if (i != j)
        assert(!(vec[i] == vec[j]));
  assert(!(vec[0] == vec[77776]));
  assert(vec[11] == vec[33332]);
#endif
}
