#ifndef CXX_PLAYLIST_H
#define CXX_PLAYLIST_H

#include <list>
#include <map>
#include <memory>
#include <stdexcept>
#include <utility>
#include <cstddef>
#include <type_traits>
#include <set>
#include  <unordered_map>

namespace cxx {

template <typename T, typename P>
class playlist {
private:
    // forward declarations iterator types
    struct Data {
        std::unordered_map<T, std::list<typename std::list<std::pair<T&, P>>::iterator>> occurences; // zbiór unikalnych utworów
        std::list<std::pair<T&, P>> play_list; // lista odtwarzania
        std::set<T&> sorted_set; // mapa do szybkiego dostępu 
    }
    std::shared_ptr<Data> data_;

    
public:
    // konstruktorzy / destruktor
    playlist() noexcept(std::is_nothrow_default_constructible_v<std::shared_ptr<Data>>){
        
    };
    playlist(playlist const & other) noexcept {
        data_ = other.data_;
    } // shallow copy — CoW
    playlist(playlist && other) noexcept {
        data_ = std::move(other.data_);
    };
    ~playlist() noexcept;

    playlist & operator=(playlist other) noexcept;

    // modyfikacje
    void push_back(T const & track, P const & params); // O(log n)
    void pop_front(); // O(1)
    const std::pair<T const &, P const &> front() const; // O(1)

    void remove(T const & track); // O((k+1) log n)
    void clear() noexcept; // O(n)

    size_t size() const noexcept; // O(1)

    // play / pay
    const std::pair<T const &, P const &> play(play_iterator const & it) const; // O(1)
    const std::pair<T const &, size_t> pay(sorted_iterator const & it) const; // O(k)

    // params access
    P & params(play_iterator const & it);
    const P & params(play_iterator const & it) const;

    // iteratory
    play_iterator play_begin() const noexcept;
    play_iterator play_end() const noexcept;

    sorted_iterator sorted_begin() const noexcept;
    sorted_iterator sorted_end() const noexcept;
};

}; // namespace cxx

#endif // CXX_PLAYLIST_H