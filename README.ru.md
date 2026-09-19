# Power Timer (русская версия)

[![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-3.24%2B-064F8C?logo=cmake&logoColor=white)](https://cmake.org/)
[![FLTK](https://img.shields.io/badge/GUI-FLTK%201.4.5-4B9CD3)](https://www.fltk.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![CI](https://img.shields.io/badge/CI-GitHub%20Actions-2088FF?logo=githubactions&logoColor=white)](.github/workflows/build.yml)

Power Timer — небольшое приложение для запуска сна или выключения компьютера через заданный интервал. Одно окно, без фоновой службы, сети и сохранённых настроек.

Релиз **0.0.1** · C++17 · CMake 3.24+ · FLTK 1.4.5 · Windows, macOS, Linux

Основная документация: [README.md](README.md)

![Главное окно Power Timer](docs/power-timer-main-window.png)

## Возможности

- Сон или выключение через 1 минуту — 24 часа.
- Быстрый выбор: 15 минут, 30 минут, 1 час и 2 часа.
- Обратный отсчёт, ожидаемое местное время и отмена кнопкой или Escape.
- По умолчанию интерфейс на английском; для русского используйте `--lang ru`.
- `--dry-run` и `--smoke-test` для безопасной проверки без изменения питания.

## Ограничения таймера

Отсчёт работает только внутри запущенного приложения. Закрытие, выход из сеанса, перезагрузка или сбой отменяют таймер; системное задание не создаётся. Компьютер должен оставаться бодрствующим. Приложение не запрещает сон и не будит компьютер.

При задержке проверки более 10 секунд или заметном расхождении системных и монотонных часов таймер отменяется. Сон до 10 секунд может не распознаться. После нулевого значения отмена недоступна: отправляется один запрос без повторов. Принятый системой запрос не означает, что сон или выключение уже завершены. Сохраните документы заранее.

На macOS App Nap отключается только во время отсчёта. На Linux нужны systemd 248+, systemd-logind, `busctl` и `systemctl`; запускайте приложение обычным пользователем, не через `sudo`.

## Сборка

При первой настройке скачиваются закреплённые исходники FLTK 1.4.5 с проверкой SHA-256. Нужен компилятор C++17.

### Windows

Visual Studio 2022+ с компонентами разработки классических приложений на C++:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
.\build\Release\PowerTimer.exe
```

### macOS

С установленными Xcode Command Line Tools:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/PowerTimer
```

### Linux

Debian/Ubuntu, стандартный X11/XWayland:

```sh
sudo apt install cmake g++ make libx11-dev libxext-dev libxft-dev libxinerama-dev libxcursor-dev libxfixes-dev libxrender-dev libfontconfig1-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/power-timer
```

## Безопасная проверка и документы

```sh
./build/power-timer --dry-run --smoke-test
```

В Windows используйте `.\build\Release\PowerTimer.exe --dry-run --smoke-test`, в macOS — `./build/PowerTimer --dry-run --smoke-test`. Тесты логики можно собрать без FLTK с `-DPOWER_TIMER_BUILD_APP=OFF`.

Подробности проверок: [docs/VALIDATION.md](docs/VALIDATION.md). Чек-лист выпуска: [docs/RELEASING.md](docs/RELEASING.md).

## Выпуск

Запустите вручную **Actions → Build and release → Run workflow**. Укажите версию из `CMakeLists.txt` и `CHANGELOG.md`. При `create_release=false` Actions публикует три отдельных исполняемых файла. При `true` создаётся черновик GitHub Release с теми же бинарниками для коммита workflow; тег `v<версия>` создаётся только для этого черновика.

Для macOS и Linux после скачивания выполните `chmod +x`. Команда `--license` показывает встроенные уведомления о лицензии приложения и сторонних компонентах.

## Лицензия

Power Timer распространяется по [лицензии MIT](LICENSE). Уведомления о FLTK: [THIRD_PARTY.md](THIRD_PARTY.md).
