#ifndef AUTOMATA_STUDIO_AUTOMATA_H
#define AUTOMATA_STUDIO_AUTOMATA_H

#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace automata {

using State = std::string;
using Symbol = std::string;
using StateSet = std::set<State>;
using TransitionKey = std::pair<State, Symbol>;

struct NFA {
    std::vector<State> states;
    std::vector<Symbol> alphabet;
    State initialState;
    StateSet finalStates;
    std::map<State, StateSet> epsilonTransitions;
    std::map<TransitionKey, StateSet> transitions;

    void addTransition(const State& from,
                       const std::optional<Symbol>& symbol,
                       const State& to);
    void validate() const;
};

struct DFAState {
    std::string id;
    StateSet nfaStates;
    bool isFinal = false;
};

struct DFATransition {
    std::string from;
    Symbol symbol;
    std::string to;
};

struct DFA {
    std::vector<DFAState> states;
    std::vector<Symbol> alphabet;
    std::string initialState;
    std::vector<DFATransition> transitions;
};

StateSet epsilonClosure(const NFA& nfa, const StateSet& states);
StateSet move(const NFA& nfa, const StateSet& states, const Symbol& symbol);
DFA subsetConstruction(const NFA& nfa);

}  // namespace automata

#endif
