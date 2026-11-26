#include "automata.h"

#include <algorithm>
#include <queue>
#include <stdexcept>

namespace automata {
namespace {

bool contains(const std::vector<std::string>& values, const std::string& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

void validateStatesExist(const NFA& nfa, const StateSet& states) {
    for (const State& state : states) {
        if (!contains(nfa.states, state)) {
            throw std::invalid_argument("Unknown state: " + state);
        }
    }
}

StateSet epsilonClosureUnchecked(const NFA& nfa, const StateSet& states) {
    StateSet closure = states;
    std::queue<State> pending;

    for (const State& state : states) {
        pending.push(state);
    }

    while (!pending.empty()) {
        const State current = pending.front();
        pending.pop();

        const auto destinations = nfa.epsilonTransitions.find(current);
        if (destinations == nfa.epsilonTransitions.end()) {
            continue;
        }

        for (const State& destination : destinations->second) {
            if (closure.insert(destination).second) {
                pending.push(destination);
            }
        }
    }

    return closure;
}

StateSet moveUnchecked(const NFA& nfa,
                       const StateSet& states,
                       const Symbol& symbol) {
    StateSet destinations;

    for (const State& state : states) {
        const auto transition = nfa.transitions.find({state, symbol});
        if (transition != nfa.transitions.end()) {
            destinations.insert(transition->second.begin(),
                                transition->second.end());
        }
    }

    return destinations;
}

bool containsFinalState(const StateSet& states, const StateSet& finalStates) {
    for (const State& state : states) {
        if (finalStates.count(state) != 0) {
            return true;
        }
    }
    return false;
}

}  // namespace

void NFA::addTransition(const State& from,
                        const std::optional<Symbol>& symbol,
                        const State& to) {
    if (symbol.has_value()) {
        transitions[{from, *symbol}].insert(to);
    } else {
        epsilonTransitions[from].insert(to);
    }
}

void NFA::validate() const {
    if (states.empty()) {
        throw std::invalid_argument("The NFA must contain at least one state.");
    }

    StateSet uniqueStates;
    for (const State& state : states) {
        if (state.empty()) {
            throw std::invalid_argument("State names cannot be empty.");
        }
        if (!uniqueStates.insert(state).second) {
            throw std::invalid_argument("Duplicate state: " + state);
        }
    }

    StateSet uniqueSymbols;
    for (const Symbol& symbol : alphabet) {
        if (symbol.empty()) {
            throw std::invalid_argument("Alphabet symbols cannot be empty.");
        }
        if (!uniqueSymbols.insert(symbol).second) {
            throw std::invalid_argument("Duplicate alphabet symbol: " + symbol);
        }
    }

    if (uniqueStates.count(initialState) == 0) {
        throw std::invalid_argument("The initial state is not in the state list.");
    }

    for (const State& finalState : finalStates) {
        if (uniqueStates.count(finalState) == 0) {
            throw std::invalid_argument("Unknown final state: " + finalState);
        }
    }

    for (const auto& entry : transitions) {
        const State& from = entry.first.first;
        const Symbol& symbol = entry.first.second;
        if (uniqueStates.count(from) == 0) {
            throw std::invalid_argument("Transition starts at unknown state: " + from);
        }
        if (uniqueSymbols.count(symbol) == 0) {
            throw std::invalid_argument("Transition uses undeclared symbol: " + symbol);
        }
        for (const State& destination : entry.second) {
            if (uniqueStates.count(destination) == 0) {
                throw std::invalid_argument(
                    "Transition ends at unknown state: " + destination);
            }
        }
    }

    for (const auto& entry : epsilonTransitions) {
        if (uniqueStates.count(entry.first) == 0) {
            throw std::invalid_argument(
                "Epsilon transition starts at unknown state: " + entry.first);
        }
        for (const State& destination : entry.second) {
            if (uniqueStates.count(destination) == 0) {
                throw std::invalid_argument(
                    "Epsilon transition ends at unknown state: " + destination);
            }
        }
    }
}

StateSet epsilonClosure(const NFA& nfa, const StateSet& states) {
    nfa.validate();
    validateStatesExist(nfa, states);
    return epsilonClosureUnchecked(nfa, states);
}

StateSet move(const NFA& nfa, const StateSet& states, const Symbol& symbol) {
    nfa.validate();
    validateStatesExist(nfa, states);
    if (!contains(nfa.alphabet, symbol)) {
        throw std::invalid_argument("Unknown alphabet symbol: " + symbol);
    }
    return moveUnchecked(nfa, states, symbol);
}

DFA subsetConstruction(const NFA& nfa) {
    nfa.validate();

    DFA dfa;
    dfa.alphabet = nfa.alphabet;

    using StateIdMap = std::map<StateSet, std::string>;
    StateIdMap stateIds;
    std::queue<StateSet> pending;

    const StateSet initialSubset = epsilonClosureUnchecked(nfa, {nfa.initialState});
    stateIds.emplace(initialSubset, "D0");
    pending.push(initialSubset);
    dfa.initialState = "D0";

    while (!pending.empty()) {
        const StateSet currentSubset = pending.front();
        pending.pop();
        const std::string currentId = stateIds.at(currentSubset);

        dfa.states.push_back(
            {currentId, currentSubset,
             containsFinalState(currentSubset, nfa.finalStates)});

        for (const Symbol& symbol : nfa.alphabet) {
            const StateSet destinationSubset =
                epsilonClosureUnchecked(nfa, moveUnchecked(nfa, currentSubset, symbol));

            auto destination = stateIds.find(destinationSubset);
            if (destination == stateIds.end()) {
                const std::string newId = "D" + std::to_string(stateIds.size());
                destination = stateIds.emplace(destinationSubset, newId).first;
                pending.push(destinationSubset);
            }

            dfa.transitions.push_back(
                {currentId, symbol, destination->second});
        }
    }

    return dfa;
}

}  // namespace automata
