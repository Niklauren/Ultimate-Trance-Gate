# Ultimate Trance Gate

**created by mikita_laurenau** · [Instagram](https://www.instagram.com/mikita_laurenau/)

Трансгейт для рейджа, евроденса, транса и EDM: 241 ритм, вариации A/B/C, гейт громкости или фильтра, живая волна сигнала и перетаскивание ритма в DAW как MIDI.
VST3 + AU для macOS (Apple Silicon и Intel), VST3 для Windows.

![Ultimate Trance Gate](docs/ui-main.png)

## Скачать

Готовые сборки лежат на странице **[Releases](../../releases)**:
- **Mac** (Apple Silicon + Intel, VST3 + AU): `UltimateTranceGate-<версия>-Mac-Installer.pkg` — двойной клик → Продолжить → Установить. Если macOS не даёт открыть: правый клик → Открыть (на macOS 15+: Настройки → Конфиденциальность и безопасность → «Всё равно открыть»).
- **Windows** (VST3): `UltimateTranceGate-<версия>-Windows-Setup.exe` — Далее → Установить. Если SmartScreen предупреждает: «Подробнее» → «Выполнить в любом случае».
- Zip-архивы — тот же плагин для ручной установки (`install_mac.command` / копирование в `C:\Program Files\Common Files\VST3`).

Потом пересканируй плагины в DAW. Полное руководство — [docs/MANUAL.md](docs/MANUAL.md).

## Что умеет

- **PRESETS** — 241 ритм: **Standard A–H**, жанры (Trance, Psytrance, Eurodance, Hands Up, EDM / Big Room, Future Bass, Hardstyle, Rage, Trap / Drill / Plugg, House / Garage, Jersey Club, Techno, Breaks / DnB, Euclidean / Poly, Stutter / FX) и **Creative** (100 экспериментальных). У каждого пресета видны столбики ритма. Есть **поиск** и **★ избранное**. Каждый пресет приносит свою огибающую (CHARACTER + ADSR + GATE) под стиль. Полный список — [docs/PRESETS.md](docs/PRESETS.md).
- **VAR A / B / C** — три независимые вариации: у каждой свой ритм и свои ручки. Переключение можно автоматизировать.
- **Classic Gate** — стартовый ритм, как у Kilohearts Trance Gate (16 шагов по 1/32).
- **Шаги** — до 32, сетка от 1/4 до 1/64 (триоли, точки); на каждом шаге громкость, **ratchet** (2–4 удара) и **tie** (легато).
- **Режимы гейта** — **VOL** (громкость), **LPF** и **HPF** (гейт открывает и закрывает фильтр; CUTOFF и RESO во всплывающей панели).
- **Сигнал** — под шагами в реальном времени видно входящий звук (серый) и то, что выходит (голубой).
- **MIDI** — перетащи кнопку MIDI на дорожку: получится MIDI-клип с этим ритмом.
- **Undo / Redo**, генераторы **EUCLID** и **RANDOM**, инструменты сдвига, реверса, инверсии, удвоения.
- **CHARACTER** — характер гейта: **Smooth** (плавно, без щелчков, по умолчанию), **Hard** (резко, под rage), **Soft** (мягко, как аналог), **Pump** (сайдчейн-засасывание).
- **Огибающая** Attack / Decay / Sustain / Release / Gate, **Swing**, **Depth**, **Stereo** (пинг-понг), **Mix**.
- **MIDI C1–G1** загружают Standard A–H в текущую вариацию.
- **Подсказки** — наведи мышь на любой элемент на 2 секунды.

| Меню пресетов | Режим фильтра |
|---|---|
| ![](docs/ui-presets.png) | ![](docs/ui-filter.png) |

## Сборка из исходников

Каждый push в `main` собирает плагин в GitHub Actions (Mac + Windows), а тег `v*` публикует релиз.

Локально на Mac: `bash build_mac.sh` (поставит cmake, скачает JUCE, соберёт и установит).

Офлайн-тесты (Linux/Mac):
```
cmake -B build -DTG_BUILD_TESTS=ON && cmake --build build
./build/UTGRenderTest_artefacts/Release/UTGRenderTest --screenshot
```

## Устройство кода

- `Source/GateEngine.h` — DSP гейта (позиция шага, свинг, ратчеты, ADSR).
- `Source/PluginProcessor.*` — вариации, режимы фильтра, undo/redo, запись сигнала, сохранение.
- `Source/Pattern.h`, `Source/Presets.h` — данные паттернов и библиотека ритмов.
- `Source/PluginEditor.*`, `StepGrid.*`, `PresetBrowser.h`, `UiParts.h`, `Theme.h` — интерфейс.
- `Source/Tooltips.h` — тексты подсказок. `Source/Settings.h` — избранное и экспорт MIDI.
- `design/` — макет интерфейса.

Свои ритмы добавляются строкой в `Source/Presets.h`:
```
add (CR, "My Rhythm", "X-.X -.X. X-.X 2.4.");   // X=100% x=70% o=40% ,=20% .=пауза -=легато 2/3/4=ratchet
```

Построено на JUCE 8 (AGPLv3 / коммерческая лицензия). Шрифт Figtree — SIL Open Font License.
