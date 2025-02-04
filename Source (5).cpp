#ifndef __PROGTEST__
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <functional>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <optional>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <vector>
#include <unordered_map>
#include <unordered_set>

using State = unsigned;
using Symbol = uint8_t;
using Word = std::vector<Symbol>;

struct MISNFA {
    std::set<State> states;
    std::set<Symbol> alphabet;
    std::map<std::pair<State, Symbol>, std::set<State>> transitions;
    std::set<State> initialStates;
    std::set<State> finalStates;
};

struct DFA {
    std::set<State> states;
    std::set<Symbol> alphabet;
    std::map<std::pair<State, Symbol>, State> transitions;
    State initialState;
    std::set<State> finalStates;

    bool operator==(const DFA& x) const = default; // requires c++20
};
#endif

// Makes the DFA's transition function total by adding a sink state if necessary
DFA total(DFA& dfa) {
    bool isComplete = true;

    // Check if each state has a transition for every symbol
    for (const State& state : dfa.states) {
        for (const Symbol& symbol : dfa.alphabet) {
            if (dfa.transitions.find({ state, symbol }) == dfa.transitions.end()) {
                isComplete = false; // Found a missing transition
                break;
            }
        }
        if (!isComplete) break;
    }

    // If the DFA is already complete, return it as is
    if (isComplete) {
        return dfa;
    }

    // If incomplete, add a sink state
    State maxState = *std::max_element(dfa.states.begin(), dfa.states.end());
    State sinkState = maxState + 1;

    // Add the sink state to the set of states
    dfa.states.insert(sinkState);

    // Make the sink state self-loop for each symbol in the alphabet
    for (const Symbol& symbol : dfa.alphabet) {
        dfa.transitions[{sinkState, symbol}] = sinkState;
    }

    // Ensure every state has a transition for each symbol
    for (const State& state : dfa.states) {
        for (const Symbol& symbol : dfa.alphabet) {
            if (dfa.transitions.find({ state, symbol }) == dfa.transitions.end()) {
                dfa.transitions[{state, symbol}] = sinkState;
            }
        }
    }

    return dfa;
}

DFA removeUselessStates(DFA& dfa) {
    // Step 1: Identify all useful states
    std::set<State> usefulStates = dfa.finalStates;  // Start with final states as useful
    std::queue<State> queue;  // Queue to explore states leading to useful states
    
    // Initialize queue with final states
    for (const State& finalState : dfa.finalStates) {
        queue.push(finalState);
    }

    // Process states that lead to useful states
    while (!queue.empty()) {
        State currentState = queue.front();
        queue.pop();

        // Explore all states with transitions to currentState
        for (const auto& [key, targetState] : dfa.transitions) {
            const State& sourceState = key.first;

            if (targetState == currentState && usefulStates.count(sourceState) == 0) {
                // Mark sourceState as useful if it transitions to a useful state
                usefulStates.insert(sourceState);
                queue.push(sourceState);
            }
        }
    }

    // Step 2: Construct minimized DFA with only useful states
    DFA minimizedDFA;
    minimizedDFA.initialState = dfa.initialState;
    minimizedDFA.alphabet = dfa.alphabet;

    // Add only useful states to the minimized DFA
    for (const State& state : usefulStates) {
        minimizedDFA.states.insert(state);
    }

    // Add only transitions where both source and target are useful states
    for (const auto& [key, targetState] : dfa.transitions) {
        const State& sourceState = key.first;
        if (usefulStates.count(sourceState) > 0 && usefulStates.count(targetState) > 0) {
            minimizedDFA.transitions[key] = targetState;
        }
    }

    // Add only useful final states to the minimized DFA
    for (const State& finalState : dfa.finalStates) {
        if (usefulStates.count(finalState) > 0) {
            minimizedDFA.finalStates.insert(finalState);
        }
    }

    if (minimizedDFA.states.empty()) {
        minimizedDFA.states.insert(minimizedDFA.initialState);
    }


    return minimizedDFA;
}

// Function to get reachable states from a single state and a symbol in the NFA
const std::set<State> getStates(const MISNFA& nfa, State state, Symbol symbol) {
    auto key = std::make_pair(state, symbol);
    if (nfa.transitions.find(key) != nfa.transitions.end()) {
        return nfa.transitions.at(key);
    }

    return {};
}

// Function to get reachable states from a set of states and a symbol
std::set<State> getStates(const MISNFA& nfa, const std::set<State>& states, Symbol symbol) {
    std::set<State> result;
    for (const auto& state : states) {
        std::set<State> reachable = getStates(nfa, state, symbol);
        result.insert(reachable.begin(), reachable.end());
    }
    return result;
}

