#ifndef COMPUTER_H
#define COMPUTER_H

#include <array>
#include <iostream>

//---------------DEBUG----------------------
template<size_t N, typename Type>
inline void printMemory(std::array <Type, N> &mem) {
    std::cout << "-----MEMORY STATE----" << std::endl;
    for (size_t i = 0; i < N; i++) {
        std::cout << mem[i] << std::endl;
    }
    std::cout << "---------" << std::endl;
}

//-------------OPERATIONS---------------------------

enum OpType {
    TEST, LABEL, JUMP
};

// Przykładowa operacja
template<int a, int b>
struct TestOp {
    template<size_t memSize, typename memType>
    static constexpr void execute(std::array <memType, memSize> &mem, bool &ZF, bool &SF) {
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

    template<size_t memSize, typename memType>
    static constexpr void execute(std::array <memType, memSize> &mem, bool &ZF, bool &SF) {}

    static constexpr OpType type = LABEL;
    static constexpr uint64_t id = Id;
};

template<uint64_t Id>
struct Jump {

    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute(std::array <memType, memSize> &mem, bool &ZF, bool &SF) {
        labels::template find_and_run<memSize, memType, labels>(Id, mem, ZF, SF);
    }

    static constexpr OpType type = JUMP;
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
    template<size_t memSize, typename memType, typename labels>
    static constexpr void find_and_run(uint64_t id,
                                       std::array <memType, memSize> &mem,
                                       bool &ZF, bool &SF) {
        // std::cout << Label::label::type << std::endl;
        if (Label::label::id == id) {
            Label::program::template run<memSize, memType, labels>(mem, ZF, SF);
        } else {
            LabelList<Program<>, Labels...>::template find_and_run<
                    memSize, memType, labels>(mem, ZF, SF);
        }
    }
};

// base case
template<template<typename...> class Program, typename Label>
struct LabelList<Program<>, Label> {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void find_and_run(uint64_t id,
                                       std::array <memType, memSize> &mem,
                                       bool &ZF, bool &SF) {
        // std::cout << Label::label::type << std::endl;
        if (Label::label::id == id) {
            Label::program::template run<memSize, memType, labels>(mem, ZF, SF);
        }
    }
};

//------------------PROGRAM----------------------
template<typename... Ops>
struct Program {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void run(std::array <memType, memSize> &mem, bool &ZF, bool &SF);
};

template<typename Op, typename... Ops>
struct Program<Op, Ops...> {
    // recursion call
    template<size_t memSize, typename memType, typename labels>
    static constexpr void run(std::array <memType, memSize> &mem, bool &ZF, bool &SF) {
        switch (Op::type) {
            case JUMP:
                std::cout << "JUMP" << std::endl;
                Op::template execute<memSize, memType, labels>(mem, ZF, SF);
                break;
            default:
                Op::template execute<memSize, memType>(mem, ZF, SF);
                printMemory<memSize, memType>(mem);
                Program<Ops...>::template run<memSize, memType, labels>(mem, ZF, SF);
                break;
        }
    }
};

template<typename Op>
struct Program<Op> {
    // base of recursion
    template<size_t memSize, typename memType, typename labels>
    static constexpr void run(std::array <memType, memSize> &mem, bool &ZF, bool &SF) {
        switch (Op::type) {
            case JUMP:
                std::cout << "JUMP" << std::endl;
                Op::template execute<memSize, memType, labels>(mem, ZF, SF);
                break;
            default:
                Op::template execute<memSize, memType>(mem, ZF, SF);
                printMemory<memSize, memType>(mem);
                break;
        }
    }
};

template<int N>
struct Num {
    constexpr static int value = N;
};

// struct Id {
//     constexpr static int MAX_LENGTH = 6;
//     constexpr char *name;
//   private:
//     constexpr static bool check_id(const char *s) {
//         bool ans =
//     }

//     constexpr Id(const char *s) : name(s) {
//         static_assert(s != NULL);
//         static_assert(Id::check_id(s));
//     }
// };

/*
template <typename Arg1, typename Arg2>
struct Add {
    template <size_t memSize, typename memType>
    static constexpr void run(std::array<memType, memSize>& mem) {
    }
};
*/

template<size_t N, typename Type>
struct Computer {
    template<typename T>
    static constexpr std::array <Type, N> boot() {
        std::array <Type, N> memory{};
        bool ZF = false, SF = false;
        using labels = typename LabelList<T>::result;
        // std::array<uint64_t, n_labels> labels;
        // std::cout << "n_labels: " << n_labels << std::endl;
        T::template run<N, Type, labels>(memory, ZF, SF);
        return memory;
    }
};

#endif // COMPUTER_H
