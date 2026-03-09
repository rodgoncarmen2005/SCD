// SCD
// Implementación del enunciado de la variante de los fumadores del ensayo de examen de la P1

#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "scd.h"

using namespace std;
using namespace scd;

// Número de fumadores
const int num_fumadores = 3;

// semáforo para esperar a que el mostrador quede vacío.
Semaphore mostr_vacio = 1;

// Vector de semáforos para esperar que esté cada ingrediente disponibles
// (se inicializan a 0 en 'main')
std::vector<Semaphore> ingr_disp;

//-------------------------------------------------------------------------

int producir_ingrediente()
{
    // calcular milisegundos aleatorios de duración de la acción de fumar)
    chrono::milliseconds duracion_produ(aleatorio<10, 90>());

    // informa de que comienza a producir
    cout << "\t\t\t\t\t\t\t\t\tEstanquero : empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;

    // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
    this_thread::sleep_for(duracion_produ);

    const int num_ingrediente = aleatorio<0, num_fumadores - 1>();

    // informa de que ha terminado de producir
    cout << "\t\t\t\t\t\t\t\t\tEstanquero : termina de producir ingrediente " << num_ingrediente << endl;

    return num_ingrediente;
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero()
{
    int i;

    while (true)
    {
        i = producir_ingrediente();

        sem_wait(mostr_vacio);
        cout << "\t\t\t\t\t\t\t\t\tEstanquero: puesto ingrediente " << i << " en el mostrador" << endl;
        sem_signal(ingr_disp[i]);
    }
}

// ------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar(int num_fumador)
{
    // calcular milisegundos aleatorios de duración de la acción de fumar)
    chrono::milliseconds duracion_fumar(aleatorio<100, 300>());

    // informa de que comienza a fumar
    cout << "Fumador " << num_fumador << ": "
         << "empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

    // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
    this_thread::sleep_for(duracion_fumar);

    // informa de que ha terminado de fumar
    cout << "Fumador " << num_fumador << ": termina de fumar, comienza espera de ingrediente " << num_fumador << endl;
}

// ---------------------------------------------------------------------
// Función que ejecuta la hebra del fumador

void funcion_hebra_fumador(int num_fumador)
{
    while (true)
    {
        // Esperar a su ingrediente
        sem_wait(ingr_disp[num_fumador]);

        // Retirar el ingrediente del mostrador y fumar (igual que antes)
        cout << "Fumador " << num_fumador << ": retirando ingrediente" << endl;
        sem_signal(mostr_vacio);
        fumar(num_fumador);
    }
}

//----------------------------------------------------------------------

int main()
{
    cout << "-----------------------------------------------------------------" << endl
         << "Fumadores                                                        " << endl
         << "-----------------------------------------------------------------" << endl
         << flush;

    // Inicializar el vector de semáforos y los contadores
    for (int i = 0; i < num_fumadores; i++)
    {
        ingr_disp.push_back(Semaphore(0));
    }

    // Declaración de hebras
    thread hebra_estanquero(funcion_hebra_estanquero),
        hebras_fumadoras[num_fumadores];

    // Poner en marcha las hebras fumadoras
    for (int i = 0; i < num_fumadores; i++)
        hebras_fumadoras[i] = thread(funcion_hebra_fumador, i);

    // Esperar a que terminan las hebras
    hebra_estanquero.join();
    for (int i = 0; i < num_fumadores; i++)
        hebras_fumadoras[i].join();
}
