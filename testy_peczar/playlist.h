#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <list>
#include <map>
#include <memory>
#include <stdexcept>
#include <utility>
#include <iterator>
#include <cstddef>
#include <type_traits>

namespace cxx
{

    template <typename T, typename P>
    class playlist
    {
    private:
        struct Entry;
        bool forceCopy = false;

        // Type aliases.

        // List ordered by adding order.
        using SequenceList = std::list<Entry>;
        using SequenceIterator = typename SequenceList::iterator;
        using ConstSequenceIterator = typename SequenceList::const_iterator;

        // List of iterators to the sequence list for instant access.
        using OccurrencesList = std::list<SequenceIterator>;
        using OccurrencesIterator = typename OccurrencesList::iterator;

        // Sorted map, which gives chronological order.
        using IndexMap = std::map<T, OccurrencesList>;
        using IndexIterator = typename IndexMap::iterator;
        using ConstIndexIterator = typename IndexMap::const_iterator;

        // Each entry contains parameters of the track and iterators.
        struct Entry
        {
            mutable P params;
            IndexIterator map_it;
            OccurrencesIterator distinct_it;

            Entry(P const &p,
                IndexIterator m_it,
                OccurrencesIterator d_it)
                : params(p), map_it(m_it), distinct_it(d_it) {}

            explicit Entry(P const &p)
            : params(p) {}
        };

        // The structure that stores all the data in the playlist.
        struct Impl
        {
            SequenceList sequence;
            IndexMap index;

            // Adds the {iterator, param} to the sequence and updates map {track,
            // list of iterators to the position at list}.
            void insert_track(T const &track, P const &params)
            {
                // Try to add to the map
                auto target_it = index.lower_bound(track);
                bool insert_new = false;
                if (target_it == index.end() || target_it->first != track)
                {
                    target_it = index.emplace_hint(
                        target_it, track, OccurrencesList{});
                    insert_new = true;
                }

                // Prepare for allocation error.
                try
                {
                    target_it->second.push_back(sequence.end());
                }
                catch (...)
                {
                    if (insert_new)
                        index.erase(target_it);
                    throw;
                }

                auto occurrence_it = std::prev(target_it->second.end());
                // Add and handle allocation error.
                try
                {
                    // Add to main list.
                    sequence.emplace_back(params, target_it, occurrence_it);
                    // Aktualizacja iteratora w mapie
                    *occurrence_it = std::prev(sequence.end());
                }
                catch (...)
                {
                    target_it->second.pop_back();
                    if (target_it->second.empty())
                    {
                        index.erase(target_it);
                    }
                    throw;
                }
            }
        };

        std::shared_ptr<Impl> data_;
        std::shared_ptr<Impl> safeguard_;

        // Allows to manage copy on write or allocation
        // of *data_ if such does not exist.
        // This is also exception safe, as shared pointers have destructors.
        void detach()
        {
            if (!data_)
            {
                data_ = std::make_shared<Impl>();
            }
            else if (data_.use_count() > 1)
            {
                auto new_data = std::make_shared<Impl>();
                for (auto const &entry : data_->sequence)
                {
                    new_data->insert_track(entry.map_it->first, entry.params);
                }
                data_ = std::move(new_data);
            }
        }

        // Detach improved by a safeguard mechanism in case of an error.
        void guardedDetach()
        {
            if (!data_)
            {
                data_ = std::make_shared<Impl>();
            }
            else if (data_.use_count() > 1)
            {
                auto new_data = std::make_shared<Impl>();
                for (auto const &entry : data_->sequence)
                {
                    new_data->insert_track(entry.map_it->first, entry.params);
                }
                safeguard_ = data_;
                data_ = std::move(new_data);
            }
        }

        void reverseDetach()
        {
            if(safeguard_ != nullptr)
            {
                data_ = safeguard_;
                safeguard_.reset();
            }
        }

        void finalizeDetach()
        {
            forceCopy = false;
            safeguard_.reset();
        }

    public:
        // --- Iterators ---

        // Play iterator is just a wrapper to the iterator of a list.
        class play_iterator
        {
        public:
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = std::pair<T const &, P const &>;
            using difference_type = std::ptrdiff_t;
            using pointer = void;
            using reference = void;

            play_iterator() = default;

            bool operator==(play_iterator const &other) const
            {
                return it_ == other.it_;
            }

            bool operator!=(play_iterator const &other) const
            {
                return it_ != other.it_;
            }

            play_iterator &operator++()
            {
                ++it_;
                return *this;
            }
            play_iterator operator++(int)
            {
                play_iterator temp = *this;
                ++it_;
                return temp;
            }
            play_iterator &operator--()
            {
                --it_;
                return *this;
            }
            play_iterator operator--(int)
            {
                play_iterator temp = *this;
                --it_;
                return temp;
            }

        private:
            friend class playlist;
            ConstSequenceIterator it_;
            explicit play_iterator(ConstSequenceIterator it) : it_(it) {}
        };

        // Sorted iterator is just a wrapper to the iterator of a map.
        class sorted_iterator
        {
        public:
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = T const *;
            using reference = T const &;

            sorted_iterator() = default;

            bool operator==(sorted_iterator const &other) const
            {
                return it_ == other.it_;
            }

            bool operator!=(sorted_iterator const &other) const
            {
                return it_ != other.it_;
            }

            sorted_iterator &operator++()
            {
                ++it_;
                return *this;
            }
            sorted_iterator operator++(int)
            {
                sorted_iterator temp = *this;
                ++it_;
                return temp;
            }
            sorted_iterator &operator--()
            {
                --it_;
                return *this;
            }
            sorted_iterator operator--(int)
            {
                sorted_iterator temp = *this;
                --it_;
                return temp;
            }

            T const &operator*() const
            {
                return it_->first;
            }

