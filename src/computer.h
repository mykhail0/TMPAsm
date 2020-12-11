#ifndef COMPUTER_H
#define COMPUTER_H

#include <array>
#include <utility>
#include <iostream>
#include <cassert>

namespace {
    constexpr unsigned MAX_ID_LEN = 6;
    // [0..9], [a-z]
    constexpr int ALLOWED_CHAR_CNT = ('z' - 'a' + 1) + ('9' - '0' + 1);

    constexpr int encode_char(char c) {
        if ('0' <= c && c <= '9')
            return static_cast<int>(c) - '0';
        if ('a' <= c && c <= 'z')
            return c - 'a' + 10;
        if ('A' <= c && c <= 'Z')
            return c - 'A' + 10;
        throw "Character out of range";
    }
};

//---------------DEBUG----------------------
template<size_t N, typename Type>
inline void printMemory(std::array<Type, N> &mem) {
    std::cout << "-----MEMORY STATE----" << std::endl;
    for (size_t i = 0; i < N; i++) {
        std::cout << mem[i] << std::endl;
    }
    std::cout << "---------" << std::endl;
}

//-------------OPERATIONS---------------------------

enum OpType {
    TEST, LABEL, JMP, JZ, JS
};

// Przykładowa operacja
template<int a, int b>
struct TestOp {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute(std::array<memType, memSize> &mem, bool &ZF, bool &SF) {
        mem[a] = b;
    }

    static constexpr OpType type = TEST;
};

template<typename Label, typename Program>
struct LabelHolder {
    using program = Program;
    using label = Label;
};

template<uint64_t Id>
struct Label {

    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute(std::array<memType, memSize> &mem, bool &ZF, bool &SF) {}

    static constexpr OpType type = LABEL;
    static constexpr uint64_t id = Id;
};

template<uint64_t Id>
struct Jmp {

    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute(std::array<memType, memSize> &mem, bool &ZF, bool &SF) {
        labels::template find_and_run<Id,memSize, memType, labels>(mem, ZF, SF);
    }

    static constexpr OpType type = JMP;
    static constexpr uint64_t id = Id;
};

template<uint64_t Id>
struct Jz {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute(std::array<memType, memSize> &mem, bool &ZF, bool &SF) {
        labels::template find_and_run<Id,memSize, memType, labels>(mem, ZF, SF);
    }

    static constexpr OpType type = JZ;
    static constexpr uint64_t id = Id;
};

template<uint64_t Id>
struct Js {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute(std::array<memType, memSize> &mem, bool &ZF, bool &SF) {
        labels::template find_and_run<Id,memSize, memType, labels>(mem, ZF, SF);
    }

    static constexpr OpType type = JS;
    static constexpr uint64_t id = Id;
};

//----------------HANDLING LABELS-------------------
template<typename... Labels>
struct LabelList;

// Create list

// recursion if label
template<template<typename...> class Program, typename... Ops,
        typename... Labels, uint64_t n>
struct LabelList<Program<Label<n>, Ops...>, Labels...> {
    using result =
    typename LabelList<Program<Ops...>, Labels...,
            LabelHolder<Label<n>, Program<Ops...>>>::result;
};

// recursion otherwise
template<template<typename...> class Program, typename... Ops, typename Op,
        typename... Labels>
struct LabelList<Program<Op, Ops...>, Labels...> {
    using result = typename LabelList<Program<Ops...>, Labels...>::result;
};

// base case if label
template<template<typename...> class Program, typename... Labels, uint64_t n>
struct LabelList<Program<Label<n>>, Labels...> {
    using result =
    LabelList<Program<>, Labels..., LabelHolder<Label<n>, Program<>>>;
};

// base case otherwise
template<template<typename...> class Program, typename Op, typename... Labels>
struct LabelList<Program<Op>, Labels...> {
    using result = LabelList<Program<>, Labels...>;
};

// Handle Jump operations

// recursion
template<template<typename...> class Program, typename Label,
        typename... Labels>
struct LabelList<Program<>, Label, Labels...> {
    template<uint64_t id, size_t memSize, typename memType, typename labels>
    static constexpr void find_and_run(
                                       std::array<memType, memSize> &mem,
                                       bool &ZF, bool &SF) {
        // std::cout << Label::label::type << std::endl;
        if (Label::label::id == id) {
            Label::program::template run<memSize, memType, labels>(mem, ZF, SF);
        } else {
            LabelList<Program<>, Labels...>::template
            find_and_run<id, memSize, memType, labels>(mem, ZF, SF);
        }
    }
};

// base case
template<template<typename...> class Program, typename Label>
struct LabelList<Program<>, Label> {
    template<uint64_t id, size_t memSize, typename memType, typename labels>
    static constexpr void find_and_run(
                                       std::array<memType, memSize> &mem,
                                       bool &ZF, bool &SF) {
        static_assert(Label::label::id == id, "Label doesn't exist");
        Label::program::template run<memSize, memType, labels>(mem, ZF, SF);
    }
};

