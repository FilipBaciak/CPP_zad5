#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <list>
#include <map>
#include <memory>
#include <stdexcept>
#include <utility>
#include <iterator>
#include <algorithm> // dla std::distance, std::advance

namespace cxx {

template <typename T, typename P>
class playlist {
private:
    struct Entry;
    
    // Aliasy typów
    using SequenceList = std::list<Entry>;
    using SequenceIterator = typename SequenceList::iterator;
    using ConstSequenceIterator = typename SequenceList::const_iterator;

    using OccurrencesList = std::list<SequenceIterator>;
    using OccurrencesIterator = typename OccurrencesList::iterator;

    using IndexMap = std::map<T, OccurrencesList>;
    using IndexIterator = typename IndexMap::iterator;
    using ConstIndexIterator = typename IndexMap::const_iterator;

    struct Entry {
        P params;
        IndexIterator map_it;
        OccurrencesIterator distinct_it;

        Entry(P const &p, IndexIterator m_it, OccurrencesIterator d_it)
            : params(p), map_it(m_it), distinct_it(d_it) {}
        
        Entry(P const &p) : params(p) {}
    };

    struct Impl {
        SequenceList sequence;
        IndexMap index;

        void insert_track(T const &track, P const &params) {
            auto lb = index.lower_bound(track);
            bool insert_new = false;

            if (lb == index.end() || index.key_comp()(track, lb->first)) {
                lb = index.emplace_hint(lb, track, OccurrencesList{});
                insert_new = true;
            }

            // Wstępna rezerwacja w liście wystąpień
            try{
                lb->second.push_back(sequence.end()); 
            }
            catch(...){

                if(insert_new)
                {
                    index.erase(lb);
                }
                throw;
            }

            auto occ_it = std::prev(lb->second.end());

            try {
                // Wstawienie do sekwencji (główna lista)
                sequence.emplace_back(params, lb, occ_it);
                
                // Aktualizacja iteratora w mapie
                *occ_it = std::prev(sequence.end());
            } catch (...) {
                lb->second.pop_back();
                if (lb->second.empty()) {
                    index.erase(lb);
                }
                throw;
            }
        }
    };

    std::shared_ptr<Impl> data_;

    void detach() {
        if (!data_) {
            data_ = std::make_shared<Impl>();
        } else if (data_.use_count() > 1) {
            auto new_data = std::make_shared<Impl>();
            for (auto const &entry : data_->sequence) {
                new_data->insert_track(entry.map_it->first, entry.params);
            }
            data_ = std::move(new_data);
        }
    }

public:
    // --- Iteratory ---

    class play_iterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = std::pair<T const &, P const &>;
        using difference_type = std::ptrdiff_t;
        using pointer = void; 
        using reference = void;

        play_iterator() = default;

        bool operator==(play_iterator const &other) const { return it_ == other.it_; }
        bool operator!=(play_iterator const &other) const { return it_ != other.it_; }

        play_iterator &operator++() {
            ++it_;
            return *this;
        }
        play_iterator operator++(int) {
            play_iterator temp = *this;
            ++it_;
            return temp;
        }
        play_iterator &operator--() {
            --it_;
            return *this;
        }
        play_iterator operator--(int) {
            play_iterator temp = *this;
            --it_;
            return temp;
        }

    private:
        friend class playlist;
        ConstSequenceIterator it_;
        explicit play_iterator(ConstSequenceIterator it) : it_(it) {}
    };

    class sorted_iterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T const *;
        using reference = T const &;

        sorted_iterator() = default;

        bool operator==(sorted_iterator const &other) const { return it_ == other.it_; }
        bool operator!=(sorted_iterator const &other) const { return it_ != other.it_; }

        sorted_iterator &operator++() {
            ++it_;
            return *this;
        }
        sorted_iterator operator++(int) {
            sorted_iterator temp = *this;
            ++it_;
            return temp;
        }
        sorted_iterator &operator--() {
            --it_;
            return *this;
        }
        sorted_iterator operator--(int) {
            sorted_iterator temp = *this;
            --it_;
            return temp;
        }

        T const & operator*() const {
            return it_->first;
        }
        
        T const * operator->() const {
            return &(it_->first);
        }

