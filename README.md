# lab06-system — Просмотр событий журнала Application (вариант 18)

Лабораторная работа 6 по дисциплине «Операционные среды и системное программирование» (БГУИР).
Раздел Д, вариант 18: просмотр событий журнала Application средствами Windows Event Log API.

- Номер варианта: 18 (раздел Д).
- Назначение: утилита evtview.exe открывает журнал Application запросом EvtQuery в обратном
  порядке, читает события пакетами через EvtNext, извлекает дату и время (с переводом в локальное),
  источник, идентификатор, уровень и краткое сообщение через EvtRender и EvtFormatMessage
  и выводит не более --limit записей в формате text, csv или json.
- Используемые Windows API: EvtQuery, EvtNext, EvtRender, EvtOpenPublisherMetadata,
  EvtFormatMessage, EvtClose, GetLastError, FormatMessageW, SystemTimeToFileTime,
  FileTimeToLocalFileTime, FileTimeToSystemTime, QueryPerformanceCounter.
- Окружение: Windows 10/11 x64, MSVC 2022 (C++17), Python 3.10+ для тестов и графиков.
- STUDENT_ID = 71, SEED = 7887. Чтение только из доступных текущему пользователю журналов;
  запись в журналы и изменение системных параметров не выполняются.

## Сборка

```
scripts\build.bat
```

Результат: `build\evtview.exe`.

## Параметры запуска

```
build\evtview.exe --log <канал> --limit <N> --format <text|csv|json> [--query <XPath>] [--log-file <файл>]
```

- `--log` — имя канала журнала (по умолчанию Application);
- `--limit` — ограничение числа выводимых событий;
- `--format` — формат вывода: text, csv или json;
- `--query` — собственный XPath-запрос вместо выборки последних событий;
- `--log-file` — файл журнала утилиты (при формате csv/json журнал дублируется в stderr).

## Полный цикл прогона

1. Базовый просмотр последних 20 событий журнала Application:

```
build\evtview.exe --log Application --limit 20
```

2. Просмотр в машинных форматах (для скриптов и проверок):

```
build\evtview.exe --log Application --limit 50 --format csv
build\evtview.exe --log Application --limit 50 --format json
```

3. Проверка обработки ошибок: пустая выборка, некорректный запрос, отсутствующий канал,
   отрицательный лимит:

```
build\evtview.exe --limit 10 --query "*[System[EventID=4294967295]]"
build\evtview.exe --limit 10 --query "not-a-xpath[["
build\evtview.exe --limit 10 --log NoSuchChannelXYZ
build\evtview.exe --limit -3
```

4. Прогон с журналом утилиты для контроля дескрипторов (число EVT_CLOSE равно числу
   выведенных событий плюс один набор результатов):

```
build\evtview.exe --log Application --limit 100 --log-file data\evt100.log
```

5. Завершение работы штатное: утилита одноразовая, закрывает набор результатов и каждый
   дескриптор события через RAII-обёртку EvtGuard и возвращает код 0.

## Тесты

```
cd tests
python smoke_test.py
```

14 проверок: лимиты и форматы, пустая выборка, некорректный запрос, отсутствующий канал,
отрицательный лимит, журнал и освобождение дескрипторов, повторяемость прогонов.

## Эксперименты

```
python scripts\run_experiments.py
```

Конфигурации: A — последние 20 событий, B — 50, C — 100; по 3 прогона каждой.
Результаты: `data/experiments.csv` (время, число событий, ошибки, распределение по уровням)
и журналы прогонов в `data/`.

Графики и диаграммы (чёрно-белые):

```
python scripts\plot_time.py
python scripts\plot_levels.py
python scripts\make_diagrams.py
python scripts\sync_listings.py
```

PNG сохраняются в `report/img/`, синхронные копии листингов — в `report/listings/`.
Редактируемые исходники диаграмм: `diagrams/flow.puml`, `diagrams/architecture.drawio`.

## Отчёт

```
typst compile report\main.typ report\main.pdf
python <путь к bsuir_typst>\linter\linter.py report\main.typ   (предварительно set "PYTHONUTF8=1")
```
