#!/bin/bash

# Kolory dla czytelności
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Flagi kompilatora
CXX="g++"
# CXXFLAGS="-Wall -Wextra -O2 -std=c++23"
CXXFLAGS="
  -Wall
  -Wextra
  -Wpedantic
  -Wshadow
  -Woverloaded-virtual
  -Wformat=2
  -Wformat-security
  -Wimplicit-fallthrough
  -Wcast-align
  -Wcast-qual
  -Wunused
  -Wuninitialized
  -Wmaybe-uninitialized
  -O2
  -std=c++23
"


# Komenda Valgrind (dokładnie taka jak w poleceniu)
VALGRIND_CMD="valgrind --error-exitcode=123 --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all --run-cxx-freeres=yes -q"

# Funkcja do czyszczenia po testach
cleanup() {
    rm -f *.o *.out out.txt my_out.txt
}

# Obsługa przerwania skryptu (Ctrl+C)
trap cleanup EXIT

echo -e "${YELLOW}=== Rozpoczynanie testów playlist.h ===${NC}"

# Sprawdzenie czy plik rozwiązania istnieje
if [ ! -f "playlist.h" ]; then
    echo -e "${RED}BŁĄD: Nie znaleziono pliku playlist.h!${NC}"
    exit 1
fi

# ---------------------------------------------------------
# KROK 1: Test playlist_example (Kompilacja + Diff + Valgrind)
# ---------------------------------------------------------
echo -e "\n${YELLOW}--- Testowanie playlist_example.cpp ---${NC}"

# 1. Kompilacja
echo -n "Kompilacja... "
$CXX $CXXFLAGS playlist_example.cpp -o playlist_example.o
if [ $? -ne 0 ]; then
    echo -e "${RED}BŁĄD KOMPILACJI${NC}"
    exit 1
else
    echo -e "${GREEN}OK${NC}"
fi

# 2. Sprawdzenie outputu (diff)
echo -n "Sprawdzanie poprawności wyjścia (diff)... "
if [ ! -f "playlist_example.log" ]; then
    echo -e "${RED}Brak pliku playlist_example.log do porównania!${NC}"
else
    # Uruchomienie i przekierowanie stderr (2>) do pliku, tak jak w poleceniu
    ./playlist_example.o 2> my_out.txt
    
    # Porównanie
    DIFF_OUT=$(diff my_out.txt playlist_example.log)
    if [ "$DIFF_OUT" != "" ]; then
        echo -e "${RED}BŁĄD${NC}"
        echo "Oczekiwano innej zawartości. Wynik diff:"
        diff my_out.txt playlist_example.log
        exit 1
    else
        echo -e "${GREEN}OK${NC}"
    fi
fi

# 3. Valgrind
echo -n "Sprawdzanie pamięci (Valgrind)... "
$VALGRIND_CMD ./playlist_example.o > /dev/null 2>&1
RET=$?
if [ $RET -eq 123 ]; then
    echo -e "${RED}WYCIEKI PAMIĘCI!${NC}"
    # Uruchom ponownie, aby pokazać błędy na ekranie
    $VALGRIND_CMD ./playlist_example.o > /dev/null
    exit 1
elif [ $RET -ne 0 ]; then
    echo -e "${RED}BŁĄD WYKONANIA (Segfault/Exception) Kod: $RET${NC}"
    exit 1
else
    echo -e "${GREEN}OK${NC}"
fi

# ---------------------------------------------------------
# KROK 2: Testy automatyczne 1-4 (Kompilacja + Valgrind)
# ---------------------------------------------------------

# Lista plików testowych
TEST_FILES=("playlist_tests1.cpp" "playlist_tests2.cpp" "playlist_tests3.cpp" "playlist_tests4.cpp" "playlist_tests5.cpp")

for FILE in "${TEST_FILES[@]}"; do
    # Wyciągnij nazwę bez rozszerzenia (np. playlist_tests1)
    BASENAME=$(basename "$FILE" .cpp)
    OUTPUT_BIN="${BASENAME}.o"

    echo -e "\n${YELLOW}--- Testowanie $FILE ---${NC}"

    # Sprawdzenie czy plik testowy istnieje
    if [ ! -f "$FILE" ]; then
        echo -e "${RED}Pominięto (brak pliku $FILE)${NC}"
        continue
    fi

    # 1. Kompilacja
    echo -n "Kompilacja... "
    $CXX $CXXFLAGS "$FILE" -o "$OUTPUT_BIN"
    if [ $? -ne 0 ]; then
        echo -e "${RED}BŁĄD KOMPILACJI${NC}"
        exit 1
    else
        echo -e "${GREEN}OK${NC}"
    fi

    # 2. Valgrind (uruchamia też testy logiczne zawarte w pliku)
    echo -n "Uruchamianie testów i sprawdzanie pamięci... "
    
    # Przechwyć output, żeby nie śmiecić, chyba że będzie błąd
    $VALGRIND_CMD "./$OUTPUT_BIN" > /dev/null 2>&1
    RET=$?

    if [ $RET -eq 123 ]; then
        echo -e "${RED}WYCIEKI PAMIĘCI!${NC}"
        echo "Szczegóły valgrind:"
        $VALGRIND_CMD "./$OUTPUT_BIN"
        exit 1
    elif [ $RET -ne 0 ]; then
        echo -e "${RED}BŁĄD LOGICZNY LUB CRASH (Kod: $RET)${NC}"
        echo "Uruchamiam bez Valgrinda, aby zobaczyć komunikat błędu:"
        "./$OUTPUT_BIN"
        exit 1
    else
        echo -e "${GREEN}OK${NC}"
    fi
done

echo -e "\n${GREEN}=== WSZYSTKIE TESTY ZALICZONE GRATULACJE ===${NC}"
echo "Pamiętaj, że ostrzeżenia kompilatora (warnings) też odejmują punkty."
echo "Sprawdź powyżej, czy przy kompilacji nie pojawił się tekst inny niż 'OK'."