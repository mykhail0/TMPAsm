#ifndef COMPUTER_H
#define COMPUTER_H

#include <array>
#include <utility>
#include <iostream>
#include <cassert>
#include <type_traits>

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

    template<size_t memSize>
    constexpr size_t get_addr(uint64_t id, std::array<uint64_t, memSize> &addr, size_t sz) {
        for (size_t i = 0; i < sz; ++i) {
            if (addr[i] == id)
                return i;
        }
        return sz;
    }

    //struct State {
    //};
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

template<size_t N>
inline void printAddr(std::array<uint64_t, N> &mem) {
    std::cout << "-----ADDRESS STATE----" << std::endl;
    for (size_t i = 0; i < N; i++) {
        std::cout << mem[i] << std::endl;
    }
    std::cout << "---------" << std::endl;
}


//-------------OPERATIONS---------------------------

enum OpType {
    TEST, LABEL, JMP, JZ, JS, DECL
};

// Przykładowa operacja
template<int a, int b>
struct TestOp {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute(std::array<memType, memSize> &mem, bool &ZF, bool &SF) {
        mem[a] = b;
    }

    template<size_t memSize>
    static constexpr void load_variable(std::array<uint64_t, memSize> &addr, size_t &last_free) {}
    static constexpr OpType type = TEST;
};

// VARIABLES IN PROGRAM
template<auto N>
struct Num {
    static_assert(std::is_integral<decltype(N)>::value, "Integral required.");
    constexpr static auto value = N;
};

constexpr uint64_t Id(const char s[]) {
    uint64_t ans = 0, base_power = 1;
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


// Loading variables into memory.
// TODO weak solution but idk
// Mem would use get() function to access address, passing addr and last_free to it
template<uint64_t id>
struct Lea {
    template<size_t memSize>
    constexpr static size_t get(std::array<uint64_t, memSize> &addr, size_t last_free) {
        return get_addr(id, addr, last_free);
    }
};

template<uint64_t id, typename T>
struct D {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute(std::array<memType, memSize> &mem, bool &ZF, bool &SF);

    template<size_t memSize>
    static constexpr void load_variable(std::array<uint64_t, memSize> &addr, size_t &last_free) {
        throw "Value is not Numeric.";
    }
    static constexpr OpType type = DECL;
};

template<uint64_t id, auto val>
struct D<id, Num<val>> {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute(std::array<memType, memSize> &mem, bool &ZF, bool &SF) {}

    template<size_t memSize>
    static constexpr void load_variable(std::array<uint64_t, memSize> &addr, size_t &last_free) {
        assert(last_free < memSize);
        // two times the same variable causes compilation error
        assert(get_addr<memSize>(id, addr, last_free) == last_free);
        addr[last_free++] = id;
    }
    static constexpr OpType type = DECL;
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

    template<size_t memSize>
    static constexpr void load_variable(std::array<uint64_t, memSize> &addr, size_t &last_free) {}
    static constexpr OpType type = LABEL;
    static constexpr uint64_t id = Id;
};

template<uint64_t Id>
struct Jmp {

    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute(std::array<memType, memSize> &mem, bool &ZF, bool &SF) {
        labels::template find_and_run<Id,memSize, memType, labels>(mem, ZF, SF);
    }

    template<size_t memSize>
    static constexpr void load_variable(std::array<uint64_t, memSize> &addr, size_t &last_free) {}
    static constexpr OpType type = JMP;
    static constexpr uint64_t id = Id;
};

template<uint64_t Id>
struct Jz {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute(std::array<memType, memSize> &mem, bool &ZF, bool &SF) {
        labels::template find_and_run<Id,memSize, memType, labels>(mem, ZF, SF);
    }

    template<size_t memSize>
    static constexpr void load_variable(std::array<uint64_t, memSize> &addr, size_t &last_free) {}
    static constexpr OpType type = JZ;
    static constexpr uint64_t id = Id;
};

template<uint64_t Id>
struct Js {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute(std::array<memType, memSize> &mem, bool &ZF, bool &SF) {
        labels::template find_and_run<Id,memSize, memType, labels>(mem, ZF, SF);
    }

    template<size_t memSize>
    static constexpr void load_variable(std::array<uint64_t, memSize> &addr, size_t &last_free) {}
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

    template<size_t memSize>
    static constexpr void load_variables(std::array<uint64_t, memSize> &addr, size_t &last_free);
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

    template<size_t memSize>
    static constexpr void load_variables(std::array<uint64_t, memSize> &addr, size_t &last_free) {
        if (Op::type == DECL) {
            Op::template load_variable<memSize>(addr, last_free);
            //std::cout << "Declaration" << std::endl;
            //printAddr<memSize>(addr);
        }
        Program<Ops...>::template load_variables<memSize>(addr, last_free);
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

    template<size_t memSize>
    static constexpr void load_variables(std::array<uint64_t, memSize> &addr, size_t &last_free) {
        if (Op::type == DECL) {
            Op::template load_variable<memSize>(addr, last_free);
            //printAddr<memSize>(addr);
        }
    }
};

template<size_t N, typename Type>
struct Computer {
    template<typename T>
    static constexpr std::array<Type, N> boot() {
        static_assert(std::is_integral<Type>::value, "Computer requires integral types.");

        std::array<Type, N> memory{};
        std::array<uint64_t, N> addresses{};
        bool ZF = false, SF = false;
        size_t variables_cnt = 0;

        T::template load_variables<N>(addresses, variables_cnt);

        using labels = typename LabelList<T>::result;
        // std::array<uint64_t, n_labels> labels;
        // std::cout << "n_labels: " << n_labels << std::endl;

        T::template run<N, Type, labels>(memory, ZF, SF);
        return memory;
    }
};

#endif // COMPUTER_H
