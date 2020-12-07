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

template <typename Op, typename... Ops> struct Program {
    //  wywołanie rekurencyjne
    template <size_t memSize, typename memType>
    static constexpr void run(std::array<memType, memSize>& mem) {
        Op::template run<memSize, memType>(mem);
        printMemory<memSize, memType>(mem);
        Program<Ops...>::run(mem);
    };
};
template <typename Op> struct Program<Op> {
    //  wywołanie bazowe
    template <size_t memSize, typename memType>
    static constexpr void run(std::array<memType, memSize>& mem) {
        Op::template run<memSize, memType>(mem);
        printMemory<memSize, memType>(mem);
    };
};

// Przykładowa operacja
template <int a, int b> struct TestOp {
    template <size_t memSize, typename memType>
    static constexpr void run(std::array<memType, memSize>& mem) {
        mem[a] = b;
    }
};

template <size_t N, typename Type> struct Computer {
    template <typename T> static constexpr std::array<Type, N> boot() {
        std::array<Type, N> memory{};
        T::template run<N, Type>(memory);
        return memory;
    }
};

int main() {
    Computer<10, int>::boot<
        Program<TestOp<0, 3>, TestOp<1, 2>, TestOp<0, 2>, TestOp<8, 2137>>>();
}