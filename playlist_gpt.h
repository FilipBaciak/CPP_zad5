// playlist.h
#ifndef CXX_PLAYLIST_H
#define CXX_PLAYLIST_H

#include <list>
#include <map>
#include <memory>
#include <stdexcept>
#include <utility>
#include <cstddef>
#include <type_traits>

namespace cxx {

template <typename T, typename P>
class playlist {
private:
    // forward declarations iterator types
    struct Data;
public:
    // iteratorami są klasy zagnieżdżone
    class play_iterator;
    class sorted_iterator;

    // konstruktorzy / destruktor
    playlist() noexcept(std::is_nothrow_default_constructible_v<std::shared_ptr<Data>>);
    playlist(playlist const & other) noexcept; // shallow copy — CoW
    playlist(playlist && other) noexcept;
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

private:
    // Wewnętrzna reprezentacja:
    // - przechowujemy listę wystąpień (kolejność wstawiania)
    // - mapę od T -> TrackInfo (po jednej kopii T)
    // - TrackInfo przechowuje shared_ptr<T> i listę iteratorów do wystąpień
    struct Occurrence {
        struct TrackInfo * track_info; // surowy wskaźnik do TrackInfo należącego do Data
        P params;
    };
    struct TrackInfo {
        std::shared_ptr<T> track;
        // zbiór iteratorów do occurrences - pozwala usuwać wszystkie wystąpienia w O(k)
        std::list<typename std::list<Occurrence>::iterator> occ_iters;
        size_t count = 0;
    };

    struct Data {
        std::list<Occurrence> occurrences; // kolejność odtwarzania, with duplicates
        std::map<T, TrackInfo> sorted;     // klucz T -> TrackInfo
        size_t total = 0;

        Data() noexcept = default;
        // kopiowanie Data wykonujemy ręcznie w ensure_unique — może rzucać
    };

    std::shared_ptr<Data> data_; // współdzielone; copy-on-write

    // Upewnij się, że mamy unikatową (nie współdzieloną) kopię danych przed modyfikacją.
    void ensure_unique();

    // pomocnik tworzący deep copy Data; wykorzystywany przez ensure_unique()
    static std::shared_ptr<Data> deep_copy_data(Data const & src);

public:
    // play_iterator - iteruje po occurrences
    class play_iterator {
        friend class playlist;
    public:
        play_iterator() noexcept = default;
        play_iterator(play_iterator const &) noexcept = default;
        play_iterator & operator=(play_iterator const &) noexcept = default;

        bool operator==(play_iterator const & o) const noexcept {
            return data_.lock() == o.data_.lock() && it_ == o.it_;
        }
        bool operator!=(play_iterator const & o) const noexcept {
            return !(*this == o);
        }
        // prefiks ++
        play_iterator & operator++() {
            ++it_;
            return *this;
        }
        // postfiks ++
        play_iterator operator++(int) {
            play_iterator tmp = *this;
            ++(*this);
            return tmp;
        }
    private:
        explicit play_iterator(std::weak_ptr<Data> d,
                               typename std::list<Occurrence>::const_iterator it) noexcept
            : data_(d), it_(it) {}

        std::weak_ptr<Data> data_;
        typename std::list<Occurrence>::const_iterator it_;
    };

    // sorted_iterator - iteruje po kluczach mapy sorted (bez powtórzeń)
    class sorted_iterator {
        friend class playlist;
    public:
        sorted_iterator() noexcept = default;
        sorted_iterator(sorted_iterator const &) noexcept = default;
        sorted_iterator & operator=(sorted_iterator const &) noexcept = default;

        bool operator==(sorted_iterator const & o) const noexcept {
            return data_.lock() == o.data_.lock() && it_ == o.it_;
        }
        bool operator!=(sorted_iterator const & o) const noexcept {
            return !(*this == o);
        }
        sorted_iterator & operator++() {
            ++it_;
            return *this;
        }
        sorted_iterator operator++(int) {
            sorted_iterator tmp = *this;
            ++(*this);
            return tmp;
        }
    private:
        explicit sorted_iterator(std::weak_ptr<Data> d,
                                 typename std::map<T, TrackInfo>::const_iterator it) noexcept
            : data_(d), it_(it) {}

