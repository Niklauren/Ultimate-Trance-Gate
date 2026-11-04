#pragma once

#include <juce_core/juce_core.h>

// Tooltip texts (Russian). Shown after hovering a control for 2 seconds.
// The source file is UTF-8 (MSVC builds with /utf-8).
namespace tg::tips
{
inline juce::String ru (const char* utf8) { return juce::String::fromUTF8 (utf8); }

// ---- header -----------------------------------------------------------------
inline juce::String presetButton() { return ru ("PRESETS — открыть библиотеку ритмов: Standard A–H, жанры, Creative.\n"
                                                "Там же поиск и избранное (★). Пресет загружается в текущую вариацию A/B/C.\n"
                                                "Звёздочка * после названия — ты изменил ритм после загрузки."); }
inline juce::String prevPreset()   { return ru ("Предыдущий пресет в этой категории."); }
inline juce::String nextPreset()   { return ru ("Следующий пресет в этой категории."); }
inline juce::String variation (char v) { return ru ("Вариация ") + juce::String::charToString ((juce::juce_wchar) v)
                                         + ru (" — отдельная копия всей настройки: ритм и все ручки.\n"
                                               "Сделай на A, B и C разные варианты и переключай, чтобы сравнить на слух.\n"
                                               "Переключение можно автоматизировать в Ableton (параметр Variation)."); }
inline juce::String search()       { return ru ("Поиск по всем пресетам: по названию и по жанру (например rage, triplet, stutter)."); }
inline juce::String star()         { return ru ("Добавить в избранное / убрать. Избранное — первая категория в списке, общая для всех проектов."); }

// ---- pattern row ---------------------------------------------------------------
inline juce::String undo()   { return ru ("Отменить последнее действие (Cmd+Z): правку шагов, пресет, ручку, режим, вариацию."); }
inline juce::String redo()   { return ru ("Вернуть отменённое действие (Cmd+Shift+Z)."); }
inline juce::String steps()  { return ru ("STEPS — длина паттерна, от 1 до 32 шагов. 16 шагов по 1/16 = один такт.\n"
                                          "Нечётная длина (5, 7) даёт полиметрию: ритм сдвигается каждый такт."); }
inline juce::String rate()   { return ru ("RATE — длина одного шага. T — триоль, D — с точкой.\n"
                                          "1/16 — классика трансгейта, 1/32 и 1/64 — статтеры."); }
inline juce::String hits()   { return ru ("EUCLID HITS — сколько ударов разложить равномерно по шагам (евклидов ритм).\n"
                                          "Паттерн меняется сразу. 3 из 8 — тресилло, 5 из 16 — босса."); }
inline juce::String rot()    { return ru ("EUCLID ROT — сдвигает евклидов ритм на N шагов."); }
inline juce::String random() { return ru ("RANDOM — сгенерировать случайный ритм (Sparse / Medium / Dense / Rage)\n"
                                          "или изменить текущий (Mutate 25%, случайные акценты)."); }
inline juce::String shiftL() { return ru ("Сдвинуть весь паттерн на шаг влево (по кругу)."); }
inline juce::String shiftR() { return ru ("Сдвинуть весь паттерн на шаг вправо (по кругу)."); }
inline juce::String rev()    { return ru ("REV — перевернуть паттерн задом наперёд."); }
inline juce::String inv()    { return ru ("INV — инвертировать: включённые шаги выключаются, выключенные включаются."); }
inline juce::String dup()    { return ru ("x2 — скопировать паттерн во вторую половину и удвоить длину (до 32 шагов)."); }
inline juce::String half()   { return ru ("/2 — оставить только первую половину паттерна."); }
inline juce::String fill()   { return ru ("FILL — включить все шаги на 100%."); }
inline juce::String clr()    { return ru ("CLR — очистить паттерн (отменяется кнопкой Undo)."); }
inline juce::String midi()   { return ru ("MIDI — потяни мышкой и брось на дорожку Ableton:\n"
                                          "получится MIDI-клип с этим ритмом (нота C3, громкость = сила удара).\n"
                                          "Удобно, чтобы хэты, бас или чопы играли в том же ритме."); }

// ---- grid ----------------------------------------------------------------------
inline juce::String levelLane()   { return ru ("LEVEL — шаги паттерна. Клик включает или выключает шаг, высота клика = громкость.\n"
                                               "Протяни по шагам, чтобы рисовать или стирать. Тяни вверх-вниз по шагу — громкость.\n"
                                               "Двойной клик — 100%. Правый клик — tie (легато).\n"
                                               "Оранжевая линия — громкость гейта, волна под шагами — твой звук."); }
inline juce::String ratchetLane() { return ru ("RATCHET — сколько быстрых ударов внутри шага: клик 1 → x2 → x3 → x4.\n"
                                               "Правый клик — сброс. Для роллов и статтеров."); }
inline juce::String tieLane()     { return ru ("TIE — склеить шаг со следующим: гейт не закрывается,\n"
                                               "и следующий шаг не бьёт заново. Получаются длинные ноты (легато)."); }
inline juce::String signal()      { return ru ("SIGNAL — показать звук под шагами в реальном времени.\n"
                                               "Серая волна (INPUT) — что пришло в плагин, голубая (OUTPUT) — что вышло.\n"
                                               "Видимая часть серой волны — то, что гейт срезал."); }

// ---- mode + knobs ----------------------------------------------------------------
inline juce::String modeVol() { return ru ("VOL — классический трансгейт: гейт меняет громкость."); }
inline juce::String modeLp()  { return ru ("LPF — гейт управляет фильтром низких частот:\n"
                                           "на закрытых шагах звук становится тёмным и глухим, а не тихим.\n"
                                           "Клик по активной кнопке LPF открывает и закрывает панель фильтра."); }
inline juce::String modeHp()  { return ru ("HPF — гейт управляет фильтром высоких частот:\n"
                                           "на закрытых шагах из звука уходит низ, он становится тонким.\n"
                                           "Клик по активной кнопке HPF открывает и закрывает панель фильтра."); }
inline juce::String cutoff()  { return ru ("CUTOFF — частота фильтра на ЗАКРЫТЫХ шагах. На открытых шагах фильтр полностью открыт.\n"
                                           "LPF: чем ниже, тем глуше паузы. HPF: чем выше, тем тоньше паузы."); }
inline juce::String reso()    { return ru ("RESO — резонанс фильтра: подчёркивает частоту среза, звук становится «кислотнее»."); }
inline juce::String closeFilter() { return ru ("Свернуть панель фильтра. Фильтр продолжает работать.\n"
                                               "Открыть снова — клик по активной кнопке LPF / HPF."); }

inline juce::String attack()  { return ru ("ATTACK — как быстро открывается гейт на каждом шаге.\n"
                                           "Мало (0.1–2 мс) — резкий удар. Много — плавное нарастание.\n"
                                           "Если слышны щелчки, подними до 2–5 мс."); }
inline juce::String decay()   { return ru ("DECAY — за сколько звук спадает после пика до уровня Sustain.\n"
                                           "Работает, только если Sustain ниже 100%."); }
inline juce::String sustain() { return ru ("SUSTAIN — уровень, на котором держится шаг после Decay.\n"
                                           "100% — шаг звучит ровно на всю длину. Ниже — каждый удар становится акцентом, как стаб."); }
inline juce::String release() { return ru ("RELEASE — как долго звук затухает после закрытия гейта.\n"
                                           "Мало — резкая отсечка. Много — хвост тянется в паузу."); }
inline juce::String gate()    { return ru ("GATE — какую часть шага гейт остаётся открытым.\n"
                                           "25% — короткие стаккато, 75% — классический трансгейт, 100% — шаги почти сливаются."); }
inline juce::String swing()   { return ru ("SWING — сдвигает каждый второй шаг позже, даёт шаффл. 20–40% — хаус и гараж."); }
inline juce::String depth()   { return ru ("DEPTH — насколько глубоко гейт режет в паузах. 100% — до конца, 50% — наполовину."); }
inline juce::String stereo()  { return ru ("STEREO — правый канал играет тот же паттерн с отставанием на N шагов (пинг-понг).\n"
                                           "Off — оба канала одинаковые. На моно-дорожке не работает."); }
inline juce::String mix()     { return ru ("MIX — смесь обработанного и исходного звука. 0% — гейта нет, 100% — только гейт.\n"
                                           "Посмотри на волну под шагами: видно, сколько звука проходит в паузах."); }
} // namespace tg::tips
