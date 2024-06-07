#pragma once
#include <string>
#include <array>

struct Student {
    std::string name;
    long number;
    long grade;
    std::array<std::string, 3> hobby;
};
