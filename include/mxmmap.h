/*
 * Mxmmap - Memory Analyzer for Windows
 * Header principal del proyecto.
 *
 * Este archivo define metadatos, versiones, constantes y firmas
 * de utilidades compartidas. Todo lo que necesites reutilizar
 * entre archivos .cpp va aquí.
 */

#ifndef MXMMAP_H
#define MXMMAP_H

// ---------- Metadatos del proyecto ----------

#define MXMMAP_NAME     "Mxmmap"
#define MXMMAP_VERSION  "0.1.0"
#define MXMMAP_AUTHOR   "Falconmx1"
#define MXMMAP_YEAR     "2026"

// ---------- Includes comunes ----------

#include <string>
#include <vector>
#include <cstdint>

// ---------- Namespace ----------

namespace mxmmap {

    // Version como cadena completa
    inline const char* versionString() {
        return MXMMAP_NAME " v" MXMMAP_VERSION;
    }

    // ---------- Estructura de proceso ----------

    struct ProcessInfo {
        unsigned long pid;
        std::string   name;
    };

    // ---------- Utilidades declaradas (implementadas en main.cpp) ----------

    // Convierte una cadena a minusculas (seguro con caracteres no ASCII)
    std::string toLower(std::string s);

    // Formatea bytes en algo legible: "1.23 MB", "512.00 KB", etc.
    std::string formatBytes(std::uint64_t bytes);

    // Traduce el estado de una region de memoria (MEM_COMMIT, MEM_RESERVE, MEM_FREE)
    const char* memStateToString(unsigned long state);

    // Traduce el tipo de una region de memoria (MEM_IMAGE, MEM_MAPPED, MEM_PRIVATE)
    const char* memTypeToString(unsigned long type);

    // Traduce el nivel de proteccion (PAGE_READONLY, PAGE_EXECUTE_READ, etc.)
    const char* protectToString(unsigned long prot);

} // namespace mxmmap

#endif // MXMMAP_H
