=== Zadanie 2 (duże) ===

Zadanie składa się z dwóch części.

== Część A ==

Zaimplementować program player umożliwiający odtwarzanie radia internetowego
i zapisywanie odbieranego dźwięku do pliku. Program ma ściągać strumień audio
w formacie MP3 za pomocą protokołu Shoutcast znanego też pod nazwą ICY.
Komunikacja wygląda jak w HTTP. Klient łączy się z serwerem radia za pomocą TCP
i wysyła zapytanie GET. Odpowiedź serwera składa się (jak w HTTP) z: linii
statusu, ciągu tekstowych pól nagłówka oraz pustej linii. Następnie serwer
rozpoczyna wysyłanie binarnego strumienia audio ewentualnie zmultipleksowanego
z tekstowymi metadanymi. Sterowanie programem będzie się odbywać przez sieć za
pomocą poleceń przesyłanych po UDP.

Wywołanie programu:

./player host path r-port file m-port md

Parametry:

host   – nazwa serwera udostępniającego strumień audio;
path   – nazwa zasobu, zwykle sam ukośnik;
r-port – numer portu serwera udostępniającego strumień audio,
         liczba dziesiętna;
file   – nazwa pliku, do którego zapisuje się dane audio,
         a znak minus, jeśli strumień audio ma być wysyłany na standardowe
         wyjście (w celu odtwarzania na bieżąco);
m-port – numer portu UDP, na którym program nasłuchuje poleceń,
         liczba dziesiętna;
md     – no, jeśli program ma nie żądać przysyłania metadanych,
         yes, jeśli program ma żądać przysyłania metadanych.

Do odtwarzania dźwięku na bieżąco można na przykład wykorzystać program
mplayer, przekierowując standardowe wyjście programu player na standardowe
wejście programu mplayer.

Przykładowe wywołania:

./player stream3.polskieradio.pl / 8900 - 10000 no | mplayer - [opcje]
./player stream3.polskieradio.pl / 8904 test3.mp3 30000 no
./player ant-waw-01.cdn.eurozet.pl / 8602 test5.mp3 50000 yes

Polecenia obsługiwane przez program player:

PAUSE – wstrzymuje odtwarzanie strumienia audio lub jego zapisywanie do pliku;
PLAY  – wznawia odtwarzanie strumienia audio lub jego zapisywanie do pliku;
TITLE – odsyła ostatnio odebraną wartość pola StreamTitle metadanych;
QUIT  – kończy działanie programu.

Polecenia przesyła się jako napisy ASCII bez terminalnego zera. Odpowiedź na
polecenie TITLE przesyła się do jego nadawcy jako tekst ASCII bez terminalnego
zera. Proces przysyłający polecenia może być uruchomiony na innej maszynie niż
program odtwarzacza audio.

Program musi być odporny na podanie złej liczby parametrów, na niepoprawne
wartości parametrów i nieprawidłowe dane przysyłane przez serwer radia
internetowego. Błąd należy sygnalizować, wypisując stosowny komunikat na
standardowe wyjście błędów i kończąc program z kodem powrotu 1.

Program musi być odporny na nieprawidłowe polecenia przysyłane przez
sieć. W tym przypadku należy wypisać stosowny komunikat na standardowe
wyjście błędów i zignorować takie polecenie. Program nie powinien się
zawieszać. W szczególności należy przyjąć maksymalny czas oczekiwania na
zbudowanie połączenia TCP wynikający z implementacji interfejsu gniazd, a
maksymalny czas oczekiwania na dane od serwera radia internetowego należy
ustalić na 5 sekund. Poprawne zakończenie programu (w wyniku wykonania
polecenia QUIT lub zamknięcia połączenia przez serwer radia internetowego w
trakcie przysyłania strumienia audio) należy sygnalizować kodem powrotu 0.

