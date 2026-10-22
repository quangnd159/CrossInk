# Dict — segmentation-ready CJK dictionary index

Ported from [Matcha Reader](https://github.com/eszter007/matcha-reader) (MIT,
commit `8f1d3e5`, 2026-10-21): `DictIndex`, `WordLookup` and `Deinflector`.
The on-disk format (`vocab.idx` 40-byte sorted records, `vocab.dat` definition
payloads, `vocab.spx` sparse index, optional `names.*` / `grammar.*` slots and
`*.title`) is unchanged, so packs built for Matcha Reader work here as is.

Packs are built with `tools/dict_convert/convert_jmdict.py` (also from Matcha
Reader) and `scripts/gen_dict_spx.py`. See `docs/chinese.md` for the Mandarin
setup. App integration lives in `src/util/CjkDictionary.*` and
`src/util/CjkSegmenter.*`.
