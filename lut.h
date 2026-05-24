#ifndef LUT_H
#define LUT_H

#include <vector>
#include <utility>
#include <string>
#include <windows.h>

class LUT {
public:
    static LUT& GetInstance();
    void Load();
    LONG Apply(LONG force);

private:
    LUT();
    ~LUT() = default;

    std::vector<std::pair<double, double>> table;
    bool isLoaded;
};

#endif // LUT_H
