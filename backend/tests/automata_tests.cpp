#include "automata.h"

#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using automata::DFA;
using automata::NFA;
using automata::State;
using automata::StateSet;
using automata::Symbol;

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

const automata::DFAState& findDfaState(const DFA& dfa, const std::string& id) {
    for (const auto& state : dfa.states) {
        if (state.id == id) {
            return state;
        }
    }
    throw std::runtime_error("DFA state not found: " + id);
}

std::string transitionDestination(const DFA& dfa,
                                  const std::string& state,
                                  const Symbol& symbol) {
    for (const auto& transition : dfa.transitions) {
        if (transition.from == state && transition.symbol == symbol) {
            return transition.to;
        }
    }
    throw std::runtime_error("DFA transition not found.");
}

bool acceptsNfa(const NFA& nfa, const std::vector<Symbol>& input) {
    StateSet current = automata::epsilonClosure(nfa, {nfa.initialState});
    for (const Symbol& symbol : input) {
        current = automata::epsilonClosure(nfa, automata::move(nfa, current, symbol));
    }
    for (const State& state : current) {
        if (nfa.finalStates.count(state) != 0) {
            return true;
        }
    }
    return false;
}

bool acceptsDfa(const DFA& dfa, const std::vector<Symbol>& input) {
    std::string current = dfa.initialState;
    for (const Symbol& symbol : input) {
        current = transitionDestination(dfa, current, symbol);
    }
    return findDfaState(dfa, current).isFinal;
}

void requireEquivalentForShortWords(const NFA& nfa, const DFA& dfa) {
    std::set<std::string> stateIds;
    std::set<StateSet> subsets;
    std::set<std::pair<std::string, Symbol>> transitionKeys;
    std::set<std::pair<std::string, Symbol>> actualTransitionKeys;

    for (const auto& state : dfa.states) {
        require(stateIds.insert(state.id).second, "DFA state IDs must be unique.");
        require(subsets.insert(state.nfaStates).second,
                "Equivalent NFA subsets must not create duplicate DFA states.");

        bool expectedFinal = false;
        for (const State& nfaState : state.nfaStates) {
            if (nfa.finalStates.count(nfaState) != 0) {
                expectedFinal = true;
                break;
            }
        }
        require(state.isFinal == expectedFinal,
                "DFA final-state marking does not match its NFA subset.");

        for (const Symbol& symbol : nfa.alphabet) {
            require(transitionKeys.insert({state.id, symbol}).second,
                    "DFA has duplicate transitions for a state and symbol.");
            const std::string destination =
                transitionDestination(dfa, state.id, symbol);
            const StateSet expectedSubset = automata::epsilonClosure(
                nfa, automata::move(nfa, state.nfaStates, symbol));
            require(findDfaState(dfa, destination).nfaStates == expectedSubset,
                    "DFA transition does not match move plus epsilon closure.");
        }
    }

    require(dfa.transitions.size() == dfa.states.size() * nfa.alphabet.size(),
            "DFA must have exactly one transition per state and symbol.");
    for (const auto& transition : dfa.transitions) {
        require(actualTransitionKeys
                    .insert({transition.from, transition.symbol})
                    .second,
                "DFA contains duplicate state/symbol transitions.");
        require(stateIds.count(transition.from) != 0 &&
                    stateIds.count(transition.to) != 0,
                "DFA transition refers to an unknown DFA state.");
    }
    require(actualTransitionKeys == transitionKeys,
            "DFA transition table is incomplete.");

    std::vector<Symbol> word;
    std::function<void(std::size_t)> checkWords = [&](std::size_t remaining) {
        require(acceptsNfa(nfa, word) == acceptsDfa(dfa, word),
                "NFA and DFA disagree on an input word.");
        if (remaining == 0) {
            return;
        }
        for (const Symbol& symbol : nfa.alphabet) {
            word.push_back(symbol);
            checkWords(remaining - 1);
            word.pop_back();
        }
    };
    checkWords(4);
}

void testOrdinaryNfaWithNondeterminism() {
    NFA nfa{{"q0", "q1", "q2"}, {"a", "b"}, "q0", {"q2"}};
    nfa.addTransition("q0", "a", "q0");
    nfa.addTransition("q0", "a", "q1");
    nfa.addTransition("q1", "b", "q2");
    nfa.addTransition("q2", "a", "q2");

    require(automata::move(nfa, {"q0"}, "a") == StateSet({"q0", "q1"}),
            "Move must collect every destination for a symbol.");
    const DFA dfa = automata::subsetConstruction(nfa);
    require(dfa.states.size() > 1, "Nondeterministic subsets were not generated.");
    requireEquivalentForShortWords(nfa, dfa);
}