    private:
        friend class playlist;
        ConstIndexIterator it_;
        explicit sorted_iterator(ConstIndexIterator it) : it_(it) {}
    };

    // --- Konstruktory i Destruktor ---

    playlist() : data_(std::make_shared<Impl>()) {}

    playlist(playlist const &other) : data_(other.data_) {}

    playlist(playlist &&other) noexcept : data_(std::move(other.data_)) {}

    ~playlist() = default;

    playlist & operator=(playlist other) {
        std::swap(data_, other.data_);
        return *this;
    }

    // --- Metody Modyfikujące ---

    void push_back(T const &track, P const &params) {
        detach();
        data_->insert_track(track, params);
    }

    void pop_front() {
        if (!data_ || data_->sequence.empty()) {
            throw std::out_of_range("pop_front, playlist empty");
        }
        detach();

        auto it = data_->sequence.begin();
        it->map_it->second.erase(it->distinct_it);

        if (it->map_it->second.empty()) {
            data_->index.erase(it->map_it);
        }
        data_->sequence.pop_front();
    }

    void remove(T const &track) {
        if (!data_) { 
             throw std::invalid_argument("remove, unknown track");
        }
        auto it = data_->index.find(track);
        if (it == data_->index.end()) {
            throw std::invalid_argument("remove, unknown track");
        }

        detach();
        
        // Ponowne wyszukanie po detach
        auto map_it = data_->index.find(track);
        for (auto seq_it : map_it->second) {
            data_->sequence.erase(seq_it);
        }
        data_->index.erase(map_it);
    }

    void clear() {
        if (data_ && data_.use_count() > 1) {
            data_ = std::make_shared<Impl>();
        } else if (data_) {
            data_->sequence.clear();
            data_->index.clear();
        } else {
             data_ = std::make_shared<Impl>();
        }
    }

    P & params(play_iterator const &it) {
        if (data_ && data_.use_count() > 1) {
            // Mamy współdzielenie, musimy zrobić detach.
            // Problem: 'it' wskazuje na stary obiekt Impl.
            // Musimy znaleźć odpowiadający mu element w nowym Impl.
            // Jedyny bezpieczny sposób to obliczenie dystansu.
            auto old_begin = data_->sequence.begin();
            auto dist = std::distance(ConstSequenceIterator(old_begin), it.it_);
            detach();
            auto new_it = data_->sequence.begin();
            std::advance(new_it, dist);

            auto &nonconst_it = const_cast<play_iterator &>(it);
            nonconst_it.it_ = new_it;

            return new_it->params;
        }
        
        // Jeśli jesteśmy unique, modyfikujemy w miejscu.
        // const_cast jest bezpieczny, bo wiemy, że obiekt należy tylko do nas,
        // a iteratory są wrapperami na const_iterator tylko dla interfejsu.
        Entry &e = const_cast<Entry &>(*(it.it_));
        return e.params;
    }

    // --- Metody Dostępowe (Const) ---

    const std::pair<T const &, P const &> front() const {
        if (!data_ || data_->sequence.empty()) {
            throw std::out_of_range("front, playlist empty");
        }
        Entry const &e = data_->sequence.front();
        return std::pair<T const &, P const &>(e.map_it->first, e.params);
    }

    const std::pair<T const &, P const &> play(play_iterator const &it) const {
        Entry const &e = *(it.it_);
        return std::pair<T const &, P const &>(e.map_it->first, e.params);
    }

    const std::pair<T const &, size_t> pay(sorted_iterator const &it) const {
        return std::pair<T const &, size_t>(it.it_->first, it.it_->second.size());
    }

    const P & params(play_iterator const &it) const {
        Entry const &e = *(it.it_);
        return e.params;
    }

    size_t size() const {
        if (!data_) return 0;
        return data_->sequence.size();
    }

    // --- Metody zwracające iteratory (Const) ---

    play_iterator play_begin() const {
        if (!data_) return play_iterator();
        return play_iterator(data_->sequence.begin());
    }

    play_iterator play_end() const {
        if (!data_) return play_iterator();
        return play_iterator(data_->sequence.end());
    }

    sorted_iterator sorted_begin() const {
        if (!data_) return sorted_iterator();
        return sorted_iterator(data_->index.begin());
    }

    sorted_iterator sorted_end() const {
        if (!data_) return sorted_iterator();
        return sorted_iterator(data_->index.end());
    }
};

} // namespace cxx

#endif // PLAYLIST_H