// Helper function to perform subset construction and convert NFA to DFA
DFA convertToDFA(const MISNFA& nfa) {
    DFA dfa;
    dfa.alphabet = nfa.alphabet;

    std::queue<std::set<State>> remaining;
    std::map<std::set<State>, State> stateMapping;
    std::vector<std::set<State>> dfaStates;

    std::set<State> dfaInitialState = nfa.initialStates;
    remaining.push(dfaInitialState);

    State stateCounter = 0;
    stateMapping[dfaInitialState] = stateCounter++;
    dfa.initialState = stateMapping[dfaInitialState];

    while (!remaining.empty()) {
        std::set<State> currentState = remaining.front();
        remaining.pop();

        if (stateMapping.find(currentState) == stateMapping.end()) {
            stateMapping[currentState] = stateCounter++;
        }

        dfaStates.push_back(currentState);

        // Determine final states for the DFA
        for (State s : currentState) {
            if (nfa.finalStates.count(s) > 0) {
                dfa.finalStates.insert(stateMapping[currentState]);
                break;
            }
        }

        for (Symbol symbol : dfa.alphabet) {
            std::set<State> newState = getStates(nfa, currentState, symbol);

            if (!newState.empty() && stateMapping.find(newState) == stateMapping.end()) {
                stateMapping[newState] = stateCounter++;
                remaining.push(newState);
            }

            if (!newState.empty()) {
                dfa.transitions[{stateMapping[currentState], symbol}] = stateMapping[newState];
            }
        }
    }

    // Add all mapped states to dfa.states
    for (const auto& kv : stateMapping) {
        dfa.states.insert(kv.second);
    }

    // Check if sink state is needed
    bool hasUndefinedTransitions = false;
    for (State state : dfa.states) {
        for (Symbol symbol : dfa.alphabet) {
            if (dfa.transitions.find({ state, symbol }) == dfa.transitions.end()) {
                hasUndefinedTransitions = true;
                break; // No need to check further for this state
            }
        }
        if (hasUndefinedTransitions) {
            break; // No need to check further states
        }
    }

    return dfa;
}

// Function to complement the DFA by toggling final states
DFA complement(const MISNFA& nfa) {
    DFA dfa = convertToDFA(nfa);
    total(dfa);

    std::set<State> newFinalStates;
    for (State state : dfa.states) {
        if (dfa.finalStates.count(state) == 0) {
            newFinalStates.insert(state);
        }
    }
    dfa.finalStates = newFinalStates;

    dfa = removeUselessStates(dfa);
  
    return dfa;
}

// Function to check if a word is accepted by the DFA
bool run(const DFA& dfa, const Word& word) {
    State currentState = dfa.initialState;

    for (Symbol symbol : word) {
        auto it = dfa.transitions.find({ currentState, symbol });
        if (it == dfa.transitions.end()) {
            return false;
        }
        currentState = it->second;
    }

    return dfa.finalStates.count(currentState) > 0;
}

