#include "lut.h"
#include "logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>

LUT::LUT() : isLoaded(false) {}

LUT& LUT::GetInstance() {
    static LUT instance;
    return instance;
}

void LUT::Load() {
    if (isLoaded) return;
    
    table.clear();
    
    // Get DLL directory to load ffb_lut.txt
    char dllPath[MAX_PATH];
    HMODULE hMod = nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCSTR)&LUT::GetInstance, &hMod);
    
    std::string lutFilePath = "ffb_lut.txt";
    if (hMod) {
        GetModuleFileNameA(hMod, dllPath, MAX_PATH);
        std::string pathStr(dllPath);
        size_t lastSlash = pathStr.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            lutFilePath = pathStr.substr(0, lastSlash + 1) + "ffb_lut.txt";
        }
    }
    
    LOG_FMT("[LUT] Attempting to load LUT from: %s", lutFilePath.c_str());
    
    std::ifstream file(lutFilePath);
    if (!file.is_open()) {
        LOG_MSG("[LUT] Warning: ffb_lut.txt not found in DLL directory. Defaulting to linear FFB.");
        isLoaded = true;
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Strip comments starting with '#'
        size_t hashPos = line.find('#');
        if (hashPos != std::string::npos) {
            line = line.substr(0, hashPos);
        }
        
        // Trim leading/trailing spaces
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), line.end());
        
        if (line.empty()) continue;
        
        // Replace separators with space to make parsing easier
        std::replace(line.begin(), line.end(), ',', ' ');
        std::replace(line.begin(), line.end(), '|', ' ');
        std::replace(line.begin(), line.end(), '\t', ' ');
        
        std::stringstream ss(line);
        double inputVal = 0.0;
        double outputVal = 0.0;
        if (ss >> inputVal >> outputVal) {
            table.push_back({inputVal, outputVal});
        }
    }
    
    file.close();
    
    if (table.empty()) {
        LOG_MSG("[LUT] Warning: LUT file was empty. Defaulting to linear FFB.");
        isLoaded = true;
        return;
    }
    
    // Sort table by input value
    std::sort(table.begin(), table.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });
    
    // Ensure bounds [0.0, 1.0]
    if (table.front().first > 0.0) {
        table.insert(table.begin(), {0.0, 0.0});
    }
    if (table.back().first < 1.0) {
        table.push_back({1.0, 1.0});
    }
    
    LOG_FMT("[LUT] Successfully loaded LUT with %d entries.", (int)table.size());
    for (const auto& entry : table) {
        LOG_FMT("[LUT]   In: %.4f -> Out: %.4f", entry.first, entry.second);
    }
    
    isLoaded = true;
}

LONG LUT::Apply(LONG force) {
    if (!isLoaded) {
        Load();
    }
    
    if (table.empty()) {
        return force; // Fallback to linear
    }
    
    // DirectInput forces range from -10000 to 10000.
    // Linearization is symmetric, so we process the magnitude.
    LONG sign = (force < 0) ? -1 : 1;
    double absForce = std::abs(force);
    double ratio = absForce / 10000.0;
    
    // Clamp ratio between 0.0 and 1.0
    if (ratio < 0.0) ratio = 0.0;
    if (ratio > 1.0) ratio = 1.0;
    
    // Find interpolation interval
    double outRatio = ratio;
    for (size_t i = 0; i < table.size() - 1; ++i) {
        if (ratio >= table[i].first && ratio <= table[i + 1].first) {
            double x0 = table[i].first;
            double y0 = table[i].second;
            double x1 = table[i + 1].first;
            double y1 = table[i + 1].second;
            
            double dx = x1 - x0;
            if (dx > 0.0) {
                double t = (ratio - x0) / dx;
                outRatio = y0 + t * (y1 - y0);
            } else {
                outRatio = y0;
            }
            break;
        }
    }
    
    LONG outForce = static_cast<LONG>(outRatio * 10000.0);
    
    // Restore sign and clamp just in case
    LONG finalForce = outForce * sign;
    if (finalForce > 10000) finalForce = 10000;
    if (finalForce < -10000) finalForce = -10000;
    
    return finalForce;
}
