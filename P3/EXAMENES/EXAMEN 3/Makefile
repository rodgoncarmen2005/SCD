# Define el compilador y las banderas
CXX = mpicxx
CXXFLAGS = -std=c++11

# Obtener todos los archivos fuente (.cpp) en el directorio actual
SOURCES = $(wildcard *.cpp)

# Obtener los nombres de los ejecutables reemplazando la extensión .cpp con nada
# Esto asume que cada archivo .cpp genera un ejecutable separado.
EXECUTABLES = $(patsubst %.cpp,%,$(SOURCES))

.PHONY: all clean

# Objetivo principal: compilar todos los ejecutables
all: $(EXECUTABLES)

# Regla genérica para compilar cualquier archivo .cpp
# $< es el primer prerrequisito (el archivo .cpp)
# $@ es el nombre del objetivo (el nombre del ejecutable)
%: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

# Regla de limpieza para remover todos los ejecutables y archivos temporales
clean:
	rm -f $(EXECUTABLES) *.o
