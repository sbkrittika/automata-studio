# Automata Studio – NFA to DFA Visualizer

This project is being built in phases. The first phase implements and tests the
C++17 automata model and subset-construction algorithm. The REST API and web
interface are intentionally not part of this phase.

## Backend algorithm phase

`backend/automata.h` defines an NFA using a list of states and symbols, a
single initial state, a set of final states, and maps of symbol and epsilon
transitions. A missing transition is represented by the absence of an entry.
Passing `std::nullopt` to `NFA::addTransition` adds an epsilon transition.

`epsilonClosure` uses a queue to visit epsilon-reachable states. Its set of
visited states prevents epsilon cycles from causing repeated work. With ordered
sets and maps, for `V` states and `E_epsilon` epsilon edges, one closure takes
`O((V + E_epsilon) log V)` worst-case time and `O(V)` additional space.

`move` unions destinations for one symbol across a set of NFA states. With
`k` source states, `T` stored symbol-transition keys, and `r` matching
destinations, it takes `O(k log T + r log r)` worst-case time with the ordered
map and set.

`subsetConstruction` begins with the epsilon closure of the initial state and
processes discovered subsets with a queue. Each distinct subset is assigned
one DFA ID. For every subset and alphabet symbol it computes `move` followed by
epsilon closure; thus transitions are present even when the destination is
the empty set. The empty set, if reached, becomes a regular DFA state whose
transitions loop back to itself. A DFA subset is final if it contains any NFA
final state.

An NFA with `n` states can have up to `2^n` DFA states. With alphabet size
`a`, the construction processes at most `2^n * a` subset/symbol pairs, and its
time and space are exponential in `n` in the worst case. The queue and set/map
structures ensure each reachable subset is processed only once.

## Build and run tests

From `backend`:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The test executable covers nondeterministic transitions, multiple epsilon
edges and epsilon cycles, multiple final states, missing transitions, the
empty-set sink, and invalid input. It also compares NFA and DFA acceptance for
all words up to length four over each example alphabet.
