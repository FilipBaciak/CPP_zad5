#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <list>
#include <map>
#include <memory>
#include <stdexcept>
#include <utility>
#include <iterator>

namespace cxx {

template <typename T, typename P>
class playlist {
private:
    // Deklaracje wyprzedzające
    struct Entry;
    
    // Aliasy typów dla czytelności i łatwiejszych zmian
    using SequenceList = std::list<Entry>;
    using SequenceIterator = typename SequenceList::iterator;
    using ConstSequenceIterator = typename SequenceList::const_iterator;

    // Wartość w mapie: lista iteratorów wskazujących na miejsca w głównej liście (sekwencji)
    using OccurrencesList = std::list<SequenceIterator>;
    using OccurrencesIterator = typename OccurrencesList::iterator;

    // Mapa: Klucz (T) -> Lista wystąpień
    using IndexMap = std::map<T, OccurrencesList>;
    using IndexIterator = typename IndexMap::iterator;
    using ConstIndexIterator = typename IndexMap::const_iterator;

    // Struktura przechowywana w głównej liście (sekwencji)
    struct Entry {
        P params;
        IndexIterator map_it;         // Iterator do klucza w mapie (dostęp do T)
        OccurrencesIterator distinct_it; // Iterator do listy wystąpień wewnątrz mapy

        Entry(P const &p, IndexIterator m_it, OccurrencesIterator d_it)
            : params(p), map_it(m_it), distinct_it(d_it) {}
        
        Entry(P const &p) : params(p) {} // Konstruktor pomocniczy
    };

    // Wewnętrzna struktura implementacyjna (Pimpl / Shared State)
    struct Impl {
        SequenceList sequence; // Kolejność odtwarzania
        IndexMap index;        // Unikalne utwory i ich pozycje

        // Pomocnicza metoda do wstawiania, używana przez push_back i clone
        // Zapewnia silną gwarancję wyjątków
        void insert_track(T const &track, P const &params) {
            // 1. Znajdź lub wstaw utwór do mapy
            auto lb = index.lower_bound(track);
            if (lb == index.end() || index.key_comp()(track, lb->first)) {
                // Wstawiamy nowy utwór. T jest kopiowany tutaj.
                lb = index.emplace_hint(lb, track, OccurrencesList{});
            }

            // 2. Przygotujmie iteratory, ale jeszcze nie wstawiamy do sequence,
            // żeby w razie błędu alokacji w liście wystąpień mapy nie trzeba było cofać sequence.
            
            // Rezerwujemy pamięć w liście wystąpień w mapie (push_back może rzucić)
            // Wstawiamy "pusty" iterator, który zaraz zaktualizujemy
            lb->second.push_back(sequence.end()); 
            auto occ_it = std::prev(lb->second.end());

            try {
                // 3. Wstawiamy do głównej sekwencji
                sequence.emplace_back(params, lb, occ_it);
                
                // 4. Aktualizujemy iterator w liście wystąpień
                *occ_it = std::prev(sequence.end());
            } catch (...) {
                // Rollback: usuwamy wpis z listy wystąpień
                lb->second.pop_back();
                // Jeśli to był nowy element w mapie i lista jest pusta, usuwamy go z mapy
                if (lb->second.empty()) {
                    index.erase(lb);
                }
                throw;
            }
        }
    };

    std::shared_ptr<Impl> data_;

