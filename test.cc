#include <array>
#include <iostream>

template <typename... Ops> struct Program {
    template <int memSize, typename memType>
    static constexpr void run(std::array<memType, memSize>& mem);
};
template <typename Op, typename... Ops> struct Program<Op, Ops...> {
    //  wywołanie rekurencyjne
    template <int memSize, typename memType>
    static constexpr void run(std::array<memType, memSize>& mem) {
        std::cout << "rekurencja" << std::endl;
        Program<Ops...>::run(mem);
    };
};
template <typename Op> struct Program<Op> {
    //  wywołanie bazowe
    template <int memSize, typename memType>
    static constexpr void run(std::array<memType, memSize>& mem) {
        std::cout << "koniec rekurencji " << mem[0] << std::endl;
        mem[0] = 4;
        std::cout << "koniec rekurencji " << mem[0] << std::endl;
    };
};

struct xd {};

template <int N, typename Type> struct Computer {
    template <typename T> static constexpr std::array<Type, N> boot() {
        std::array<Type, N> memory{};
        T::template run<N, Type>(memory);
        return memory;
    }
};

int main() { Computer<10, int>::boot<Program<xd>>(); }