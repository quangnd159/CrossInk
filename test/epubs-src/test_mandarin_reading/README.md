Simplified Chinese reading fixture (original text) for the Mandarin dictionary
smoke test: covers frequency-based segmentation (including the classic 和尚未必 ambiguity),
common words, and runs that wrap across lines. Build with:

    (cd test/epubs-src/test_mandarin_reading && zip -X0 ../../epubs/test_mandarin_reading.epub mimetype && zip -Xr9D ../../epubs/test_mandarin_reading.epub META-INF OEBPS)