#ifndef __PROGTEST__
int main()
{
    MISNFA in0 = {
        {0, 1, 2},
        {'e', 'l'},
        {
            {{0, 'e'}, {1}},
            {{0, 'l'}, {1}},
            {{1, 'e'}, {2}},
            {{2, 'e'}, {0}},
            {{2, 'l'}, {2}},
        },
        {0},
        {1, 2},
    };
    auto out0 = complement(in0);
    assert(run(out0, {}) == true);
    assert(run(out0, { 'e', 'l' }) == true);
    assert(run(out0, { 'l' }) == false);
    assert(run(out0, { 'l', 'e', 'l', 'e' }) == true);
    MISNFA in1 = {
        {0, 1, 2, 3},
        {'g', 'l'},
        {
            {{0, 'g'}, {1}},
            {{0, 'l'}, {2}},
            {{1, 'g'}, {3}},
            {{1, 'l'}, {3}},
            {{2, 'g'}, {1}},
            {{2, 'l'}, {0}},
            {{3, 'l'}, {1}},
        },
        {0},
        {0, 2, 3},
    };
    auto out1 = complement(in1);
    assert(run(out1, {}) == false);
    assert(run(out1, { 'g' }) == true);
    assert(run(out1, { 'g', 'l' }) == false);
    assert(run(out1, { 'g', 'l', 'l' }) == true);
    MISNFA in2 = {
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
        {'a', 'b'},
        {
            {{0, 'a'}, {1}},
            {{0, 'b'}, {2}},
            {{1, 'a'}, {3}},
            {{1, 'b'}, {4}},
            {{2, 'a'}, {5}},
            {{2, 'b'}, {6}},
            {{3, 'a'}, {7}},
            {{3, 'b'}, {8}},
            {{4, 'a'}, {9}},
            {{5, 'a'}, {5}},
            {{5, 'b'}, {10}},
            {{6, 'a'}, {8}},
            {{6, 'b'}, {8}},
            {{7, 'a'}, {11}},
            {{8, 'a'}, {3}},
            {{8, 'b'}, {9}},
            {{9, 'a'}, {5}},
            {{9, 'b'}, {5}},
            {{10, 'a'}, {1}},
            {{10, 'b'}, {2}},
            {{11, 'a'}, {5}},
            {{11, 'b'}, {5}},
        },
        {0},
        {5, 6},
    };
    auto out2 = complement(in2);
    assert(run(out2, {}) == true);
    assert(run(out2, { 'a' }) == true);
    assert(run(out2, { 'a', 'a', 'a', 'a', 'a', 'b' }) == true);
    assert(run(out2, { 'a', 'a', 'a', 'c', 'a', 'a' }) == false);
    MISNFA in3 = {
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
        {'e', 'j', 'k'},
        {
            {{0, 'e'}, {1}},
            {{0, 'j'}, {2}},
            {{0, 'k'}, {3}},
            {{1, 'e'}, {2}},
            {{1, 'j'}, {4}},
            {{1, 'k'}, {5}},
            {{2, 'e'}, {6}},
            {{2, 'j'}, {0}},
            {{2, 'k'}, {4}},
            {{3, 'e'}, {7}},
            {{3, 'j'}, {2}},
            {{3, 'k'}, {1}},
            {{4, 'e'}, {4}},
            {{4, 'j'}, {8}},
            {{4, 'k'}, {7}},
            {{5, 'e'}, {4}},
            {{5, 'j'}, {0}},
            {{5, 'k'}, {4}},
            {{6, 'e'}, {7}},
            {{6, 'j'}, {8}},
            {{6, 'k'}, {4}},
            {{7, 'e'}, {3}},
            {{7, 'j'}, {1}},
            {{7, 'k'}, {8}},
            {{8, 'e'}, {2}},
            {{8, 'j'}, {4}},
            {{8, 'k'}, {9}},
            {{9, 'e'}, {4}},
            {{9, 'j'}, {0}},
            {{9, 'k'}, {4}},
        },
        {0},
        {1, 6, 8},
    };
    auto out3 = complement(in3);
    assert(run(out3, {}) == true);
    assert(run(out3, { 'b', 'e', 'e' }) == false);
    assert(run(out3, { 'e', 'e', 'e' }) == false);
    assert(run(out3, { 'e', 'j' }) == true);
    assert(run(out3, { 'e', 'k', 'j', 'e', 'j', 'j', 'j', 'e', 'k' }) == true);
    MISNFA in4 = {
        {0, 1, 2, 3, 4, 5},
        {'e', 'n', 'r'},
        {
            {{0, 'e'}, {1}},
            {{0, 'n'}, {1}},
            {{0, 'r'}, {2}},
            {{1, 'e'}, {2}},
            {{1, 'n'}, {3}},
            {{1, 'r'}, {3}},
            {{2, 'e'}, {3}},
            {{2, 'n'}, {3}},
            {{2, 'r'}, {1}},
            {{3, 'e'}, {1}},
            {{3, 'n'}, {1}},
            {{3, 'r'}, {2}},
            {{4, 'r'}, {5}},
        },
        {0},
        {4, 5},
    };
    auto out4 = complement(in4);
    assert(run(out4, {}) == true);
    assert(run(out4, { 'e', 'n', 'r', 'e', 'n', 'r', 'e', 'n', 'r', 'e', 'n', 'r' }) == true);
    assert(run(out4, { 'n', 'e', 'r', 'n', 'r', 'r', 'r', 'n', 'e', 'n', 'n', 'm', '\x0c', '\x20' }) == false);
    assert(run(out4, { 'r', 'r', 'r', 'e', 'n' }) == true);
    MISNFA in5 = {
        {0, 1, 2, 3, 4, 5, 6},
        {'l', 'q', 't'},
        {
            {{0, 'l'}, {2, 4, 5}},
            {{0, 'q'}, {2}},
            {{0, 't'}, {1}},
            {{1, 'l'}, {0, 2}},
            {{1, 'q'}, {1, 4}},
            {{1, 't'}, {0, 2}},
            {{2, 'l'}, {5}},
            {{2, 'q'}, {0, 4}},
            {{2, 't'}, {0}},
            {{3, 'l'}, {3, 4}},
            {{3, 'q'}, {0}},
            {{3, 't'}, {0, 2}},
            {{4, 't'}, {4}},
            {{5, 'l'}, {0, 2}},
            {{5, 'q'}, {4, 5}},
            {{5, 't'}, {0, 2}},
            {{6, 'l'}, {4, 6}},
            {{6, 'q'}, {0}},
            {{6, 't'}, {0, 2}},
        },
        {2, 4},
        {0, 1, 3, 5, 6},
    };
    auto out5 = complement(in5);
    assert(run(out5, {}) == true);
    assert(run(out5, { 'q', 'q' }) == true);
    assert(run(out5, { 'q', 'q', 'l' }) == false);
    assert(run(out5, { 'q', 'q', 'q' }) == false);

    MISNFA in6 = {
        {0, 1, 2},
        {'a', 'b', 'c'},
        {
            {{0, 'a'}, {0}},
            {{1, 'b'}, {1}},
            {{2, 'c'}, {2}}
        },
        {0, 1, 2},
        {0, 1, 2},
    };
    auto out6 = complement(in6);

    MISNFA in7 = {
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17},
        {'b', 'k', 'y'},
        {
            {{0, 'b'}, {0, 1, 5, 6, 7, 8, 11}},
            {{0, 'k'}, {8, 11, 14, 15}},
            {{0, 'y'}, {0, 1, 5, 6, 7, 9, 16, 17}},
            {{1, 'b'}, {3, 6, 7, 14}},
            {{1, 'k'}, {1, 4, 9, 10, 11}},
            {{1, 'y'}, {3, 5, 6, 8}},
            {{2, 'b'}, {0, 5, 6, 11, 14, 16}},
            {{2, 'k'}, {0, 6, 7, 9, 10, 11, 14, 16}},
            {{2, 'y'}, {0, 1, 3, 4, 5, 6, 7, 11}},
            {{3, 'b'}, {1, 4, 5, 8, 11, 14, 15}},
            {{3, 'k'}, {4, 5, 6, 7, 8, 10, 14, 15}},
            {{3, 'y'}, {0, 1, 4, 6, 7, 8, 14, 17}},
            {{4, 'b'}, {1, 3, 6, 8, 15, 17}},
            {{4, 'k'}, {0, 4, 5}},
            {{4, 'y'}, {6, 11, 14, 15}},
            {{5, 'b'}, {3, 4, 8, 10, 15}},
            {{5, 'k'}, {1, 6, 7, 8, 9}},
            {{5, 'y'}, {0, 2, 4, 5, 7, 8, 10, 14, 15}},
            {{6, 'b'}, {7, 10, 11, 17}},
            {{6, 'k'}, {1, 2, 7, 10, 15, 17}},
            {{6, 'y'}, {8, 11, 14, 16}},
            {{7, 'b'}, {0, 3, 4, 6, 7, 9, 11, 15}},
            {{7, 'k'}, {0, 1, 3, 4, 5, 6, 11, 17}},
            {{7, 'y'}, {3, 4, 5, 8, 16}},
            {{8, 'b'}, {3, 4, 6, 7, 10, 11, 14}},
            {{8, 'k'}, {0, 1, 3, 5, 10, 14, 17}},
            {{8, 'y'}, {3, 6, 15}},
            {{9, 'b'}, {4, 7, 14}},
            {{9, 'k'}, {0, 1, 2, 5, 6, 8, 14}},
            {{9, 'y'}, {2, 4, 7, 17}},
            {{10, 'b'}, {1, 7, 17}},
            {{10, 'k'}, {0, 3, 5, 7, 8, 10}},
            {{10, 'y'}, {2, 3, 11}},
            {{11, 'b'}, {0, 1, 3, 6, 10, 14}},
            {{11, 'k'}, {1, 3, 4, 5, 11, 14, 15, 16}},
            {{11, 'y'}, {3, 4, 6, 7, 8, 9, 11, 14}},
            {{12, 'b'}, {4, 9, 10, 11, 13}},
            {{12, 'k'}, {0, 1, 2, 3, 4, 5, 7, 9, 10, 14}},
            {{12, 'y'}, {3, 4, 12, 14, 17}},
            {{13, 'b'}, {1, 3, 4, 6, 9}},
            {{13, 'k'}, {2, 4, 13, 14, 16, 17}},
            {{13, 'y'}, {8, 11, 12, 14, 17}},
            {{14, 'b'}, {15}},
            {{14, 'k'}, {14}},
            {{14, 'y'}, {14}},
            {{15, 'b'}, {15}},
            {{16, 'b'}, {0, 2, 5, 6, 11, 14}},
            {{16, 'k'}, {0, 6, 7, 9, 10, 11, 14, 16}},
            {{16, 'y'}, {0, 1, 3, 4, 5, 6, 7, 11}},
            {{17, 'b'}, {4, 7, 14}},
            {{17, 'k'}, {0, 1, 5, 6, 8, 14, 16}},
            {{17, 'y'}, {4, 7, 9, 16}},
        },
        {0, 1, 2, 3, 4, 5, 8, 9, 11, 14, 15, 16, 17},
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 14, 15, 16, 17},
    };
    auto out7 = complement(in7);
}
#endif