//------------------PROGRAM----------------------
template<typename... Ops>
struct Program {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void run(std::array<memType, memSize> &mem, bool &ZF, bool &SF);
};

template<typename Op, typename... Ops>
struct Program<Op, Ops...> {
    // recursion call
    template<size_t memSize, typename memType, typename labels>
    static constexpr void run(std::array<memType, memSize> &mem, bool &ZF, bool &SF) {
        switch (Op::type) {
            case JMP:
//                std::cout << "JUMP" << std::endl;
                Op::template execute<memSize, memType, labels>(mem, ZF, SF);
                break;
            case JZ:
//                std::cout << "JUMPZ" << std::endl;
                if (ZF) {
                    Op::template execute<memSize, memType, labels>(mem, ZF, SF);
                } else {
                    Program<Ops...>::template run<memSize, memType, labels>(mem, ZF, SF);
                }
                break;
            case JS:
//                std::cout << "JUMPS" << std::endl;
                if (SF) {
                    Op::template execute<memSize, memType, labels>(mem, ZF, SF);
                } else {
                    Program<Ops...>::template run<memSize, memType, labels>(mem, ZF, SF);
                }
                break;
            default:
                Op::template execute<memSize, memType, labels>(mem, ZF, SF);
//                printMemory<memSize, memType>(mem);
                Program<Ops...>::template run<memSize, memType, labels>(mem, ZF, SF);
                break;
        }
    }
};

template<typename Op>
struct Program<Op> {
    // base of recursion
    template<size_t memSize, typename memType, typename labels>
    static constexpr void run(std::array<memType, memSize> &mem, bool &ZF, bool &SF) {
        switch (Op::type) {
            case JMP:
//                std::cout << "JMP" << std::endl;
                Op::template execute<memSize, memType, labels>(mem, ZF, SF);
                break;
            case JZ:
//                std::cout << "JZ" << std::endl;
                if (ZF) {
                    Op::template execute<memSize, memType, labels>(mem, ZF, SF);
                }
                break;
            case JS:
//                std::cout << "JS" << std::endl;
                if (SF) {
                    Op::template execute<memSize, memType, labels>(mem, ZF, SF);
                }
                break;
            default:
                Op::template execute<memSize, memType, labels>(mem, ZF, SF);
//                printMemory<memSize, memType>(mem);
                break;
        }
    }
};

template<int N>
struct Num {
    constexpr static int value = N;
};

// VARIABLES IN PROGRAM
// test for passing null, for passing pointer
// string constructor should be constexpr afaik
constexpr long long Id(const char s[]) {
    long long ans = 0, base_power = 1;
    unsigned char_cnt = 0;

    while (s[char_cnt] != '\0') {
        int char_val = encode_char(s[char_cnt]);
        ans += char_val * base_power;

        char_cnt++;
        base_power *= ALLOWED_CHAR_CNT;
        assert(char_cnt <= MAX_ID_LEN);
    }
    assert(char_cnt != 0);
    return ans;
}

//https://stackoverflow.com/a/16491172
template <int kk, int vv>
struct KeyValue
{
    static const int k = kk, v = vv;
};

template <int dflt, typename...>
struct ct_map;

template <int dflt>
struct ct_map<dflt>
{
    template<int>
    struct get
    {
        static const int val = dflt;
    };
};

template<int dflt, int k, int v, typename... rest>
struct ct_map<dflt, kv<k, v>, rest...>
{
    template<int kk>
    struct get
    {
        static const int val =
            (kk == k) ?
            v :
            ct_map<dflt, rest...>::template get<kk>::val;
    };
};

typedef ct_map<42, kv<10, 20>, kv<11, 21>, kv<23, 7>> mymap;

// Loading variables into memory.
template <typename T>
struct Lea {
};

/* SO answer. Need to see if works. How do I find all Id's in Program?
Also need to assign Lea's to them.
using Item = std::pair<int, int>;
constexpr Item map_items[] = {
    { 6, 7 },
    { 10, 12 },
    { 300, 5000 },
};

constexpr auto map_size = sizeof map_items/sizeof map_items[0];

static constexpr int findValue(int key, int range = map_size) {
    return
            (range == 0) ? throw "Key not present":
            (map_items[range - 1].first == key) ? map_items[range - 1].second:
            findValue(key, range - 1);
};

static constexpr int findKey(int value, int range = map_size) {
    return
            (range == 0) ? throw "Value not present":
            (map_items[range - 1].second == value) ? map_items[range - 1].first:
            findKey(value, range - 1);
};
*/

template<size_t N, typename Type>
struct Computer {
    template<typename T>
    static constexpr std::array<Type, N> boot() {
        std::array<Type, N> memory{};
        bool ZF = false, SF = false;
        using labels = typename LabelList<T>::result;
        // std::array<uint64_t, n_labels> labels;
        // std::cout << "n_labels: " << n_labels << std::endl;
        T::template run<N, Type, labels>(memory, ZF, SF);
        return memory;
    }
};

#endif // COMPUTER_H