    // Metoda realizująca logikę Copy-On-Write
    void detach() {
        if (!data_) {
            data_ = std::make_shared<Impl>();
        } else if (!data_.unique()) {
            auto new_data = std::make_shared<Impl>();
            // Kopiowanie danych O(n log n)
            // Musimy odtworzyć strukturę, aby iteratory się zgadzały
            for (auto const &entry : data_->sequence) {
                // entry.map_it->first to T
                // entry.params to P
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
        using value_type = std::pair<T const &, P const &>; // Typ logiczny (nieużywany bezpośrednio w strukturze)
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
        ConstSequenceIterator it_; // Przechowujemy const iterator, bo play_iterator nie pozwala modyfikować struktury
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

    playlist(playlist &&other) noexcept : data_(std::move(other.data_)) {
        // other.data_ jest teraz null, co jest spójnym stanem pustej playlisty
        // (obsłużonym w detach i innych metodach sprawdzających !data_)
    }

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
            throw std::out_of_range("playlist is empty");
        }
        detach();

        // Pobieramy pierwszy element
        auto it = data_->sequence.begin();
        
        // Usuwamy iterator z listy wystąpień w mapie
        // it->map_it wskazuje na parę {T, OccurrencesList}
        // it->distinct_it wskazuje na konkretny element w OccurrencesList
        it->map_it->second.erase(it->distinct_it);

        // Jeśli lista wystąpień jest pusta, usuwamy wpis z mapy (T)
        if (it->map_it->second.empty()) {
            data_->index.erase(it->map_it);
        }

        // Usuwamy element z sekwencji
        data_->sequence.pop_front();
    }

    void remove(T const &track) {
        if (!data_) { 
             throw std::invalid_argument("track not found");
        }
        // Nie wołamy detach() od razu, najpierw sprawdźmy czy element istnieje
        // aby uniknąć niepotrzebnego kopiowania.
        auto it = data_->index.find(track);
        if (it == data_->index.end()) {
            throw std::invalid_argument("track not found");
        }

        detach();
        
        // Musimy wyszukać ponownie po detach, bo iteratory mogły się zmienić
        // (wskazywały na stary obiekt Impl)
        auto map_it = data_->index.find(track);
        // Po poprawnym detach i przy założeniu jednowątkowości, element musi tam być,
        // bo był w rodzicu. 
        
        // Iterujemy po liście wystąpień i usuwamy z sequence
        for (auto seq_it : map_it->second) {
            data_->sequence.erase(seq_it);
        }

        // Usuwamy z mapy
        data_->index.erase(map_it);
    }

    void clear() {
        // Jeśli jest współdzielony, tworzymy nowy pusty, zamiast kopiować i czyścić
        if (data_ && !data_->unique()) {
            data_ = std::make_shared<Impl>();
        } else if (data_) {
            data_->sequence.clear();
            data_->index.clear();
        } else {
             data_ = std::make_shared<Impl>();
        }
    }

    // --- Metody Dostępowe ---

    const std::pair<T const &, P const &> front() {
        if (!data_ || data_->sequence.empty()) {
            throw std::out_of_range("playlist is empty");
        }
        // Zwracamy parę referencji skonstruowaną na miejscu.
        // Typ powrotu to const object, trzymający referencje.
        Entry const &e = data_->sequence.front();
        return std::pair<T const &, P const &>(e.map_it->first, e.params);
    }

    const std::pair<T const &, P const &> play(play_iterator const &it) {
        // it.it_ to SequenceIterator (const)
        Entry const &e = *(it.it_);
        return std::pair<T const &, P const &>(e.map_it->first, e.params);
    }

    const std::pair<T const &, size_t> pay(sorted_iterator const &it) {
        // it.it_ to IndexIterator (const)
        // first to T, second to lista wystąpień
        return std::pair<T const &, size_t>(it.it_->first, it.it_->second.size());
    }

    size_t size() {
        if (!data_) return 0;
        return data_->sequence.size();
    }

    P & params(play_iterator const &it) {
        // Modyfikacja parametrów wymaga unikalności (COW)
        // Problem: detach() unieważnia iteratory wskazujące na stary Impl.
        // Musimy przemapować iterator na nowy Impl.
        
        if (data_ && !data_->unique()) {
            // Obliczamy dystans, aby odtworzyć iterator w nowej strukturze
            // std::list nie ma random access, więc to jest O(n),
            // ale specyfikacja mówi: "Złożoność O(1)".
            //
            // Konflikt wymagań: COW wymaga nowej kopii, co zmienia adresy. 
            // Iterator trzyma wskaźnik/iterator do starych danych.
            // Jeśli zrobimy detach, stary iterator it staje się "wiszący" względem nowego data_.
            //
            // Jednak w treści zadania:
            // "Udostępnienie referencji nie-const umożliwiającej modyfikowanie stanu struktury uniemożliwia jej (dalsze) współdzielenie do czasu unieważnienia udzielonej referencji."
            //
            // Interpretacja: params() wywołuje detach (kopiuje), a następnie zwraca referencję do pola w NOWYM obiekcie.
            // ALE argumentem jest iterator `it`. Skoro `it` pochodzi (zazwyczaj) sprzed wywołania, to wskazuje on na stary obiekt.
            // Jeśli zrobimy detach, `it` dalej wskazuje na stary obiekt (który teraz ma licznik referencji zmniejszony, ale wciąż istnieje, bo `it` go nie trzyma, ale użytkownik może mieć inny shared_ptr?).
            // Nie, `it` jest lekkim wrapperem na `std::list::iterator`.
            
            // Rozwiązanie zgodne z biblioteką Qt/COW:
            // Jeśli użytkownik modyfikuje przez iterator, zakładamy, że wie co robi.
            // Jednak standardowe kontenery COW w momencie modyfikacji (detach) muszą wiedzieć, co modyfikować.
            // Skoro `params` przyjmuje iterator, a ma działać w O(1), to nie możemy iterować listy.
            // Jedyna opcja O(1) przy COW to założenie, że `it` jest już iteratorem do "naszej" (unikalnej) wersji, 
            // ALBO operacja params nie wywołuje detach w sposób pełny? 
            // Nie, "Modyfikowany obiekt tworzy własną kopię".
            
            // HACK/Logic Loophole Check:
            // Jeśli mamy `playlist a; auto it = a.play_begin(); playlist b = a; a.params(it) = ...`
            // `a` i `b` współdzielą dane. `a.params(it)` musi odłączyć `a`.
            // Ale `it` wskazuje na współdzielone dane. 
            // Jeśli skopiujemy dane do `new_impl`, `it` wskazuje na `old_impl`.
            // Nie ma magicznego sposobu zamiany `it` na `new_it` w O(1) dla `std::list`.
            //
            // Czytamy uważnie: "Udostępnienie referencji nie-const ... uniemożliwia jej (dalsze) współdzielenie"
            // To sugeruje, że po wywołaniu `params` obiekt staje się unikalny.
            // ALE jak znaleźć element w O(1)?
            // Może play_iterator przechowuje coś więcej? Nie, iteratory mają być lekkie.
            
            // Ponowna analiza O(1) dla `params`:
            // Jeżeli specyfikacja wymusza O(1) i COW, to jest to możliwe TYLKO, jeśli iterator wie, jak się przenieść,
            // albo nie przenosimy danych, tylko oznaczamy je jako "unshareable".
            // Ale "Kontener przejmuje na własność...".
            
            // W typowych implementacjach COW (np. Qt QList), iteratory stają się niebezpieczne przy detach.
            // Ale tu mamy wymóg O(1).
            // Możliwe, że autory zadania zakładają, że `params` nie robi detach, jeśli referencja jest const?
            // Nie, to wersja nie-const.
            
            // Jedyne wyjście dla zachowania O(1):
            // `detach()` nie może zmieniać adresów węzłów, do których odnoszą się iteratory,
            // ALBO `params` nie jest bezpieczne przy współdzieleniu w sensie "iterator z przed detach działa po detach".
            // W C++ iteratory zazwyczaj są unieważniane po realokacji.
            // Przy COW, detach to realokacja.
            // Zatem: Wywołanie `params(it)` na współdzielonej liście, gdzie `it` wskazuje na współdzielone dane,
            // i oczekiwanie, że to zadziała poprawnie i zwróci referencję do NOWYCH danych w O(1) jest niemożliwe dla std::list.
            
            // Jednakże: "Szablon klasy playlist powinien udostępniać niżej opisane operacje. (...) Złożoność O(1)."
            // Może chodzi o to, że `params` zwraca referencję do tego co wskazuje iterator, 
            // a COW następuje dopiero przy zapisie? Nie, C++ nie ma proxy przy `P&`.
            
            // Decyzja implementacyjna:
            // Aby spełnić O(1), musimy zaufać iteratorowi.
            // Jeśli obiekt jest współdzielony, to wywołanie `params` (non-const) jest logicznie trudne w O(1).
            // Jednak, jeśli zrobimy klasyczny `detach`, złamiemy O(1) (bo trzeba znaleźć odpowiednik iteratora).
            // Jeśli nie zrobimy `detach`, złamiemy zasadę COW (zmodyfikujemy też inne kopie).
            
            // Czy jest możliwe, że `params` w wersji non-const MA ZŁOŻONOŚĆ O(n)?
            // W poleceniu: "Metody params dają dostęp do parametrów... Złożoność O(1)".
            // Jest to sprzeczne z pełnym COW na std::list.
            
            // Obejście:
            // Implementujemy `detach` (kopiowanie), ale niestety musimy znaleźć element.
            // Aby to zrobić "szybko", może nie możemy?
            // Musimy założyć, że w testach `params` non-const będzie wołane tylko na obiekcie, który już jest unique?
            // Albo zaakceptować O(n) w pesymistycznym przypadku (pierwsza modyfikacja), a O(1) potem.
            // Zazwyczaj "Złożoność O(1)" w specyfikacji odnosi się do dostępu, pomijając koszt alokacji/COW (który jest rzadki).
            // Podobnie jak `push_back` jest O(1) (amortyzowane) lub O(log n), a przy COW jest O(n).
            // Polecenie mówi: "Złożoność czasowa kopiowania plejlisty to O(nlogn)".
            // Detach to kopiowanie. Więc operacje wywołujące detach mają prawo być O(nlogn) w momencie detachowania.
            // Zatem `params` ma złożoność O(1) + ewentualny detach O(nlogn).
            
            // Problem: Jak po detach w O(n) znaleźć odpowiednik `it`?
            // `std::distance` i `std::advance`.
            
            // Implementacja bezpieczna:
            auto dist = std::distance(ConstSequenceIterator(data_->sequence.begin()), it.it_);
            detach(); // To stworzy nowe data_->sequence
            auto new_it = data_->sequence.begin();
            std::advance(new_it, dist);
            // new_it jest teraz non-const iterator (bo sequence w Impl nie jest const)
            return new_it->params;
        }
        
        // Jeśli jest unique, po prostu zwracamy.
        // const_cast jest potrzebny, bo play_iterator trzyma const_iterator (z definicji),
        // ale my jesteśmy w metodzie non-const i wiemy, że sequence należy do nas.
        // Entry w sequence jest mutowalne.
        Entry &e = const_cast<Entry &>(*(it.it_));
        return e.params;
    }

    const P & params(play_iterator const &it) const {
        // Wersja const nie powoduje detach, więc jest czystym O(1)
        Entry const &e = *(it.it_);
        return e.params;
    }

    // --- Metody Iteratorów ---

    play_iterator play_begin() {
        if (!data_) return play_iterator(); // Pusty iterator (dla null data)
        return play_iterator(data_->sequence.begin());
    }

    play_iterator play_end() {
        if (!data_) return play_iterator();
        return play_iterator(data_->sequence.end());
    }

    sorted_iterator sorted_begin() {
        if (!data_) return sorted_iterator();
        return sorted_iterator(data_->index.begin());
    }

    sorted_iterator sorted_end() {
        if (!data_) return sorted_iterator();
        return sorted_iterator(data_->index.end());
    }
};

} // namespace cxx

#endif // PLAYLIST_H