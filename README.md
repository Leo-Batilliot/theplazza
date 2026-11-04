# Plazza

![Demo](docs/demo.gif)

Pizzeria simulation in C++ combining multi-processing (kitchens), multi-threading
(cooks) and IPC (reception ↔ kitchens).

> Stub README. Full content (architecture, IPC protocol, bonus list) lands as
> the milestones progress. See [`PROJECT.md`](./PROJECT.md) for the spec and
> [`plan.md`](./plan.md) for the ticket backlog.

## Build

Requires a C++17 compiler and GNU make. Tests additionally require
[Criterion](https://github.com/Snaipe/Criterion).

```bash
make            # builds the ./plazza binary
make tests_run  # builds and runs the unit-test binary
make re         # full rebuild
make fclean     # remove binaries + objects
```

## Usage

```bash
./plazza <multiplier> <cooks_per_kitchen> <restock_time_ms>
```

| Argument             | Type           | Notes                                                    |
|----------------------|----------------|----------------------------------------------------------|
| `multiplier`         | float ≥ 0      | Acts as a divisor of cooking time when in `[0, 1]`.      |
| `cooks_per_kitchen`  | positive int   | Number of cook threads each kitchen process spawns.      |
| `restock_time_ms`    | positive int   | Milliseconds between `+1` of every ingredient.           |

Invalid arguments print usage on `stderr` and exit `84` (Epitech convention).

### Order grammar

```
S       := TYPE SIZE NUMBER [; TYPE SIZE NUMBER]*
TYPE    := [a..zA..Z]+
SIZE    := S | M | L | XL | XXL
NUMBER  := x[1..9][0..9]*
```

Example:

```
regina XXL x2; fantasia M x3; margarita S x1
```

## Layout

```
src/         production sources (one class per .hpp/.cpp pair)
tests/       Criterion unit & integration tests, mirrors src/
include/     public headers (if any)
bonus/       bonus features (M8)
```

## Contributing

See [`CONTRIBUTING.md`](./CONTRIBUTING.md) for commit conventions, ticket
workflow and review gates.
