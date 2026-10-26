# Reading Chinese

This fork is set up for learning Mandarin by reading: tap through a Chinese
book a word at a time, see pinyin, meanings and HSK level, and save the words
you want to study into a file you can import into Anki.

## Quick start with the release bundle

Each [release](https://github.com/quangnd159/CrossInk/releases) has a ready-made
SD bundle: `crossink-chinese-sdcard-vi-*.zip` (Chinese–Vietnamese) or
`crossink-chinese-sdcard-en-*.zip` (Chinese–English). Unzip it and copy each item
into the matching place on the card, creating any folder the card does not have
yet:

| In the zip | Copy into, on the card |
| --- | --- |
| `dictionaries/zh` | `dictionaries/` |
| `fonts/NotoSansSC` | `fonts/` |
| `.crosspoint/languages/chinese-simplified.yaml` | `.crosspoint/languages/` |
| `.crosspoint/languages/fonts/Noto Sans SC` | `.crosspoint/languages/fonts/` |

Only the dictionary is needed for lookups; the rest are the reading font, the
Chinese menus and the UI Font described below.

Do not drag the whole `.crosspoint` or `fonts` folder onto the card. On a Mac,
Finder offers to Replace the card's folder, which deletes your settings, reading
progress and fonts. `.crosspoint` is hidden; show it with Cmd+Shift+. in Finder,
or View > Show > Hidden items in Windows File Explorer.

## What you need on the SD card

| What | Where on the card |
| --- | --- |
| A Chinese dictionary pack (see below) | `/dictionaries/zh/` (or the hidden `/.dictionaries/zh/`) |
| A font with Chinese characters | `/fonts/<Family name>/` |

**Font.** Any font that covers Chinese works, for example Noto Sans SC.

- X3 and X4: use a `.cpfont` family (see [SD Card Fonts](sd-card-fonts.md)).
- X4 Classic, X4 Pro and Sticky: a `.cpfont` family works, and so does a
  static `.ttf` or `.otf` file (see [Scalable fonts](scalable-fonts.md)).

Then choose the family in **Settings > Reader > Font Options > Font Family**.

## Chinese menus and file names

Chinese file names, folder names and book titles display without any setup. The
firmware has Chinese glyphs built in for the menu fonts: the ~3,650 most common
simplified characters, including every HSK 3.0 character. X3 and X4 use one
glyph size for all menu text, to fit in their flash.

To switch the menus to Chinese:

1. Copy `chinese-simplified.yaml` from `lib/I18n/translations/` (or from the
   release's language ZIP) to `/.crosspoint/languages/` on the card.
2. On the reader, open **Settings > System > Device > Language** and choose
   **简体中文**. The reader restarts in Chinese.

**Rare and traditional characters (X4 Classic, X4 Pro, Sticky).** Characters
outside the built-in set show as ◆ until you add a UI Font. Make one from the
Noto Sans CJK source in this repository:

```sh
python3 lib/EpdFont/scripts/make-cjk-ui-ttf.py
```

Copy the resulting `NotoSansSC-UI.ttf` (about 9 MB) to
`/.crosspoint/languages/fonts/Noto Sans SC/`, then choose **Noto Sans SC** in
**Settings > System > Device > UI Font**. The built-in glyphs still draw the
common characters; the UI Font only fills in the rest.

## Looking words up

Open a Chinese book and start word select. Chinese has no spaces between words,
so the reader splits each sentence into dictionary words: the cursor moves one
word (他们, 读书) at a time rather than one character, and Confirm looks up the
whole word. Multi-select still lets you pick any span by hand.

The definition shows the word, its pinyin, its HSK level, the senses and any
traditional/simplified note. Lookups that the Chinese pack cannot answer fall
through to your regular dictionary, if you have one.

## Vocabulary log for Anki

Looking a word up does not save it. To keep a word, press **Save Word** in the
definition popup (on touch readers it sits next to **Switch Dictionary**; on
button readers press Right). The button then shows the word is saved, and one line
is added to `/vocab/mandarin.tsv`:

| Word | Reading | Meaning | Sentence | Book | Level |
| --- | --- | --- | --- | --- | --- |
| 第一 | dì yī | first; most important; primary | **第一**章 去银行 | my_book | HSK 1 |

The sentence is the one on the page around the word, with the word in bold.

To study the words:

1. Download the file from the web interface's file manager (or copy it off the card).
2. In Anki, choose **File > Import** and pick the file. The first lines of the
   file tell Anki it is tab-separated, contains HTML and what the columns are, so
   you only need to map the columns to your note type's fields.
3. Delete or rename the file on the card when you have imported it, so the next
   import only has new words.

## Building the dictionary pack

The pack is built on a computer from free data:

- [CC-CEDICT](https://www.mdbg.net/chinese/dictionary?page=cedict)
  (`cedict_1_0_ts_utf-8_mdbg.txt`, CC BY-SA 4.0): words, pinyin and meanings.
- [jieba](https://github.com/fxsjy/jieba) `dict.txt` (MIT): word frequencies, so
  common senses come first and sentences split the way a reader would split them.
- [hsk30](https://github.com/ivankra/hsk30) `hsk30.csv` (MIT): HSK 3.0 levels.

```sh
python3 tools/dict_convert/convert_jmdict.py --lang zh \
  --input cedict_1_0_ts_utf-8_mdbg.txt --frequency dict.txt --split-names \
  --levels hsk30.csv --level-name HSK --title "CC-CEDICT" \
  --output-dir out/zh
python3 scripts/gen_dict_spx.py out/zh
```

Copy the `out/zh` folder to the card as `/dictionaries/zh/`. `--split-names`
moves proper nouns into a separate names dictionary so they do not crowd out
ordinary words. The pack is about 18 MB, and every entry is indexed under both
its simplified and traditional form, so it also works for traditional books.

### Chinese–Vietnamese (Trung-Việt)

For Vietnamese speakers there is a pack with Vietnamese meanings and the
Sino-Vietnamese (Hán-Việt) reading of every word, shown after the pinyin:
学习 【xué xí · HỌC TẬP】 học; tìm hiểu. The vocabulary log's Reading column
carries the Hán-Việt too. It is built from:

- [Hán Thông](https://github.com/DucThanhNguyen/han-thong) `data/dict.u8`
  (CC BY-SA 4.0): [CVDICT](https://github.com/teefan/CVDICT) by Phong Phan, about
  123,000 CC-CEDICT entries translated into Vietnamese and then reviewed, with Hán Thông's
  corrections and per-word Hán-Việt. CVDICT's meanings started as machine
  translations, so rare words can read awkwardly.
- [hanviet-pinyin-wordlist](https://github.com/brianhuster/hanviet-pinyin-wordlist)
  `hanviet.csv` (MIT): Hán-Việt for each character and pinyin, filling the words
  Hán Thông leaves without one.
- jieba and hsk30, as above.

```sh
python3 tools/dict_convert/convert_jmdict.py --lang zh \
  --input dict.u8 --hanviet hanviet.csv --frequency dict.txt --split-names \
  --levels hsk30.csv --level-name HSK --title "Trung-Việt" --names-title "Tên riêng" \
  --output-dir out/zh
python3 scripts/gen_dict_spx.py out/zh
```

It goes in the same `/dictionaries/zh/` folder, in place of the English pack.

The pack format and converter come from
[Matcha Reader](https://github.com/eszter007/matcha-reader) (MIT), so packs built
for Matcha Reader work here too.

## Memory use

The pack is read from the card as you look words up; only a small index (about
9 KB) stays in memory while word select is open, and it is released when you
close it. This keeps lookup working on the X3 and X4, which have no PSRAM.
