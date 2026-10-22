`fixture/zh` is generated from `fixture-src/` (a CC-CEDICT excerpt, CC BY-SA 4.0):

    python3 tools/dict_convert/convert_jmdict.py --lang zh \
      --input test/cjk_dictionary/fixture-src/mini_cedict.u8 \
      --frequency test/cjk_dictionary/fixture-src/freq.txt \
      --title "Mini CEDICT" --output-dir test/cjk_dictionary/fixture/zh

CC-CEDICT is © MDBG and contributors (https://www.mdbg.net/chinese/dictionary?page=cedict),
licensed under Creative Commons Attribution-ShareAlike 4.0. The excerpt keeps that license.