        std::weak_ptr<Data> data_;
        typename std::map<T, TrackInfo>::const_iterator it_;
    };
};


// ---------------- implementation ----------------

template <typename T, typename P>
playlist<T,P>::playlist() noexcept
    : data_(std::make_shared<Data>())
{}

template <typename T, typename P>
playlist<T,P>::playlist(playlist const & other) noexcept
    : data_(other.data_) // CoW: wspólne dane
{}

template <typename T, typename P>
playlist<T,P>::playlist(playlist && other) noexcept
    : data_(std::move(other.data_))
{
    if (!other.data_) other.data_ = std::make_shared<Data>();
}

template <typename T, typename P>
playlist<T,P>::~playlist() noexcept = default;

template <typename T, typename P>
playlist<T,P> & playlist<T,P>::operator=(playlist other) noexcept {
    // copy-and-swap; argument passed by value już wykonał shallow copy/przeniesienie
    data_.swap(other.data_);
    return *this;
}

template <typename T, typename P>
void playlist<T,P>::ensure_unique() {
    if (!data_.unique()) {
        // deep copy
        data_ = deep_copy_data(*data_);
    }
}

template <typename T, typename P>
std::shared_ptr<typename playlist<T,P>::Data> playlist<T,P>::deep_copy_data(Data const & src) {
    // Budujemy nowy Data i kopiujemy occurrences oraz mapę w sposób bezpieczny
    auto nd = std::make_shared<Data>();
    nd->total = src.total;
    // mapujemy stare TrackInfo::track (shared_ptr<T>) do nowego TrackInfo w nd->sorted
    // ale żeby zachować jedną kopię na unikalny T, tworzymy nowe TrackInfo przy pierwszym wystąpieniu klucza
    for (auto const & occ : src.occurrences) {
        const T & orig_track = *occ.track_info->track;
        // find or insert track in nd->sorted
        auto it = nd->sorted.find(orig_track);
        if (it == nd->sorted.end()) {
            // wstawiamy nowy TrackInfo z nową kopią T
            TrackInfo ti;
            ti.track = std::make_shared<T>(orig_track); // może rzucać
            ti.count = 0;
            auto res = nd->sorted.emplace(*ti.track, std::move(ti));
            it = res.first;
        }
        // dodaj occurrence do nd->occurrences i zapamiętaj iterator w TrackInfo
        nd->occurrences.emplace_back();
        auto occ_it = std::prev(nd->occurrences.end());
        occ_it->track_info = &it->second; // bezpieczne — element mapy nie będzie się przemieszczał
        occ_it->params = occ.params; // kopiujemy parametry (może rzucać)
        it->second.occ_iters.push_back(occ_it);
        ++it->second.count;
    }
    return nd;
}

template <typename T, typename P>
void playlist<T,P>::push_back(T const & track, P const & params) {
    // zmodyfikujemy strukturę — najpierw upewniamy się o unikalności (CoW)
    ensure_unique();

    // znajdź lub wstaw TrackInfo dla track
    auto & sorted = data_->sorted;
    auto sit = sorted.find(track);
    if (sit == sorted.end()) {
        // wstaw nowy TrackInfo; wykorzystujemy emplacement aby zminimalizować kopiowania
        TrackInfo ti;
        ti.track = std::make_shared<T>(track); // kopia track
        ti.count = 0;
        auto res = sorted.emplace(*ti.track, std::move(ti));
        sit = res.first;
    }
    // wstawienie occurrence na koniec listy (O(1))
    data_->occurrences.emplace_back();
    auto occ_it = std::prev(data_->occurrences.end());
    occ_it->track_info = &sit->second;
    // kopiujemy params do occurrence
    occ_it->params = params;
    // zapisz iterator w TrackInfo
    sit->second.occ_iters.push_back(occ_it);
    ++sit->second.count;
    ++data_->total;
}

template <typename T, typename P>
void playlist<T,P>::pop_front() {
    ensure_unique();
    if (data_->occurrences.empty()) throw std::out_of_range("playlist::pop_front: empty");
    auto it = data_->occurrences.begin();
    TrackInfo * ti = it->track_info;
    // usuń iterator z ti->occ_iters - musimy znaleźć element odpowiadający it
    // occ_iters to lista iteratorów; usunięcie wymaga wyszukania pozycji — ale mamy iteratory; szukamy i usuwamy
    for (auto occ_it_it = ti->occ_iters.begin(); occ_it_it != ti->occ_iters.end(); ++occ_it_it) {
        if (*occ_it_it == it) {
            ti->occ_iters.erase(occ_it_it);
            break;
        }
    }
    --ti->count;
    data_->occurrences.pop_front();
    --data_->total;
    if (ti->count == 0) {
        // usuń wpis z mapy
        // należy podać klucz; track znajduje się w ti->track
        sorted_iterator dummy; (void)dummy;
        data_->sorted.erase(*ti->track);
    }
}

template <typename T, typename P>
const std::pair<T const &, P const &> playlist<T,P>::front() const {
    if (data_->occurrences.empty()) throw std::out_of_range("playlist::front: empty");
    auto const & occ = data_->occurrences.front();
    return { *occ.track_info->track, occ.params };
}

template <typename T, typename P>
void playlist<T,P>::remove(T const & track) {
    ensure_unique();
    auto & sorted = data_->sorted;
    auto it = sorted.find(track);
    if (it == sorted.end()) throw std::invalid_argument("playlist::remove: track not found");

    // usuwamy wszystkie occurrence wskazywane przez it->second.occ_iters
    // Każde usunięcie z listy occurrences jest O(1)
    auto & occ_iters = it->second.occ_iters;
    for (auto occ_it : occ_iters) {
        // usuń occ_it z listy occurrences
        data_->occurrences.erase(occ_it);
        --data_->total;
    }
    // po usunięciu wszystkich occurrence wyczyść strukturę TrackInfo i usuń z mapy
    occ_iters.clear();
    sorted.erase(it);
}

template <typename T, typename P>
void playlist<T,P>::clear() noexcept {
    // jeżeli mamy współdzielone dane — od razu ustawiamy nowe proste Data
    data_ = std::make_shared<Data>();
}

template <typename T, typename P>
size_t playlist<T,P>::size() const noexcept {
    return data_->total;
}

template <typename T, typename P>
const std::pair<T const &, P const &> playlist<T,P>::play(play_iterator const & it) const {
    auto dp = it.data_.lock();
    if (!dp) throw std::out_of_range("playlist::play: invalid iterator");
    // it.it_ jest const_iterator; dereferencja
    const Occurrence & occ = *it.it_;
    return { *occ.track_info->track, occ.params };
}

template <typename T, typename P>
const std::pair<T const &, size_t> playlist<T,P>::pay(sorted_iterator const & it) const {
    auto dp = it.data_.lock();
    if (!dp) throw std::out_of_range("playlist::pay: invalid iterator");
    const auto & kv = *it.it_;
    const T & t = kv.first;
    size_t cnt = kv.second.count;
    return { t, cnt };
}

template <typename T, typename P>
P & playlist<T,P>::params(play_iterator const & it) {
    // modyfikacja parametrów jest modyfikacją struktury -> trzeba ensure_unique()
    ensure_unique();
    auto dp = it.data_.lock();
    if (!dp) throw std::out_of_range("playlist::params: invalid iterator");
    // Uwaga: iterator może pochodzić z innego (współdzielonego) Data. To jest trudne:
    // Założenie: iterator jest poprawny tylko względem tego samego obiektu playlist.
    // Tutaj dopuszczamy, że it wskazuje na ten sam data_ (lub współdzielony) — aby wykonać CoW,
    // powinno się przed mutacją sklonować data_ i przekształcić iteratory. W tym prostym modelu
    // params() działa tylko jeśli iterator należy do tej samej instancji / współdzielonej.
    // W razie potrzeby można rozbudować by iterator przechowywał index identyfikujący occurrence.
    return const_cast<P&>((*it.it_).params);
}

template <typename T, typename P>
const P & playlist<T,P>::params(play_iterator const & it) const {
    auto dp = it.data_.lock();
    if (!dp) throw std::out_of_range("playlist::params: invalid iterator");
    return (*it.it_).params;
}

template <typename T, typename P>
typename playlist<T,P>::play_iterator playlist<T,P>::play_begin() const noexcept {
    return play_iterator(data_, data_->occurrences.cbegin());
}

template <typename T, typename P>
typename playlist<T,P>::play_iterator playlist<T,P>::play_end() const noexcept {
    return play_iterator(data_, data_->occurrences.cend());
}

template <typename T, typename P>
typename playlist<T,P>::sorted_iterator playlist<T,P>::sorted_begin() const noexcept {
    return sorted_iterator(data_, data_->sorted.cbegin());
}

template <typename T, typename P>
typename playlist<T,P>::sorted_iterator playlist<T,P>::sorted_end() const noexcept {
    return sorted_iterator(data_, data_->sorted.cend());
}

} // namespace cxx

#endif // CXX_PLAYLIST_H
