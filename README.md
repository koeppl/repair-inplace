Re-Pair In-Place
================

This is a proof-of-concept implementation of the paper

> Dominik Köppl, Tomohiro I, Isamu Furuya, Yoshimasa Takabatake, Kensuke Sakai, Keisuke Goto:
> Re-Pair In-Place. CoRR abs/1908.04933 (2019)

It is written in C++17 without external requirements.
You can compile the program `repair-inplace` with a recent GCC version and `make`.
You can change the compile flags for a release or debug build in the `Makefile`.
Running the program `repair-inplace` without parameters displays its parameters and its usage.
Add `-DVERBOSE` and `-DVVERBOSE` to enhance verbose or very verbose messages, outputting, among others, the created rules.

The program `repair-inplace` takes as input a file, which it treats as a byte stream.
So the input alphabet size is the byte alphabet.
We simplified the implementation in that we store the input in a 16-bit array of fixed size.
Hence, the program works whenever the sum of all different terminals and non-terminal is less than 2^16.
It is easy to enhance the program with a dynamic resizing of the bit width of the text by using a variable bit vector like 
the `IntVector` of [bit span](https://github.com/tudocomp/bit_span).
We refrained from adding this feature as the program is already very slow, taking already one hour for computing Re-Pair on 1 MiB text files.
This slow running time looks inevitable, as it is an O(n^2) time algorithm, where the time bound looks tight.
We did not implement the word-packed variant in the paper, as the number of involved operations look to make the algorithm actually slower.

As this is a proof-of-concept implementation, it lacks of crucial features like

* there is no output - however you can extract the rules and the start symbol from the log messages
* there is no decompression verifying that the text is restorable from the output
* the code is far from optimized (see the CAVEAT comments)
