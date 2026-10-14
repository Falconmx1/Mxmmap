/*
 * Mxmmap - Memory Analyzer for Windows
 * Header principal del proyecto.
 */

#ifndef MXMMAP_H
#define MXMMAP_H

#define MXMMAP_NAME     "Mxmmap"
#define MXMMAP_VERSION  "0.1.0"
#define MXMMAP_AUTHOR   "Falconmx1"
#define MXMMAP_YEAR     "2026"

#include <string>
#include <cstdint>

namespace mxmmap {

    inline const char* versionString() {
        return MXMMAP_NAME " v" MXMMAP_VERSION;
    }

    struct ProcessInfo {
        unsigned long pid;
        std::string   name;
    };

    std::string toLower(std::string s);
    std::string formatBytes(std::uint64_t bytes);
    const char* memStateToString(unsigned long state);
    const char* memTypeToString(unsigned long type);
    const char* protectToString(unsigned long prot);

} // namespace mxmmap

#endif // MXMMAP_H
