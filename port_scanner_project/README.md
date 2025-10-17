\
     Port Scanner (macOS M1/M2 ready)

    Gereksinimler
    - Homebrew (https://brew.sh)
    - boost (Homebrew ile: `brew install boost`)
    - cmake

    Kurulum (terminal)
    bash
    brew update
    brew install boost cmake
    mkdir build && cd build
    cmake .. -DCMAKE_PREFIX_PATH=/opt/homebrew
    cmake --build . --config Release
    

    Çalıştırma
    bash
    ./scanner 127.0.0.1 22,80,443 50 400
    veya
    ./scanner example.com 1-1024 200 300
    

    Not: Araç yalnızca eğitim/test/izinli hedeflerde kullanılmalıdır.
