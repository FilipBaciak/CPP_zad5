#include "playlist.h"

#include <type_traits>
#include <utility>

using T = int;
using P = int;
using Playlist = cxx::playlist<T, P>;

// Sprawdzamy cechy typu (konstruktory, destruktor, operator=).
static_assert(std::is_nothrow_copy_constructible_v<Playlist>,
              "playlist copy ctor should be noexcept");
static_assert(std::is_nothrow_move_constructible_v<Playlist>,
              "playlist move ctor should be noexcept");
static_assert(std::is_nothrow_destructible_v<Playlist>,
              "playlist dtor should be noexcept");
static_assert(std::is_nothrow_move_assignable_v<Playlist>,
              "playlist move assignment should be noexcept");

// Sprawdzamy specyfikacje noexcept metod oznaczonych jako noexcept.

// size()
static_assert(noexcept(std::declval<Playlist const&>().size()),
              "size() should be noexcept");

// iteratory play_*
static_assert(noexcept(std::declval<Playlist const&>().play_begin()),
              "play_begin() should be noexcept");
static_assert(noexcept(std::declval<Playlist const&>().play_end()),
              "play_end() should be noexcept");

// iteratory sorted_*
static_assert(noexcept(std::declval<Playlist const&>().sorted_begin()),
              "sorted_begin() should be noexcept");
static_assert(noexcept(std::declval<Playlist const&>().sorted_end()),
              "sorted_end() should be noexcept");

// play() i pay()
static_assert(
    noexcept(std::declval<Playlist const&>().play(
        std::declval<typename Playlist::play_iterator const&>())),
    "play(play_iterator) should be noexcept");

static_assert(
    noexcept(std::declval<Playlist const&>().pay(
        std::declval<typename Playlist::sorted_iterator const&>())),
    "pay(sorted_iterator) should be noexcept");

int main()
{
    // Nic nie robimy w runtime – cały test jest na etapie kompilacji.
    return 0;
}
