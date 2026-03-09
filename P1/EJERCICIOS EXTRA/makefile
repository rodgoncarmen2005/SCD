# Compilador y opciones
CXX = g++
CXXFLAGS = -std=c++11 -pthread -I.

# Archivos fuente
COMMON = scd.cpp                   # Archivos comunes (añade aquí más si los hay)
MAINS = $(filter-out $(COMMON), $(wildcard *.cpp))   # Todos los .cpp excepto los comunes

# Nombres de ejecutables (uno por cada archivo principal)
EXECS = $(MAINS:.cpp=)

# Regla por defecto: compilar todos los ejecutables
all: $(EXECS)

# Regla genérica: cada ejecutable depende de su .cpp + los comunes
%: %.cpp $(COMMON)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo ">> Compilado ejecutable: $@"

# Limpiar ejecutables y objetos intermedios
clean:
	rm -f $(EXECS) *.o
	@echo ">> Limpieza completa"

