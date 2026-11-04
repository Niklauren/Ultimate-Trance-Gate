#pragma once

#include "Pattern.h"
#include <vector>

namespace tg
{
struct PatternPreset
{
    juce::String category, name;
    PatternData data;
};

//==============================================================================
// Pattern syntax (spaces are ignored, they only group beats):
//   X = 100%   x = 70%   o = 40%   , = 20%   . = off
//   - = tie (legato from the previous step)
//   2 / 3 / 4 = ratchet (2, 3 or 4 retriggers inside the step)
// Default grid is 1/16. Shuffled styles (garage, 2-step, jersey triplets) are
// written on a 1/16T grid: 6 slots per beat, a swung 16th sits on slot 2.
//==============================================================================
inline const std::vector<PatternPreset>& presetLibrary()
{
    static const std::vector<PatternPreset> lib = []
    {
        std::vector<PatternPreset> v;
        auto add = [&v] (const char* cat, const char* name, const char* pattern, int res = R16)
        {
            v.push_back ({ cat, name, PatternData::fromString (pattern, res) });
        };

        //=================================================================== STANDARD (A-H)
        // The original eight starting rhythms. MIDI notes C1..G1 load them into the current variation.
        const char* SD = "Standard";
        add (SD, "Sunrise Gate",      "X.XX .XX. X.XX .X.X");
        add (SD, "Rhythm Stab 3-3-2", "X..X ..X. X..X ..X.");
        add (SD, "Rage Chop",         "X-.X -.X. X-.X 2.4.");
        add (SD, "Gallop Gate",       "X.XX X.XX X.XX X.XX");
        add (SD, "KBBB Roll",         ".XXX .XXX .XXX .XXX");
        add (SD, "Supersaw Stutter",  "4.2. X.3. 4.2. X.33");
        add (SD, "3-3-2 Chords",      "X--X --X- X--X --X-");
        add (SD, "Build-Up 32",       "X.X. X.X. X.X. X.X.  XXXX XXXX 2222 4444");

        //=================================================================== TRANCE
        // 128-140 BPM. Gated supersaw pads/chords, 1/16 grid, offbeat drive.
        const char* T = "Trance";
        add (T, "Pure Sixteenths",     "XXXX XXXX XXXX XXXX");
        add (T, "Offbeat Pulse",       "..X. ..X. ..X. ..X.");
        add (T, "Gallop Gate",         "X.XX X.XX X.XX X.XX");
        add (T, "Reverse Gallop",      "XX.X XX.X XX.X XX.X");
        add (T, "Sunrise Gate",        "X.XX .XX. X.XX .X.X");
        add (T, "Euphoria 32",         "XX.X X.XX .XX. XX.X  XX.X X.XX .X.X XXXX");
        add (T, "Pumping Pad",         ".xXX .xXX .xXX .xXX");
        add (T, "Chord Bounce",        "X.X. .X.X X.X. .XX.");
        add (T, "Anthem Legato",       "X-.X -.X. X-.X -.X.");
        add (T, "Tech Trance Chop",    "X.X. XX.X .X.X X.XX");
        add (T, "Vocal Trance Gate",   "X-X. X-X. X.X- X.X.");
        add (T, "Breakdown Swell",     "X--- X--- X--- X.X.", R8);
        add (T, "Build-Up 32",         "X.X. X.X. X.X. X.X.  XXXX XXXX 2222 4444");
        add (T, "Peak Time 32",        "X.XX .XX. X.XX .X.X  X.XX .XX. XX.X 2.3.");

        //=================================================================== PSYTRANCE / GOA
        // 138-150 BPM. Kick on the beat, bass fills the 16ths after it (K-B-B-B).
        const char* P = "Psytrance / Goa";
        add (P, "KBBB Roll",           ".XXX .XXX .XXX .XXX");
        add (P, "KBB Skip",            ".XX. .XX. .XX. .XX.");
        add (P, "Off-Bass",            "..X. ..X. ..X. ..XX");
        add (P, "Full-On Accents",     ".Xxo .Xxo .Xxo .XxX");
        add (P, "Prog Psy Pulse",      ".X.X .X.X .X.X .X.X");
        add (P, "Goa Triplet Roll",    ".XX.XX.XX.XX.XX.XX.XX.XX", R16T);
        add (P, "Night Psy 32nds",     "..XX XXXX ..XX XXXX ..XX XXXX ..XX XXXX", R32);
        add (P, "Twilight Shuffle",    "..XX .X.X ..XX .XXX");
        add (P, "Acid Line Gate",      "X.XX X..X .XX. X.X.");
        add (P, "Forest Glitch",       ".X2X .X.3 .XX2 .4.X");

        //=================================================================== EURODANCE
        // 130-145 BPM. Staccato stabs and M1-style pianos, syncopated 3-3-2,
        // offbeat organ basses, pizzicato 16ths, sparse rap verses.
        const char* E = "Eurodance";
        add (E, "90s Offbeat Stab",    "..X. ..X. ..X. ..XX");
        add (E, "Rhythm Stab 3-3-2",   "X..X ..X. X..X ..X.");
        add (E, "Dancer Stab",         "X..X ..X. ..X. X.X.");
        add (E, "Piano Shuffle",       "X..X .X.. X..X .X.X");
        add (E, "Organ Bass Bounce",   ".X.X .X.X .XX. .X.X");
        add (E, "Pizzicato Pulse",     "XxXx XxXx XxXx XxXX");
        add (E, "Dancefloor Eighths",  "X.x. X.x. X.x. X.XX");
        add (E, "Synth Strings Chop",  "X-.X -.X- .X-. X-.X");
        add (E, "Rap Verse Stab",      "X... ..X. ..X. ....");
        add (E, "Rave Stab",           "X.X. X.XX .X.X X.X.");
        add (E, "Euro Lead Hook",      "X.XX .X.X X.X. XX.X");
        add (E, "Chorus Lift 32",      "X..X ..X. X..X ..X.  X..X ..X. X.X. XXXX");
        add (E, "Key Change Fill 32",  "X..X ..X. X..X ..X.  X.X. X.X. 2222 3344");

        //=================================================================== HANDS UP / DANCE 2000s
        // ~140 BPM. Supersaw leads on fast 16ths, sidechained basses, jumpstyle bounce.
        const char* H = "Hands Up / Dance 2000s";
        add (H, "Hands Up Lead",       "X.XX X.X. XX.X X.X.");
        add (H, "Supersaw Drive",      "XxXX XxXX XxXX XXXX");
        add (H, "Waterfall Gate",      "XX.X X.XX .X.X XX.X");
        add (H, "Sidechain Bass",      ".xX. .xX. .xX. .xXX");
        add (H, "Jumpstyle Bounce",    ".X.X .X.X .X.X .XXX");
        add (H, "Clubland Stab",       "X..X X..X X.X. X.X.");
        add (H, "Nightcore Rush",      "X.XX X.XX X.XX 2222");
        add (H, "Hands In The Air 32", "X.XX X.X. XX.X X.X.  X.XX X.X. 2.2. 4444");

        //=================================================================== EDM / BIG ROOM
        // 126-130 BPM. Offbeat donk basses (Melbourne bounce), sparse festival stabs,
        // sidechain pumping, snare-roll style fills.
        const char* M = "EDM / Big Room";
        add (M, "Bounce Offbeat",      ".X.X.X.X", R8);
        add (M, "Big Room Drop",       "X... X.X. ..X. X...");
        add (M, "Festival Stab",       "X.X. ..X. X.X. ..XX");
        add (M, "Melbourne Horn",      "X..X ..X. X..X .XX.");
        add (M, "Electro Pump",        ",oxX ,oxX ,oxX ,oxX");
        add (M, "Future House Bass",   "X..X .X.. X..X .X.X");
        add (M, "Prog House Chop",     ".XXX X.XX .XXX X.X.");
        add (M, "Tropical Pluck",      "X..X ..X. ..X. X...");
        add (M, "Drop Roll 32",        "X... X... X... X...  XXXX XXXX 2222 3344");
        add (M, "Snare Roll Riser 32", "X... X... X.X. X.X.  XXXX XXXX 2222 4444");

        //=================================================================== FUTURE BASS
        // 140-160 BPM, half-time drums. Chopped / swelling supersaw chords.
        const char* F = "Future Bass";
        add (F, "3-3-2 Chords",        "X--X --X- X--X --X-");
        add (F, "Supersaw Chop",       "X-.X -.X- X-.X -.X.");
        add (F, "Stutter Chords",      "X.X. X.X. 3.3. 4444");
        add (F, "Swell Gate",          ",oxX ,oxX ,oxX XXXX");
        add (F, "Ducked Chords",       ".oX- .oX- .oX- .oX-");
        add (F, "Half-Time Triplets",  "X--X--X--X--XX.XX.XXXXXX", R16T);
        add (F, "Chord Roll Fill",     "X--- X--- X.X. 2344");
        add (F, "Wobble Triplets",     "X.XX.XX.XX.4", R8T);

        //=================================================================== HARDSTYLE
        // 150-160 BPM. Reverse bass swells between kicks, offbeat stabs, raw screeches.
        const char* HS = "Hardstyle";
        add (HS, "Reverse Bass Swell", ".oxX .oxX .oxX .oxX");
        add (HS, "Euphoric Lead",      "X.XX X.XX X-XX X.X.");
        add (HS, "Offbeat Stabs",      "..X. ..XX ..X. ..XX");
        add (HS, "Rawstyle Screech",   "X.X2 .X.X X.X2 .4X.");
        add (HS, "Anti-Climax",        "X..X .X.. X.X. .XX.");
        add (HS, "Kick Roll Fill 32",  "X... X... X... X...  X.X. X.X. XXXX 4444");

        //=================================================================== RAGE
        // 150-170 BPM. Looping supersaw leads over basic trap drums, stutters & rolls.
        const char* R = "Rage";
        add (R, "Rage Chop",           "X-.X -.X. X-.X 2.4.");
        add (R, "Supersaw Stutter",    "4.2. X.3. 4.2. X.33");
        add (R, "Alien Lead",          "X-X- X.X- X-X. X2X.");
        add (R, "Rage Bounce",         "X..X ..X. 2..X ..X.");
        add (R, "Glitch Ratchet",      "2.3X .4.. 2X.3 .4.X");
        add (R, "Moshpit Rolls",       "X.X. X.X. 3.X. 4.X.");
        add (R, "Half-Time Stab",      "X... .... X..X ....");
        add (R, "Synth Tresillo",      "X-.X -.X. X-.X -.X-");
        add (R, "Dark Pulse",          "X.xX .xX. X.xX .2X.");
        add (R, "Rage Triplets",       "XX.XX.X.XX.4", R8T);
        add (R, "Vamp Lead 32",        "X..X ..X. X..X .X..  X..X ..X. 2.2. 3344");

        //=================================================================== TRAP / DRILL / PLUGG
        // 130-160 BPM. Bouncy plugg chords, sliding 808-style holds, triplet flows.
        const char* TR = "Trap / Drill / Plugg";
        add (TR, "Plugg Bounce",       "X..X ..X. ..X. .X..");
        add (TR, "Drill Slide",        "X-.. X-.X ..X- .X..");
        add (TR, "808 Glide",          "X--- ..X- -.X. X---");
        add (TR, "Hat Roll Gate",      "X.X. X.X. X.X. 2344");
        add (TR, "Triplet Flow",       "X.XX.XX.X.XXX.XX.XX.X.XX", R16T);
        add (TR, "Hyperpop Glitch 32", "X.X.2..X XX.3.X.. X.2.X.4. XX..3.X.", R32);

        //=================================================================== HOUSE / GARAGE
        // 120-138 BPM. Offbeat chord stabs, piano house, swung 2-step (1/16T grid).
        const char* G = "House / Garage";
        add (G, "House Offbeat Chords","..X. ..X. ..X. .XX.");
        add (G, "Piano House",         "X..X ..X. .X.. X.X.");
        add (G, "Disco Chop",          "X.X. XX.X X.X. XX.X");
        add (G, "Speed Garage Bass",   ".X.X ..X. .X.X X.X.");
        add (G, "2-Step Stabs",        "...X.X ...X.. ..XX.. ...X..", R16T);
        add (G, "Garage Shuffle",      "...X.X ...X.X ...X.X ..XX.X", R16T);
        add (G, "Swung Organ",         "X.X..X ..X.X. X.X..X ..XX.X", R16T);

        //=================================================================== JERSEY CLUB
        // 130-145 BPM. The five-kick club pattern (steps 1, 5, 9, 12, 15) + triplet fills.
        const char* J = "Jersey Club";
        add (J, "Jersey Kick",         "X... X... X..X ..X.");
        add (J, "Jersey Bounce",       "X... X... X..X ..2.");
        add (J, "Jersey Chop",         "XX.. XX.. XX.X X.XX");
        add (J, "Triplet Squeaks",     "XXX...XXX...XXX...XXXXXX", R16T);
        add (J, "Baltimore Break",     "X... X..X X.X. ..X.");
        add (J, "Jersey Fill 32",      "X... X... X..X ..X.  X... X... 2..2 ..4.");

        //=================================================================== TECHNO
        // 125-150 BPM. Rumbles, off-chords, acid accents, industrial chops.
        const char* TE = "Techno";
        add (TE, "Off-Chord",          ".X.. .X.. .X.. .X..");
        add (TE, "Rumble",             ".xXx .xXx .xXx .xXx");
        add (TE, "Acid Accents",       "XXxX xXXx XxXX xXxX");
        add (TE, "Industrial Chop",    "X.X2 X.X. X.X2 X.4.");
        add (TE, "Minimal Stab",       "..X. .... X... .X..");
        add (TE, "Hard Techno Drive",  ".XXX .XX2 .XXX .X4X");

        //=================================================================== BREAKS / DNB
        // 160-175 BPM. Break-style gates on the kick/snare skeleton.
        const char* B = "Breaks / DnB";
        add (B, "Amen Gate",           "X.X. ..X. X.XX ..X.");
        add (B, "Two-Step DnB",        "X... X... ..X. X...");
        add (B, "Breakbeat",           "X..X ..X. ..XX .X..");
        add (B, "Neuro Stutter",       "X.3. ..X2 X..4 .X2.");
        add (B, "Jungle Chop 32",      "X.X. XX.X ..X. X.XX  X.X. X..X .XX. 2.4.");

        //=================================================================== EUCLIDEAN / POLY
        struct Eu { int k, n; const char* name; };
        for (auto e : { Eu { 3, 8,  "Euclid 3/8 Tresillo" },  Eu { 5, 8,  "Euclid 5/8 Cinquillo" },
                        Eu { 4, 9,  "Euclid 4/9 Aksak" },     Eu { 5, 12, "Euclid 5/12" },
                        Eu { 7, 12, "Euclid 7/12 Bell" },     Eu { 5, 16, "Euclid 5/16 Bossa" },
                        Eu { 7, 16, "Euclid 7/16 Samba" },    Eu { 9, 16, "Euclid 9/16" },
                        Eu { 11, 16, "Euclid 11/16" },        Eu { 13, 32, "Euclid 13/32" } })
        {
            PatternData p;
            p.length = e.n;
            p.resolution = R16;
            p.euclid (e.k, 0);
            v.push_back ({ "Euclidean / Poly", e.name, p });
        }
        const char* PO = "Euclidean / Poly";
        add (PO, "Poly 3",             "X..");
        add (PO, "Poly 5",             "X.X..");
        add (PO, "Poly 7",             "X.XX.X.");
        add (PO, "Dotted Eighths",     "XXXX", R8D);
        add (PO, "Triplet Eighths",    "XXXXXXXXXXXX", R8T);

        //=================================================================== STUTTER / FX
        const char* S = "Stutter / FX";
        add (S, "Stutter 32nds",       "XXXX XXXX XXXX XXXX", R32);
        add (S, "Buzz 64ths",          "X.X. X.X. X.X. X.X.", R64);
        add (S, "Machine Gun",         "4444 4444 4444 4444");
        add (S, "Ratchet Ladder",      "X.2. 3.4. X.2. 3.4.");
        add (S, "Tape Stop",           "4433 3322 22XX X-X-");
        add (S, "Morse",               "X.X. X--. X-.X X.X-");
        add (S, "Bar End Fill",        "X... X... X... 4444");

        //=================================================================== CREATIVE
        // Experimental gates: odd lengths, swells, glitches, legato shapes, concept rhythms.
        const char* CR = "Creative";
        add (CR, "Seven Up",              "X.X.XX.");
        add (CR, "Five Alive",            "X.Xx.");
        add (CR, "Nine Lives",            "X.X..XX.x");
        add (CR, "Eleven Steps",          "X.X.X..X.X.");
        add (CR, "Thirteen Ghosts",       "X..x.X.,.X.x.");
        add (CR, "Prime Time",            ".XX.X.X...X.X...");
        add (CR, "Fibonacci",             "XXX.X..X....X...");
        add (CR, "Dotted Drift",          "X.x.X", R16D);
        add (CR, "Fifteen Fold",          "X.XX.X..X.XX.X.");
        add (CR, "Odd Couple",            "X.X.XX.X.X");
        add (CR, "Breathing Pad",         ",oxX Xxo, ,oxX Xxo,");
        add (CR, "Tidal",                 ",,oo xxXX XXxx oo,,");
        add (CR, "Heartbeat",             "Xo.. .... Xo.. ....");
        add (CR, "Echo Fade",             "X.x. o.,. X.x. o.,.");
        add (CR, "Crescendo Roll",        ",.o. x.X. 2.3. 4444");
        add (CR, "Ghost Notes",           "X,,x ,,X, ,x,, X,x,");
        add (CR, "Accent Wave",           "Xoxo xXox oxXo xoxX");
        add (CR, "Rain Drops",            "x.,o ..X. ,..o .x,.");
        add (CR, "Tremolo Bloom",         ",,,, oooo xxxx XXXX", R32);
        add (CR, "Pyramid",               ",oxX Xxo, ,oxX Xxo.", R8);
        add (CR, "Fade Out Line",         "XXXX xxxx oooo ,,,,");
        add (CR, "Fade In Line",          ",,,, oooo xxxx XXXX");
        add (CR, "Wave Train",            ",oxX xo,. ,oxX xo,.");
        add (CR, "Pendulum",              "Xx.o .,.o .x.X");
        add (CR, "Bouncing Ball",         "X... .... X... ..X. .X.X .2.3 .4.4 4444");
        add (CR, "Glitch Cascade",        "4.3. 2.X. 4.3. 2.X.");
        add (CR, "Ratchet Rain",          "2.3. .4.2 ..3. 4..2");
        add (CR, "Digital Hiccup",        "X.X. X2.. X.X. X4..");
        add (CR, "Bit Crush Stutter",     "XXXX .... 4444 ....", R32);
        add (CR, "Machine Talk",          "X.2. X4.3 X.2. X.43");
        add (CR, "Rise And Fall",         "4.3. 2.X. X.2. 3.4.");
        add (CR, "Hyperdrive",            "2222 3333 4444 4444");
        add (CR, "Skipping CD",           "XX.X X.XX .4.. XX.X");
        add (CR, "Laser Burst",           "4... 4... 4..4 4.44");
        add (CR, "Data Corruption",       "X.4. .2X. 3..X .4.2");
        add (CR, "Buffer Underrun",       "XXXX XX.. X... 4...");
        add (CR, "Pixel Rain",            "2.,2 .o2. 2,.2 .x2.", R32);
        add (CR, "Modem Handshake",       "X2X3 X4X2 X3X4 2343", R32);
        add (CR, "Arcade Coin",           "X.X. 4... .... ....");
        add (CR, "Long Exposure",         "X--- ---- .... ....");
        add (CR, "Slow Motion",           "X--- .X-- -.X- .X-.");
        add (CR, "Sustain Pedal",         "X--- --.X ---- -.X.");
        add (CR, "Tape Warble",           "X-.X --.X -.X- -.X-");
        add (CR, "Drone Cuts",            "X--- ---- X--- ---.");
        add (CR, "Choir Breath",          "X--- X--- X--. X-..");
        add (CR, "Legato Morse",          "X--. X.X- -.X. .X-.");
        add (CR, "Glide Pairs",           "X-.X -.X- .X-. X-..");
        add (CR, "Staircase Hold",        "X.X- .X-- .X-- -...");
        add (CR, "Velvet Rope",           "x-X- x-X- x-X- x-X-");
        add (CR, "Swell And Cut",         ",-o- x-X- ---- X...");
        add (CR, "Off Kilter",            ".X.. X.X. .X.. X.X.");
        add (CR, "Clave Inside Out",      "..X. X... X..X ..X.");
        add (CR, "Backwards Bounce",      ".X.. X..X ..X. X..X");
        add (CR, "Lopsided",              "X..X .X.. X..X .X..");
        add (CR, "Stumble",               "X.X. .XX. .X.X ..XX");
        add (CR, "Polka Dot",             "X.X. X..X .X.X ..X.");
        add (CR, "Tumbling",              "XXX. XX.X .X.. X...");
        add (CR, "Push Pull",             "X..X X..X X..X X..X");
        add (CR, "Hiccup Groove",         "X.XX ..X. X.XX .X..");
        add (CR, "Upside Down",           "..XX .X.. ..XX .XX.");
        add (CR, "Triplet Rain",          "X.XXX. X.XX.X X.XXXX .X.XXX", R16T);
        add (CR, "Six Eight",             "X.xx.x X.xx.x", R8T);
        add (CR, "Afro Bell 12",          "X.X.XX .X.X.X", R8T);
        add (CR, "Triplet Ratchets",      "X..2.. X..3.. X..4.. X.X.X.", R16T);
        add (CR, "Lazy Triplets",         "X--X-- X--X-- X--X.X .X.X..", R16T);
        add (CR, "Swung Eighths",         "X..X.X X..X.X X..X.X X..XXX", R16T);
        add (CR, "Triplet Sparkle",       "X.X.XX .X.X.X X.X.XX .X.XXX", R32T);
        add (CR, "Ternary Pulse",         "XxoXxo XxoXxo XxoXxo XxoXxo", R16T);
        add (CR, "Shuffle Ghosts",        "X.,X., X.,X., X.xX., X.xXxX", R16T);
        add (CR, "Waltz Gate",            "X.xX.x X.xX.x", R8);
        add (CR, "Dotted Cascade",        "X.XX", R16D);
        add (CR, "Dotted Pulse",          "XXXX XXXX XXXX XXXX", R8D);
        add (CR, "Dotted Tresillo",       "X.X. X.XX", R8D);
        add (CR, "Quarter Thump",         "X.XX", R4);
        add (CR, "Half Bar Chop",         "X--. X-.X X--. X...", R8);
        add (CR, "Wide Stabs",            "X..X ..X. X..X ..X.", R8);
        add (CR, "Slow Euro",             "X.XX .X.X X.XX .X.X", R8);
        add (CR, "Sixty-Four Spray",      "X.X.XX.XX.X.XXXX X.X.XX.XX.X.XXXX", R64);
        add (CR, "Thirty-Two Twitch",     "X.X.X.XX..X.X.XX X.X.X.XX..XXXX..", R32);
        add (CR, "Granular Dust",         "x.,.o,.x,.o.,x., ,x.o.,x.,o.x,.,", R64);
        add (CR, "Palindrome",            "X.X. .XX. .XX. .X.X");
        add (CR, "Mirror Mirror",         "XX.X .X.. ..X. X.XX");
        add (CR, "Morse SOS",             "X.X.X.. X-.X-.X-.. X.X.X.. .......", R32);
        add (CR, "Heart Monitor",         "X... .... Xx.. ....");
        add (CR, "Clock Tick",            "x... X... x... X...");
        add (CR, "Typewriter",            "XXxX .XxX .XX. xXX4");
        add (CR, "Comet Tail",            "4.3. 2.X- ---- ....");
        add (CR, "Spiral",                "X... X..X .X.X XXXX");
        add (CR, "Step Ladder",           "X.X. .X.. .X.. ..X.");
        add (CR, "Countdown",             "X... X..X .X.X XX44");
        add (CR, "Trance Morse",          "X.XX -.X- .XX. X-..");
        add (CR, "Euro Ghost Stabs",      "X,,X ,,X, X,,X ,,X,");
        add (CR, "Rage Waterfall",        "4.4. 3.3. 2.2. X.X.");
        add (CR, "Hardstyle Hiccup",      ".oxX .ox2 .oxX .o4X");
        add (CR, "Psy Glitch Roll",       ".XX2 .XXX .X2X .XX4");
        add (CR, "Jersey Chopper",        "X... X... X..2 ..4.");
        add (CR, "Garage Skip",           "X..X.X ...X.. ...XX. X.X...", R16T);
        add (CR, "Future Wobble",         "X-X- X-X. X.X4", R8T);
        add (CR, "Big Room Tease",        "X... .... X... .... X... X... X.X. XXXX");
        add (CR, "Dub Echo",              "X... ..x. .... o...");

        return v;
    }();
    return lib;
}

// Index of the n-th preset of a category (or -1)
inline int presetIndex (const juce::String& category, int n)
{
    const auto& lib = presetLibrary();
    for (int i = 0, k = 0; i < (int) lib.size(); ++i)
        if (lib[(size_t) i].category == category && k++ == n)
            return i;
    return -1;
}

inline int presetIndexByName (const juce::String& category, const juce::String& name)
{
    const auto& lib = presetLibrary();
    for (int i = 0; i < (int) lib.size(); ++i)
        if (lib[(size_t) i].category == category && lib[(size_t) i].name == name)
            return i;
    return -1;
}

inline juce::StringArray presetCategories()
{
    juce::StringArray cats;
    for (auto& p : presetLibrary())
        cats.addIfNotAlreadyThere (p.category);
    return cats;
}

} // namespace tg
