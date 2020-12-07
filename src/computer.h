#ifndef COMPUTER_H
#define COMPUTER_H

#include <array>
#include <iostream>

template <size_t N, typename Type>
inline void printMemory(std::array<Type, N>& mem) {
    std::cout << "-----MEMORY STATE----" << std::endl;
    for (size_t i = 0; i < N; i++) {
        std::cout << mem[i] << std::endl;
    }
    std::cout << "---------" << std::endl;
}

// Przykładowa operacja
template <int a, int b> struct TestOp {
    template <size_t memSize, typename memType>
    static constexpr void run(std::array<memType, memSize>& mem, bool &ZF, bool &SF) {
        mem[a] = b;
    }
};

template <typename... Ops> struct Program {
    template <size_t memSize, typename memType>
    static constexpr void run(std::array<memType, memSize>& mem, bool &ZF, bool &SF);
};

template <typename Op, typename... Ops> struct Program<Op, Ops...> {
    // recursion call
    template <size_t memSize, typename memType>
    static constexpr void run(std::array<memType, memSize>& mem, bool &ZF, bool &SF) {
        Op::template run<memSize, memType>(mem, ZF, SF);
        printMemory<memSize, memType>(mem);
        Program<Ops...>::run(mem, ZF, SF);
    }
};

template <typename Op> struct Program<Op> {
    // base of recursion
    template <size_t memSize, typename memType>
    static constexpr void run(std::array<memType, memSize>& mem, bool &ZF, bool &SF) {
        Op::template run<memSize, memType>(mem, ZF, SF);
        printMemory<memSize, memType>(mem);
    }
};

template <int N>
struct Num {
    constexpr static int value = N;
};

/*
template <typename Arg1, typename Arg2>
struct Add {
    template <size_t memSize, typename memType>
    static constexpr void run(std::array<memType, memSize>& mem) {
    }
};
*/

template <size_t N, typename Type> struct Computer {
    template <typename T> static constexpr std::array<Type, N> boot() {
        std::array<Type, N> memory{};
        bool ZF = false, SF = false;
        T::template run<N, Type>(memory, ZF, SF);
        return memory;
    }
};

#endif // COMPUTER_H
