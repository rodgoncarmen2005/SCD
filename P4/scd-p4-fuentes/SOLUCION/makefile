# Configuración del compilador y flags
CXX      := g++
CXXFLAGS := -std=c++11 -Wall -Wextra -g
LDFLAGS  := -lpthread

# Obtener todos los archivos .cpp de la carpeta actual
SOURCES  := $(wildcard *.cpp)

# Definir los nombres de los ejecutables (sin la extensión .cpp)
TARGETS  := $(SOURCES:.cpp=)

# Regla por defecto: compilar todos los ejecutables
all: $(TARGETS)

# Regla para compilar cada ejecutable individualmente
%: %.cpp
	@echo "Compilando $<..."
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

# Regla para limpiar los archivos ejecutables generados
clean:
	@echo "Limpiando ejecutables..."
	rm -f $(TARGETS)

# Evitar conflictos con archivos que se llamen como las reglas
.PHONY: all clean
