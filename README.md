# Laboratorium - synchronizacja w c++

### Pliki
Do laboratorium zostały dołączone następujące pliki:
<details><summary>:pushpin: Rozwiń </summary>
<p>

- [.clang-format](.clang-format)
- [.clang-tidy](.clang-tidy)
- [CMakeLists.txt](CMakeLists.txt)
- [condition.cpp](condition.cpp)
- [lock.cpp](lock.cpp)
- [log.hpp](log.hpp)
- [mutex.cpp](mutex.cpp)
- [thread-local-static.cpp](thread-local-static.cpp)
- [thread-local.cpp](thread-local.cpp)

</p>
</details>


## Mutex

Mutex reprezentowany jest obiektem klasy [std::mutex](https://en.cppreference.com/w/cpp/thread/mutex).

Przykład [mutex.cpp](mutex.cpp):

```cpp
#include <chrono>
#include <mutex>
#include <thread>

#include "log.hpp"

int shared { 0 };

void f(const std::string& name, std::mutex& mut, int loop_rep)
{
    for (int i = 0; i < loop_rep; i++) {
        // log() is a thread-safe wrapper for writing to cout (with a mutex).
        log("f ", name, " local section start");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        log("f ", name, " local section finish");

        mut.lock();
        log("f ", name, " critical section start");

        int local = shared;
        // Simulate some work in the critical section.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        local += 1;
        shared = local;

        log("f ", name, " critical section finish");
        mut.unlock();
    }
}

int main()
{
    int loop_rep { 10 };
    std::mutex mut;
    log("main starts");
    std::thread t1 { [&mut, loop_rep] { f("t1", mut, loop_rep); } };
    std::thread t2 { [&mut, loop_rep] { f("t2", mut, loop_rep); } };
    t1.join();
    t2.join();
    log("result is correct? ", (loop_rep * 2 == shared), "");
    log("main finishes");
}
```

#### Ćwiczenia:
- Zakomentuj zajmowanie i zwalnianie muteksu i sprawdź wynik.
- Sprawdź, co stanie się, gdy wątek będzie próbował zająć muteks, który już raz zajął.

Inne wersje muteksu:
- `std::recursive_mutex`: pojedynczy wątek może wielokrotnie zająć muteks.
- `std::timed_mutex`: ustawianie limitu czasu czekania.

## Zamek (lock)

Mutex musi być zwolniony (`mutex.unlock()`), co może być trudne do zapewnienia we wszystkich przepływach sterowania programu (np. gdy kod generuje wyjątki). Zamek `std::lock_guard<mutex_t> lock(mutex_t mut)` zajmuje muteks `mut` w swoim konstruktorze, a zwalnia go w destruktorze. Dzięki temu można w prosty sposób zapewnić zwalnianie muteksu w momencie opuszczenia bloku kodu (jakakolwiek byłaby przyczyna opuszczenia). Zamek realizuje zasadę [RAII](https://en.wikipedia.org/wiki/Resource_Acquisition_Is_Initialization) (resource acquisition is initialization).

[lock.cpp](lock.cpp), fragment funkcji f z przykładu powyżej realizujący sekcję krytyczną
```cpp
  { // A block in which 'lock' is a local variable.
      std::lock_guard<std::mutex> lock { mut }; // Constructor invokes mut.lock().
      log("f ", name, " critical section start");

      int local = shared;
      // Simulate some work in the critical section.
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      local += 1;
      shared = local;

      log("f ", name, " critical section finish");
  } // lock will be destroyed here, calling mut.unlock().
  
```

#### Ćwiczenia:
- zmień zawartość sekcji krytycznej tak, by wywoływała funkcję, która czasem zgłasza wyjątek. Porównaj działanie wersji z muteksem i z zamkiem.
- usuń nazwę zmiennej *lock*, pisząc `std::lock_guard<std::mutex>{mut};` lub `std::unique_lock<std::mutex>(mut);` upewnij się, że linker (np. clang-tidy) ostrzeże, iż zamek zostanie w tym przpadku zniszczony zaraz po utworzeniu.

Zamek `std::lock_guard` jest okrojoną wersją zamka `std::unique_lock`. Używając `std::unique_lock` dalej mamy gwarancję, że destruktor zwolni muteks, jeśli zamek go zajął, natomiast mamy większą kontrolę. Możemy:

- nie zajmować muteksu podczas inicjalizacji (konstruktor: `std::unique_lock lock(mut, std::defer_lock)`
- zająć `muteks lock.lock()`
- zwolnić `muteks lock.unlock()`
- spróbować zająć muteks `lock.try_lock()` (jeśli muteks jest zajęty, metoda zwróci `false`)
- czekać na muteks określony czas `lock.try_lock_for()` albo do określonej chwili `lock.try_lock_until()`.

#### Ćwiczenia:
- Zamień `std::lock_guard` na `std::unique_lock` z parametrem konstruktora `std::defer_lock`.

## Zmienne warunkowe (conditional variables)

Przykład [condition.cpp](condition.cpp):

```cpp
// This code is adapted from: http://en.cppreference.com/w/cpp/thread/condition_variable/wait
#include <array>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <thread>

#include "log.hpp"

std::condition_variable cv;
std::mutex mut; // This mutex is used for two purposes:
                // 1) to synchronize accesses to i,
                // 2) for the condition variable cv.
int i = 0;

void waits()
{
    std::unique_lock<std::mutex> lock(mut);
    log("Waiting...");
    cv.wait(lock, [] { return i == 1; }); // Wait ends iff i == 1.
    log("Finished waiting. i == ", i);
}

void signals()
{
    std::this_thread::sleep_for(std::chrono::seconds(1));
    {
        std::lock_guard<std::mutex> lock(mut);
        i = 5;
    }
    log("Notifying...");
    cv.notify_all(); // We don't need the mutex here.

    std::this_thread::sleep_for(std::chrono::seconds(1));
    {
        std::lock_guard<std::mutex> lock(mut);
        i = 1;
    }
    log("Notifying again...");
    cv.notify_all(); // We don't need the mutex here.
}

int main()
{
    std::array<std::thread, 4> threads = {
        std::thread { waits },
        std::thread { waits },
        std::thread { waits },
        std::thread { signals }
    };
    for (auto& t : threads)
        t.join();
}
```

Zmienne warunkowe dostarczane są przez klasę `std::condition_variable`. Poniżej przedstawiamy schemat korzystania ze zmiennej warunkowej `cv` zakładając, że wątek powinien czekać, dopóki nie będzie spełniony odpowiedni warunek (chroniony muteksem `mut`).

#### Czekanie:

1. Zajmij muteks mut zamkiem `std::unique_lock` (np. przez `std::unique_lock<std::mutex> lock { mut };`)
2. Jeśli warunek nie jest spełniony, wywołaj `cv.wait(lock)`. Metoda `wait()` atomowo zwalnia muteks, usypia wątek i dołącza go do listy wątków oczekujących na warunek `cond`.
3. Ponieważ wątek może zostać obudzony przypadkowo ([spurious wakeup](https://en.wikipedia.org/wiki/Spurious_wakeup)), po obudzeniu sprawdź warunek i, jeśli ciągle nie zachodzi, uśpij wątek przez `cv.wait()`.
4. Po obudzeniu wątek jest właścicielem muteksu `mut`.

Kluczowe jest, aby założyć zamek `1.` zanim sprawdzimy warunek `2.`. Punkty `2-3` można wygodnie wyrazić przy użyciu lambdy: `cv.wait(lock, []{return warunek});`.

#### Budzenie:

1. Zajmij muteks `mut`.
2. Zmień stan systemu tak, by `cond` było spełnione.
3. Zwolnij muteks `mut`.
4. Powiadom wątki oczekujące na zmiennej warunkowej przez `cv.notify_one()` lub `cv.notify_all()`.

#### Ćwiczenia:

- sprawdź, co się stanie, gdy warunek spełniony jest od razu (czyli ustaw początkową wartość zmiennej `i` na `1`).
- sprawdź, co się stanie, gdy `notify` wywoływane jest przez wątek, który zajmuje muteks.
- sprawdź, który wątek zostanie obudzony w następującym scenariuszu: wątek 1 wywołuje `waits()` i usypia; wątek 2 wywołuje `notify()` (ale ciągle ma muteks); wątek 3 wywołuje `waits()`; wątek 2 zwalnia muteks.

## Zmienne lokalne dla wątku (thread local)

Zmienna zadeklarowana jako `thread_local` nie będzie współdzielona między wątkami: każdy wątek będzie miał własną kopię takiej zmiennej. Zatem wzajemne wykluczanie w dostępie do zmiennych `thread_local` jest niepotrzebne. Zmienne takie używane są jako subsytut zmiennych globalnych w programach wielowątkowych - np. do zmiennej errno, przechowującej wartość ostatniego błędu.

Fragment przykładu [thread-local.cpp](thread-local.cpp):

```cpp
thread_local int counter = 0;

void f()
{
    log("f() starts");
    for (int i = 0; i < 1'000'000; i++) {
        // Look, Ma! No mutex!
        int local = counter;
        local += 1;
        counter = local;
    }
    log("f() completes: counter=", counter);
}

Jako thread_local można również zadeklarować zmienne statyczne wewnątrz funkcji.

thread-local-static.cpp, fragment

int g()
{
    thread_local static int count = 0;
    count += 1;
    return count;
}

void f()
{
    int local = 0;
    log("f() starts");
    for (int i = 0; i < 1'000'000; i++) {
        local = g();
    }
    log("f() completes: local=", local);
}
```

## Zadanie punktowane: bariera

Bariera jest mechanizmem synchronizacyjnym często używanym w *High Performance Computing (HPC)*. Barierę inicjuje się dodatnią odpornością (`int resistance`). Następnie wątki wywołują metodę `reach()` bariery. (`resistance - 1`) wątków jest blokowanych. Kolejny wątek przełamuje barierę, odblokowując wszystkie czekające wątki. Po przełamaniu kolejne wywołania `reach()` są nieblokujące.

Wykorzystując zmienne warunkowe i zamki, zaimplementuj klasę `Barrier` realizującą barierę.

## Bibliografia

- [https://isocpp.org/wiki/faq/cpp11-library-concurrency](https://isocpp.org/wiki/faq/cpp11-library-concurrency)
- [http://en.cppreference.com/w/cpp/thread/mutex](http://en.cppreference.com/w/cpp/thread/mutex)
- [http://en.cppreference.com/w/cpp/thread/unique_lock](http://en.cppreference.com/w/cpp/thread/unique_lock)
- [http://en.cppreference.com/w/cpp/thread/condition_variable](http://en.cppreference.com/w/cpp/thread/condition_variable)
- [http://en.cppreference.com/w/cpp/language/storage_duration](http://en.cppreference.com/w/cpp/language/storage_duration)

Data: 2017/11/20

Autor: Krzysztof Rządca

Created: 2017-11-20 Mon 05:49

[Emacs](http://www.gnu.org/software/emacs/) 25.1.1 ([Org](http://orgmode.org/) mode 8.2.10)

[Validate](http://validator.w3.org/check?uri=referer)

