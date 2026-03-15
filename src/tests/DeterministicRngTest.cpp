#include "../logic/core/DeterministicRng.h"

#include <iostream>

namespace {

bool verify(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        return false;
    }
    return true;
}

}  // namespace

int main() {
    bool ok = true;

    tcp::logic::DeterministicRng a(123456U);
    tcp::logic::DeterministicRng b(123456U);
    tcp::logic::DeterministicRng c(654321U);

    for (int i = 0; i < 64; ++i) {
        const auto va = a.nextU32();
        const auto vb = b.nextU32();
        const auto vc = c.nextU32();

        ok &= verify(va == vb, "same seed generated different sequence");
        if (i == 0) {
            ok &= verify(va != vc, "different seed generated same first value");
        }
    }

    tcp::logic::DeterministicRng rangeRng(777U);
    for (int i = 0; i < 128; ++i) {
        const auto value = rangeRng.nextI32(-5, 5);
        ok &= verify(value >= -5 && value <= 5, "range output out of bounds");
    }

    if (!ok) {
        return 1;
    }

    std::cout << "DeterministicRng tests passed" << '\n';
    return 0;
}
