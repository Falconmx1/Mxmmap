/*
 * Mxmmap - Memory Analyzer for Windows
 *
 * Uso:
 *   Mxmmap.exe --list
 *   Mxmmap.exe --pid <PID>
 *   Mxmmap.exe --name <nombre.exe>
 *   Mxmmap.exe --modules <PID>
 *   Mxmmap.exe --export <PID> <archivo.txt>
 */

#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>

#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cctype>

#include "mxmmap.h"

#pragma comment(lib, "psapi.lib")

// ---------- Implementacion de utilidades del namespace ----------

namespace mxmmap {

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string formatBytes(std::uint64_t bytes) {
    const char* units[] = { "B", "KB", "MB", "GB", "TB" };
    double value = static_cast<double>(bytes);
    int idx = 0;
    while (value >= 1024.0 && idx < 4) {
        value /= 1024.0;
        ++idx;
    }
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.2f %s", value, units[idx]);
    return std::string(buffer);
}

const char* memStateToString(unsigned long state) {
    switch (state) {
        case MEM_COMMIT:  return "COMMIT";
        case MEM_RESERVE: return "RESERVE";
        case MEM_FREE:    return "FREE";
        default:          return "UNKNOWN";
    }
}

const char* memTypeToString(unsigned long type) {
    switch (type) {
        case MEM_IMAGE:   return "IMAGE";
        case MEM_MAPPED:  return "MAPPED";
        case MEM_PRIVATE: return "PRIVATE";
        default:          return "-";
    }
}

const char* protectToString(unsigned long prot) {
    switch (prot) {
        case PAGE_NOACCESS:          return "NOACCESS";
        case PAGE_READONLY:          return "R";
        case PAGE_READWRITE:         return "RW";
        case PAGE_WRITECOPY:         return "WC";
        case PAGE_EXECUTE:           return "X";
        case PAGE_EXECUTE_READ:      return "XR";
        case PAGE_EXECUTE_READWRITE: return "XRW";
        case PAGE_EXECUTE_WRITECOPY: return "XWC";
        case PAGE_GUARD:             return "GUARD";
        case PAGE_NOCACHE:           return "NOCACHE";
        default:                     return "?";
    }
}

} // namespace mxmmap

using namespace mxmmap;

// ---------- Banner ----------

static void printBanner() {
    std::cout << "=========================================\n";
    std::cout << "     Mxmmap v" << MXMMAP_VERSION << " - Memory Analyzer\n";
    std::cout << "=========================================\n\n";
}

// ---------- Procesos ----------

static std::vector<ProcessInfo> snapshotProcesses() {
    std::vector<ProcessInfo> list;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return list;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(snap, &pe)) {
        do {
            ProcessInfo info;
            info.pid  = pe.th32ProcessID;
            info.name = pe.szExeFile;
            list.push_back(info);
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);
    return list;
}

static bool enableDebugPrivilege() {
    HANDLE hToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return false;
    }

    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!LookupPrivilegeValueA(nullptr, SE_DEBUG_NAME, &tp.Privileges[0].Luid)) {
        CloseHandle(hToken);
        return false;
    }

    BOOL ok  = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr);
    DWORD err = GetLastError();
    CloseHandle(hToken);
    return ok && err == ERROR_SUCCESS;
}

// ---------- Analisis de un proceso ----------

static void analyzeProcess(DWORD pid, const std::string& exportFile = "") {
    HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                               FALSE, pid);
    if (!hProc) {
        std::cerr << "[!] No se pudo abrir PID " << pid
                  << " (error " << GetLastError() << "). "
                  << "Ejecuta como administrador.\n";
        return;
    }

    std::ofstream out;
    if (!exportFile.empty()) {
        out.open(exportFile, std::ios::out | std::ios::trunc);
        if (!out.is_open()) {
            std::cerr << "[!] No se pudo abrir " << exportFile << " para escritura.\n";
        }
    }

    auto emit = [&](const std::string& line) {
        std::cout << line << "\n";
        if (out.is_open()) out << line << "\n";
    };

    char exeName[MAX_PATH] = { 0 };
    DWORD nameSize = MAX_PATH;
    std::string procName = "<desconocido>";
    if (QueryFullProcessImageNameA(hProc, 0, exeName, &nameSize)) {
        procName = exeName;
    }

    emit("-----------------------------------------");
    emit("Proceso : " + procName);
    emit("PID     : " + std::to_string(pid));
    emit("-----------------------------------------");

    PROCESS_MEMORY_COUNTERS_EX pmc = {};
    pmc.cb = sizeof(pmc);
    if (GetProcessMemoryInfo(hProc,
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
                             sizeof(pmc))) {
        emit("Working Set     : " + formatBytes(pmc.WorkingSetSize));
        emit("Peak Working Set: " + formatBytes(pmc.PeakWorkingSetSize));
        emit("Private Usage   : " + formatBytes(pmc.PrivateUsage));
        emit("Pagefile Usage  : " + formatBytes(pmc.PagefileUsage));
    } else {
        emit("[!] No se pudo obtener informacion de memoria.");
    }

    MEMORY_BASIC_INFORMATION mbi;
    unsigned char* addr = nullptr;
    SIZE_T totalCommitted = 0;
    SIZE_T totalReserved  = 0;
    SIZE_T totalFree      = 0;
    size_t regionCount    = 0;

    emit("");
    emit("Regiones de memoria:");

    while (VirtualQueryEx(hProc, addr, &mbi, sizeof(mbi)) == sizeof(mbi)) {
        if (mbi.State == MEM_COMMIT)  totalCommitted += mbi.RegionSize;
        if (mbi.State == MEM_RESERVE) totalReserved  += mbi.RegionSize;
        if (mbi.State == MEM_FREE)    totalFree      += mbi.RegionSize;
        ++regionCount;

        if (regionCount <= 20) {
            char line[256];
            std::snprintf(line, sizeof(line),
                          "  [%p - %p] %-8s %-8s %-6s %s",
                          mbi.BaseAddress,
                          static_cast<unsigned char*>(mbi.BaseAddress) + mbi.RegionSize,
                          memStateToString(mbi.State),
                          memTypeToString(mbi.Type),
                          protectToString(mbi.Protect),
                          formatBytes(mbi.RegionSize).c_str());
            emit(line);
        }

        unsigned char* next = static_cast<unsigned char*>(mbi.BaseAddress) + mbi.RegionSize;
        if (next <= addr) break;
        addr = next;

        if (reinterpret_cast<uintptr_t>(addr) >= 0x7FFFFFFFFFFFULL) break;
    }

    if (regionCount > 20) {
        emit("  ... (" + std::to_string(regionCount - 20) + " regiones mas)");
    }

    emit("");
    emit("Resumen:");
    emit("  Regiones totales : " + std::to_string(regionCount));
    emit("  Committed        : " + formatBytes(totalCommitted));
    emit("  Reserved         : " + formatBytes(totalReserved));
    emit("  Free             : " + formatBytes(totalFree));
    emit("");

    CloseHandle(hProc);
    if (out.is_open()) {
        out.close();
        std::cout << "[+] Reporte exportado a: " << exportFile << "\n";
    }
}