            T const *operator->() const
            {
                return &(it_->first);
            }

        private:
            friend class playlist;
            ConstIndexIterator it_;
            explicit sorted_iterator(ConstIndexIterator it) : it_(it) {}
        };

        // --- Constructors & Destructor ---

        playlist() : data_(std::make_shared<Impl>()) {}

        playlist(playlist const &other) : data_(other.data_)
        {
            if(other.forceCopy) detach();
        }

        playlist(playlist &&other) : data_(std::move(other.data_)) {}

        ~playlist() noexcept = default;

        playlist &operator=(playlist other)
        {
            std::swap(data_, other.data_);
            return *this;
        }

        // --- Non Const Methods ---

        // Adds track and parameters at the end.
        // O(log n)
        void push_back(T const &track, P const &params)
        {
            guardedDetach();

            try
            {
                data_->insert_track(track, params);
            }
            catch (...)
            {
                reverseDetach();
                throw;
            }
            finalizeDetach();
        }

        // Removes first element from the playlist.
        // O(const)
        void pop_front()
        {
            // Handle special cases.
            if (!data_ || data_->sequence.empty())
            {
                throw std::out_of_range("pop_front, playlist empty");
            }
            guardedDetach();

            try
            {
                auto it = data_->sequence.begin();
                it->map_it->second.erase(it->distinct_it);

                if (it->map_it->second.empty())
                {
                    data_->index.erase(it->map_it);
                }
                data_->sequence.pop_front();
            }
            catch (...)
            {
                reverseDetach();
                throw;
            }

            finalizeDetach();
        }

        // Removes all occurences of a track from the playlist.
        // O(log n + k)
        void remove(T const &track)
        {
            // Check for presence.
            if (!data_)
            {
                throw std::invalid_argument("remove, unknown track");
            }
            auto it = data_->index.find(track);
            if (it == data_->index.end())
            {
                throw std::invalid_argument("remove, unknown track");
            }
            guardedDetach();

            try
            {
                // Find again after detach and remove.
                auto map_it = data_->index.find(track);
                for (auto seq_it : map_it->second)
                {
                    data_->sequence.erase(seq_it);
                }
                data_->index.erase(map_it);
            }
            catch (...)
            {
                reverseDetach();
                throw;
            }

            finalizeDetach();
        }

        // Clears the playlist.
        // O(n) when the data is not shared, O(const) when detach is possible.
        void clear()
        {
            // Detach into empty if possible, else just clean all data.
            if (data_ && data_.use_count() > 1)
            {
                data_.reset();
            }
            else if (data_)
            {
                data_->sequence.clear();
                data_->index.clear();
            }
            else
            {
                data_ = std::make_shared<Impl>();
            }
        }

        // Reads parameters of a given iterator.
        // O(const)
        // In the case of detaching here additional O(n) distance computation is
        // performed (overshadowed by O(n log n) deep copy.
        P &params(play_iterator const &it)
        {
            forceCopy = true;
            if (data_ && data_.use_count() > 1)
            {
                // If data is shared, a detach is needed, as the user is provided
                // with means to change the data.
                // After that to find the equivalent of the iterator in new
                // data, the safe way is to do it is by computing the distance
                // from the beginning.
                auto old_begin = data_->sequence.begin();
                auto dist = std::distance(
                    ConstSequenceIterator(old_begin), it.it_);
                detach();

                auto new_it = data_->sequence.begin();
                std::advance(new_it, dist);

                return new_it->params;
            }

            // This is another bypass of const, needed to return non const value.
            const auto &e = *it.it_;
            return e.params;
        }

        // --- Constant Getters (O(const) ---

        // Gets the first element of the queue as <T, P> pair.
        const std::pair<T const &, P const &> front() const
        {
            if (!data_ || data_->sequence.empty())
            {
                throw std::out_of_range("front, playlist empty");
            }
            Entry const &e = data_->sequence.front();
            return std::pair<T const &, P const &>(e.map_it->first, e.params);
        }

        // Gets the element of the queue under the iterator as <T, P> pair.
        const std::pair<T const &, P const &>
        play(play_iterator const &it) const noexcept
        {
            Entry const &e = *(it.it_);
            return std::pair<T const &, P const &>(e.map_it->first, e.params);
        }

        // Gets the track under the iterator and counts its occurences.
        const std::pair<T const &, size_t>
        pay(sorted_iterator const &it) const noexcept
        {
            return std::pair<T const &, size_t>(
                it.it_->first, it.it_->second.size());
        }

        // Gets the params of the track under the iterator.
        const P &params(play_iterator const &it) const
        {
            Entry const &e = *it.it_;
            return e.params;
        }

        // Gets the size of the playlist.
        size_t size() const noexcept
        {
            if (!data_)
                return 0;

            return data_->sequence.size();
        }

        // --- Constant Methods Returning Iterators O(const) ---

        // Gets iterator to the first element on the playlist.
        play_iterator play_begin() const noexcept
        {
            if (!data_)
                return play_iterator();

            return play_iterator(data_->sequence.begin());
        }

        // Gets iterator to the last element on the playlist.
        play_iterator play_end() const noexcept
        {
            if (!data_)
                return play_iterator();

            return play_iterator(data_->sequence.end());
        }

        // Gets iterator to the first element on the playlist in sorted order.
        sorted_iterator sorted_begin() const noexcept
        {
            if (!data_)
                return sorted_iterator();

            return sorted_iterator(data_->index.begin());
        }

        // Gets iterator to the last element on the playlist in sorted order.
        sorted_iterator sorted_end() const noexcept
        {
            if (!data_)
                return sorted_iterator();

            return sorted_iterator(data_->index.end());
        }
    };

} // namespace cxx

#endif // PLAYLIST_H