void testEpsilonTransitionsAndCycles() {
    NFA nfa{{"q0", "q1", "q2", "q3", "q4"}, {"a"}, "q0", {"q4"}};
    nfa.addTransition("q0", std::nullopt, "q1");
    nfa.addTransition("q0", std::nullopt, "q2");
    nfa.addTransition("q1", std::nullopt, "q3");
    nfa.addTransition("q2", std::nullopt, "q3");
    nfa.addTransition("q3", std::nullopt, "q1");
    nfa.addTransition("q3", "a", "q4");

    const StateSet closure = automata::epsilonClosure(nfa, {"q0"});
    require(closure == StateSet({"q0", "q1", "q2", "q3"}),
            "Epsilon closure did not include all reachable states.");
    require(automata::epsilonClosure(nfa, {"q0", "q4"}) ==
                StateSet({"q0", "q1", "q2", "q3", "q4"}),
            "Epsilon closure must include every state in its input set.");

    const DFA dfa = automata::subsetConstruction(nfa);
    require(findDfaState(dfa, dfa.initialState).nfaStates == closure,
            "Initial DFA state is not the initial epsilon closure.");
    requireEquivalentForShortWords(nfa, dfa);
}

void testMultipleFinalStates() {
    NFA nfa{{"start", "left", "right"}, {"x", "y"}, "start",
            {"left", "right"}};
    nfa.addTransition("start", "x", "left");
    nfa.addTransition("start", "y", "right");

    const DFA dfa = automata::subsetConstruction(nfa);
    require(findDfaState(dfa, transitionDestination(dfa, dfa.initialState, "x"))
                .isFinal,
            "Subset containing the first final state must be final.");
    require(findDfaState(dfa, transitionDestination(dfa, dfa.initialState, "y"))
                .isFinal,
            "Subset containing the second final state must be final.");
    requireEquivalentForShortWords(nfa, dfa);
}

void testMissingTransitionsAndEmptySetState() {
    NFA nfa{{"start", "accept"}, {"a", "b"}, "start", {"accept"}};
    nfa.addTransition("start", "a", "accept");

    const DFA dfa = automata::subsetConstruction(nfa);
    const std::string deadId =
        transitionDestination(dfa, dfa.initialState, "b");
    const auto& deadState = findDfaState(dfa, deadId);
    require(deadState.nfaStates.empty(), "Missing transition must reach empty set.");
    require(transitionDestination(dfa, deadId, "a") == deadId &&
                transitionDestination(dfa, deadId, "b") == deadId,
            "Empty-set state must loop on every alphabet symbol.");
    require(dfa.transitions.size() == dfa.states.size() * dfa.alphabet.size(),
            "DFA transition table must be complete.");
    requireEquivalentForShortWords(nfa, dfa);
}

void testMissingTransitionWithoutEmptyState() {
    NFA nfa{{"start", "accept"}, {"a"}, "start", {"accept"}};
    nfa.addTransition("start", "a", "accept");

    const DFA dfa = automata::subsetConstruction(nfa);
    for (const auto& state : dfa.states) {
        require(!state.nfaStates.empty(),
                "Empty set should not be created without an empty move.");
    }
}

void testValidation() {
    NFA nfa{{"q0"}, {"a"}, "q0", {}};
    nfa.addTransition("q0", "b", "q0");

    bool rejected = false;
    try {
        automata::subsetConstruction(nfa);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "Undeclared transition symbols must be rejected.");
}

}  // namespace

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"ordinary nondeterministic NFA", testOrdinaryNfaWithNondeterminism},
        {"epsilon transitions and cycles", testEpsilonTransitionsAndCycles},
        {"multiple final states", testMultipleFinalStates},
        {"missing transitions and empty-set state",
         testMissingTransitionsAndEmptySetState},
        {"missing transition without empty-set state",
         testMissingTransitionWithoutEmptyState},
        {"input validation", testValidation},
    };

    try {
        for (const auto& test : tests) {
            test.second();
            std::cout << "[PASS] " << test.first << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }

    return 0;
}