Serwer radia nie dba o ramkowanie danych MP3. Pierwsza przysłana ramka jest
zwykle niekompletna – brakuje początku. Przy kończeniu programu ostatnia ramka
może zostać urwana. Nie należy implementować rozpoznawania ramek MP3 – strumień
audio zapisywany do pliku lub wysyłany na standardowe wyjście powinien być
identyczny z tym odebranym od serwera.

Zanim zacznie się zadawać pytania prowadzącym, należy poczytać:

http://www.garymcgath.com/streamingprotocols.html
http://www.indexcom.com/streaming/player/SHOUTcast.html
http://www.smackfu.com/stuff/programming/shoutcast.html
http://mpgedit.org/mpgedit/mpeg_format/mpeghdr.htm
http://www.listenlive.eu/poland.html

== Część B ==

Zaimplementować program zawiadujący programami ściągającymi strumień audio
radia internetowego. Do komunikacji z użytkownikiem zawiadowca ma udostępniać
interfejs tekstowy za pomocą protokołu telnet.

Wywołanie programu:

./master [port]

Parametr:

port – opcjonalny numer portu, na którym zawiadowca ma odbierać sesje telnetowe;
       jeśli nie podano portu, zawiadowca uruchamia nasłuchiwanie na wolnym
       porcie i wypisuje jego numer; liczba dziesiętna.

Zawiadowca musi być odporny na podanie złej liczby parametrów oraz na
niepoprawną wartość parametru. Błąd przy uruchamianiu należy sygnalizować,
wypisując stosowny komunikat na standardowe wyjście błędów i kończąc
zawiadowcę z kodem powrotu 1.

Zawiadowca musi być odporny na nieprawidłowe dane przysyłane przez sieć. Takie
dane nie powinny doprowadzać do zawieszenia się zawiadowcy, naruszenia ochrony
pamięci itp.

Zawiadowca ma umożliwiać:
– uruchomienie programu ściągającego z podanymi przez użytkownika parametrami
  gdziekolwiek w sieci lokalnej;
– wydawanie poleceń działającemu programowi ściągającemu; zawiadowca czeka
  maksymalnie 3 sekundy na odpowiedź na polecenie TITLE;
– wyświetlenie komunikatu o zakończeniu działania programu ściągającego wraz
  z ewentualnym komunikatem o przyczynie (błędzie) jego zakończenia;
– wydawanie polecenia „od godz. HH.MM ściągaj z radia X audycję przez M minut”;
  odmierzanie czasu ma być zrealizowane po stronie zawiadowcy;
– równoległą obsługę wielu sesji telnetowych;
– symultaniczną obsługę wielu programów ściągających w jednej sesji telnetowej.

Komunikacja odbywa się normalną sesją telnetową:

telnet <master> <port-nasłuchujący>

Każde polecenie podawane jest jako pojedynczy wiersz tekstu, na początku wiersza
nazwa polecenia, potem rozdzielone spacjami parametry. Z wyjątkiem poleceń START
i AT pierwszym parametrem jest magiczny identyfikator ID uprzednio uruchomionego
egzemplarza programu ściągającego (zwracany przez polecenia START i AT).

Po wyświetleniu komunikatu o zakończeniu działania programu ściągającego
(komunikat ma zawierać jego ID) identyfikator jest unieważniany.

Polecenia obsługiwane przez zawiadowcę:

START <komputer> <parametr-dla-playera> ...

  Uruchamia program ściągający na podanym komputerze z podanymi parametrami.
  Przy poprawnym wykonaniu odpowiada OK wraz z ID, którego należy używać
  w pozostałych poleceniach.

<polecenie-playera> <id>

  Jedno z poleceń do przesłania programowi player (PAUSE, PLAY, TITLE, QUIT).
  Przy poprawnym wykonaniu odpowiada OK wraz z ID, a dla TITLE odpowiedź
  dodatkowo zawiera wartość StreamTitle.

