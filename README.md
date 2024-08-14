# DirectionField

A simple direction field visualizer written in C. Made for a very specific use case in Calculus class.

## Formulas

Out of laziness I didn't bother to implement a formula parser, so I made a whole system for executable formulas. The documentation for it is [here](docs/formulas.md).

## Building

**Requirements:**
- gcc
- make
- scc (For LOC)

To build this project yourself, clone the repo and run `make [build]` or `make release`.
The final binary will be under `build/bin`.
