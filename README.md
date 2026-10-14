# Mxmmap

Herramienta ligera en C++ para analizar, mapear y visualizar el uso de memoria RAM en Windows.

## ✨ Características

- Listado de procesos activos y su consumo de memoria
- Mapeo de regiones de memoria vía `VirtualQueryEx`
- Inspección de módulos cargados por proceso
- Resumen de memoria: Working Set, Private Usage, Pagefile
- Exportación de reportes a archivo de texto
- Interfaz CLI rápida y minimalista

## 🛠️ Requisitos

- Windows 10/11
- Compilador MSVC 2019+ o MinGW-w64 con soporte C++17
- CMake 3.15+
- Privilegios de administrador (recomendado para procesos del sistema)

## 🚀 Instalación

```bash
git clone https://github.com/Falconmx1/Mxmmap.git
cd Mxmmap
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

El binario se generará en `build/Release/Mxmmap.exe` (MSVC) o `build/Mxmmap.exe` (MinGW).

### Compilación directa sin CMake

**MSVC:**

```bash
cl /std:c++17 /EHsc /O2 /Iinclude src\main.cpp /Fe:Mxmmap.exe
```

**MinGW / g++:**

```bash
g++ -std=c++17 -O2 -Iinclude src/main.cpp -o Mxmmap.exe -lpsapi
```

## 📖 Uso

```bash
Mxmmap.exe --list                     # Lista todos los procesos
Mxmmap.exe --pid <PID>                # Analiza un proceso por PID
Mxmmap.exe --name <proceso.exe>       # Analiza un proceso por nombre
Mxmmap.exe --modules <PID>            # Lista módulos de un proceso
Mxmmap.exe --export <PID> <archivo>   # Exporta reporte a archivo
Mxmmap.exe --help                     # Muestra la ayuda
```

### Ejemplos

```bash
Mxmmap.exe --list
Mxmmap.exe --pid 4
Mxmmap.exe --name explorer.exe
Mxmmap.exe --modules 1234
Mxmmap.exe --export 1234 reporte.txt
```

## 📂 Estructura del proyecto

```
Mxmmap/
├── include/
│   └── mxmmap.h
├── src/
│   └── main.cpp
├── CMakeLists.txt
├── LICENSE
└── README.md
```

## 🤝 Contribuir

Los pull requests son bienvenidos. Para cambios grandes, abre primero un issue para discutir qué te gustaría cambiar.

## 📜 Licencia

MIT © 2026 Falconmx1
