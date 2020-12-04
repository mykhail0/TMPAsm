#include <array>
#include <iostream>

// TODO pewnie zamienić struct na klasy ale teraz tak jest łatwiej
// CZEMU NIE DZIAŁA????

template <typename... Ops> struct Program {

    //  wywołanie końcowe
    template <int memSize, typename memType, typename Op>
    static constexpr void run(std::array<memType, memSize>& mem) {
        // costa costam, jakiś op::run
        std::cout << "koniec rekurencji xd" << std::endl;
    }
    //  wywołanie rekurencyjne
    template <int memSize, typename memType, typename Op, typename... OpsToRun>
    static constexpr void run(std::array<memType, memSize>& mem) {
        // costam costam, jakiś op::run
        std::cout << "rekurencja xd" << std::endl;
        run<memSize, memType, OpsToRun...>(mem);
    }

    // // startowe wywołanie
    // template <int memSize, typename memType>
    // static constexpr void run(std::array<memType, memSize>& mem) {
    //     // costa costam, jakiś op::run
    //     std::cout << "pierwsze wywołanie xd" << std::endl;
    //     run<Ops..., memSize, memType>(mem);
    // }
};

struct xd {};

template <int N, typename Type> struct Computer {
    template <typename T> static constexpr std::array<Type, N> boot() {
        std::array<Type, N> memory;
        T::template run<N, Type, int, int>(memory);
        return memory;
    }
};

int main() { Computer<10, int>::boot<Program<xd>>(); }