AT <HH.MM> <M> <komputer> <parametr-dla-playera> ...

  Wersja polecenia START z opóźnionym uruchomieniem programu player. Parametry
  <HH.MM> i <M> to czas uruchomienia i czas działania. Odmierzaniem czasu
  zajmuje się zawiadowca, tzn. o podanej godzinie startuje program player, czeka
  podaną liczbę minut i wyłącza go. Parametr <file> dla programu player musi być
  nazwą pliku (a nie znakiem -). Przy poprawnym wykonaniu odpowiada OK wraz
  z ID, którego należy używać w pozostałych poleceniach.

W przypadku jakiegokolwiek błędu w odpowiedniej sesji telnetowej ma być wypisana
informacja o błędzie zaczynająca się słowem ERROR i ewentualnie ID, jeśli błąd
dotyczy konkretnego uruchomionego programu player.

Zakładamy, że program player znajduje się w katalogu, którego nazwa jest
skonfigurowana wśród ścieżek poszukiwań plików wykonywalnych na docelowym
komputerze, na którym chcemy go uruchomić.

Uwagi do protokołu i programu telnet:
– Protokół telnet oprócz tekstu przewiduje przesyłanie sekwencji sterujących
  zaczynających się od bajtu o wartości 255. Mają one dwa lub trzy bajty.
  Należy być przygotowanym na takie sekwencje.
– Wysyłane wiersze domyślnie kończą się parą znaków CR/LF, ale jest to
  konfigurowalne, więc należy być przygotowanym na najgorsze (np. samo CR).

== Oddawanie rozwiązania ==

Można oddać rozwiązanie tylko części A lub tylko części B, albo obu części.

Rozwiązanie ma:
– działać w środowisku Linux;
– być napisane w języku C lub C++ z wykorzystaniem interfejsu gniazd (nie wolno
  korzystać z libevent ani boost::asio);
– kompilować się za pomocą GCC (polecenie gcc lub g++) – wśród parametrów należy
  użyć -Wall i -O2.

Jako rozwiązanie należy dostarczyć pliki źródłowe oraz plik makefile, które
należy umieścić w repozytorium SVN

https://svn.mimuw.edu.pl/repos/SIK/

w katalogu

students/ab123456/zadanie2/

gdzie ab123456 to standardowy login osoby oddającej rozwiązanie, używany na
maszynach wydziału, wg schematu: inicjały, nr indeksu.

W wyniku wykonania polecenia make dla części A zadania ma powstać plik
wykonywalny player, a dla części B zadania – plik wykonywalny master.

== Ocena ==

Za rozwiązanie części A zadania można dostać maksymalnie 2 punkty.
Za rozwiązanie części B zadania można dostać maksymalnie 3 punkty.
Za rozwiązanie obu części zadania można dostać maksymalnie 6 punktów.

Jeśli ktoś implementuje tylko część B, to w celu testowania może napisać sobie
zaślepkę programu player. Prowadzący może testować część B z programem player
dostarczonym przez siebie.

Jeśli student odda obie części zadania, to będą one ocenione osobno.
Jeśli obie części współdziałają ze sobą i każda z nich wykazuje działanie
zgodne ze specyfikacją, ocena końcowa będzie sumą ocen za poszczególne części
pomnożoną przez 1,2.

Ocena każdej z części zadania będzie się składała z trzech składników:
– ocena wzrokowa i słuchowa działania programu (20% w części A, 60% w części B);
– testy automatyczne (60% w części A, 20% w części B);
– jakość tekstu źródłowego (20%).

Termin: poniedziałek 30 maja 2016, godzina 19.00
        (liczy się czas na serwerze SVN)

Za spóźnienie do 24h otrzymaną ocenę pomniejsza się o 1 p.
Za spóźnienie powyżej 24h, ale do 7 dni przed egzaminem – o 2 p.
Rozwiązanie z późniejszą datą można oddać tylko w II terminie.

Rozwiązanie dostarczone w I terminie można poprawić jednokrotnie,
w II terminie.

W II terminie nie obowiązują minusy za spóźnienia. Rozwiązania z datą
późniejszą niż 7 dni przed egzaminem poprawkowym nie podlegają ocenie.
