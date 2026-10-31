#include "prx/libSceIcu/libSceIcu.hpp"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <source_location>
#include <stdexcept>

namespace {

constexpr std::int32_t NotTerminated = -124;
constexpr std::int32_t Overflow = 15;
constexpr std::int32_t Minimum = std::numeric_limits<std::int32_t>::min();
constexpr std::int32_t Maximum = std::numeric_limits<std::int32_t>::max();

void Require(bool condition, std::source_location location = std::source_location::current()) {
    if (condition) return;
    std::fprintf(stderr, "ICU buffer helper check failed at line %u\n", location.line());
    std::abort();
}

template<typename TAction>
void RequireInvalidArgument(TAction action, std::source_location location = std::source_location::current()) {
    try { action(); } catch (const std::invalid_argument&) { return; }
    Require(false, location);
}

enum class Outcome { Write, Warning, Failure, Unchanged };

struct TerminationCase {
    std::int32_t capacity;
    std::int32_t length;
    Outcome outcome;
};

constexpr TerminationCase terminationCases[] {
    {4, 0, Outcome::Write}, {4, 1, Outcome::Write}, {4, 3, Outcome::Write},
    {1, 0, Outcome::Write}, {Maximum, 3, Outcome::Write},
    {0, 0, Outcome::Warning}, {3, 3, Outcome::Warning}, {1, 1, Outcome::Warning},
    {Maximum, Maximum, Outcome::Warning},
    {0, 1, Outcome::Failure}, {2, 3, Outcome::Failure}, {-1, 0, Outcome::Failure},
    {Minimum, 0, Outcome::Failure}, {0, Maximum, Outcome::Failure},
    {-1, -1, Outcome::Unchanged}, {0, -1, Outcome::Unchanged},
    {Maximum, Minimum, Outcome::Unchanged}, {Minimum, Minimum, Outcome::Unchanged},
    {-2, -1, Outcome::Unchanged}
};

constexpr std::int32_t statuses[] {0, NotTerminated, -128, -1, Minimum, 1, Overflow, Maximum};

std::int32_t ExpectedStatus(std::int32_t status, Outcome outcome) {
    if (status > 0) return status;
    switch (outcome) {
    case Outcome::Write: return status == NotTerminated ? 0 : status;
    case Outcome::Warning: return NotTerminated;
    case Outcome::Failure: return Overflow;
    case Outcome::Unchanged: return status;
    }
    std::abort();
}

template<typename T, auto TTerminate>
void Termination() {
    for (const auto& test : terminationCases) {
        for (const auto initial : statuses) {
            std::array<T, 12> buffer;
            for (std::size_t index = 0; index < buffer.size(); ++index)
                buffer[index] = static_cast<T>(0x41 + index);
            auto expected = buffer;
            if (initial <= 0 && test.outcome == Outcome::Write)
                expected[4 + static_cast<std::size_t>(test.length)] = 0;
            struct { std::int32_t before, value, after; } status {0x12345678, initial, 0x23456789};
            Require(TTerminate(buffer.data() + 4, test.capacity, test.length, &status.value) == test.length);
            Require(buffer == expected);
            Require(status.value == ExpectedStatus(initial, test.outcome));
            Require(status.before == 0x12345678 && status.after == 0x23456789);

            const auto before = buffer;
            Require(TTerminate(buffer.data() + 4, test.capacity, test.length, nullptr) == test.length);
            Require(buffer == before);

            status.value = initial;
            if (initial <= 0 && test.outcome == Outcome::Write) {
                RequireInvalidArgument([&] { TTerminate(nullptr, test.capacity, test.length, &status.value); });
                Require(status.value == initial);
            } else {
                Require(TTerminate(nullptr, test.capacity, test.length, &status.value) == test.length);
                Require(status.value == ExpectedStatus(initial, test.outcome));
            }
            Require(status.before == 0x12345678 && status.after == 0x23456789);
        }
        Require(TTerminate(nullptr, test.capacity, test.length, nullptr) == test.length);
    }
}

bool Invariant(std::uint32_t value) {
    if (value > 0x7f) return false;
    switch (value) {
    case 0x0a: case 0x21: case 0x23: case 0x24: case 0x40: case 0x5b: case 0x5c:
    case 0x5d: case 0x5e: case 0x60: case 0x7b: case 0x7c: case 0x7d: case 0x7e:
        return false;
    default:
        return true;
    }
}

template<typename T, auto TCheck>
void InvariantAlphabet(std::uint32_t limit) {
    for (std::uint32_t value = 0; value < limit; ++value) {
        std::array<T, 5> text {static_cast<T>('a'), static_cast<T>(value), static_cast<T>('z'), 0, static_cast<T>(0x21)};
        const auto before = text;
        const auto expected = Invariant(value) ? 1 : 0;
        Require(TCheck(text.data() + 1, 1) == expected);
        Require(TCheck(text.data(), 3) == expected);
        Require(TCheck(text.data(), 0) == 1);
        Require(TCheck(text.data(), 1) == 1);
        for (const auto length : {-1, -2, Minimum})
            Require(TCheck(text.data(), length) == expected);
        Require(text == before);
    }
}

template<typename T, auto TCheck>
void InvariantLengths() {
    Require(TCheck(nullptr, 0) == 1);
    for (const auto length : {1, Maximum, -1, -2, Minimum})
        RequireInvalidArgument([&] { TCheck(nullptr, length); });

    const std::array<T, 5> embedded {static_cast<T>('a'), 0, static_cast<T>('z'), static_cast<T>(0x21), 0};
    Require(TCheck(embedded.data(), 2) == 1);
    Require(TCheck(embedded.data(), 3) == 1);
    Require(TCheck(embedded.data(), 4) == 0);
    for (const auto length : {-1, -2, Minimum}) Require(TCheck(embedded.data(), length) == 1);
    const T nul = 0;
    Require(TCheck(&nul, 1) == 1);
    for (const auto length : {-1, -2, Minimum}) Require(TCheck(&nul, length) == 1);
    const T variant = static_cast<T>(0x21);
    for (const auto length : {1, Maximum, -1, -2, Minimum}) Require(TCheck(&variant, length) == 0);

    for (std::size_t position = 0; position < 5; ++position) {
        std::array<T, 6> text;
        text.fill(static_cast<T>('a'));
        text.back() = 0;
        text[position] = static_cast<T>(0x21);
        const auto before = text;
        Require(TCheck(text.data(), 5) == 0);
        Require(TCheck(text.data(), static_cast<std::int32_t>(position)) == 1);
        Require(TCheck(text.data(), static_cast<std::int32_t>(position + 1)) == 0);
        Require(TCheck(text.data(), -1) == 0);
        Require(text == before);
    }
}

}

int main() {
    Termination<char, u_terminateChars_nid_postfix>();
    Termination<char16_t, u_terminateUChars_nid_postfix>();
    Termination<std::int32_t, u_terminateUChar32s_nid_postfix>();
    InvariantAlphabet<char, uprv_isInvariantString_nid_postfix>(256);
    InvariantAlphabet<char16_t, uprv_isInvariantUString_nid_postfix>(65536);
    InvariantLengths<char, uprv_isInvariantString_nid_postfix>();
    InvariantLengths<char16_t, uprv_isInvariantUString_nid_postfix>();
    std::puts("ICU buffer helper tests passed");
    return 0;
}