// ---------- Modulos cargados ----------

static void listModules(DWORD pid) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE) {
        std::cerr << "[!] No se pudo enumerar modulos para PID " << pid
                  << " (error " << GetLastError() << ").\n";
        return;
    }

    MODULEENTRY32 me;
    me.dwSize = sizeof(MODULEENTRY32);

    std::cout << "Modulos del PID " << pid << ":\n\n";
    std::cout << std::left
              << std::setw(20) << "Base"
              << std::setw(12) << "Tamano"
              << "Modulo\n";
    std::cout << std::string(70, '-') << "\n";

    if (Module32First(snap, &me)) {
        do {
            std::cout << std::left
                      << std::setw(20) << me.modBaseAddr
                      << std::setw(12) << formatBytes(me.modBaseSize)
                      << me.szModule << "\n";
        } while (Module32Next(snap, &me));
    }

    CloseHandle(snap);
}

// ---------- Helpers ----------

static DWORD findPidByName(const std::string& name) {
    std::string target = toLower(name);
    auto procs = snapshotProcesses();
    for (const auto& p : procs) {
        if (toLower(p.name) == target) return p.pid;
    }
    return 0;
}

static void printUsage() {
    std::cout << "Uso:\n";
    std::cout << "  Mxmmap --list                     Lista todos los procesos\n";
    std::cout << "  Mxmmap --pid <PID>                Analiza un proceso por PID\n";
    std::cout << "  Mxmmap --name <proceso.exe>       Analiza un proceso por nombre\n";
    std::cout << "  Mxmmap --modules <PID>            Lista modulos de un proceso\n";
    std::cout << "  Mxmmap --export <PID> <archivo>   Exporta reporte a un archivo\n";
    std::cout << "  Mxmmap --help                     Muestra esta ayuda\n\n";
}

// ---------- Main ----------

int main(int argc, char* argv[]) {
    printBanner();

    if (argc < 2) {
        printUsage();
        return 0;
    }

    enableDebugPrivilege();

    std::string cmd = toLower(argv[1]);

    if (cmd == "--help" || cmd == "-h") {
        printUsage();
        return 0;
    }

    if (cmd == "--list") {
        auto procs = snapshotProcesses();
        std::cout << "Procesos activos: " << procs.size() << "\n\n";
        std::cout << std::left
                  << std::setw(10) << "PID"
                  << "Nombre\n";
        std::cout << std::string(60, '-') << "\n";
        for (const auto& p : procs) {
            std::cout << std::left
                      << std::setw(10) << p.pid
                      << p.name << "\n";
        }
        return 0;
    }

    if (cmd == "--pid" && argc >= 3) {
        DWORD pid = static_cast<DWORD>(std::stoul(argv[2]));
        analyzeProcess(pid);
        return 0;
    }

    if (cmd == "--name" && argc >= 3) {
        DWORD pid = findPidByName(argv[2]);
        if (pid == 0) {
            std::cerr << "[!] No se encontro el proceso: " << argv[2] << "\n";
            return 1;
        }
        analyzeProcess(pid);
        return 0;
    }

    if (cmd == "--modules" && argc >= 3) {
        DWORD pid = static_cast<DWORD>(std::stoul(argv[2]));
        listModules(pid);
        return 0;
    }

    if (cmd == "--export" && argc >= 4) {
        DWORD pid = static_cast<DWORD>(std::stoul(argv[2]));
        analyzeProcess(pid, argv[3]);
        return 0;
    }

    std::cerr << "[!] Argumento no valido: " << argv[1] << "\n\n";
    printUsage();
    return 1;
}
