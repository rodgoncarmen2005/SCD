// fumadores.cpp

#include <iostream>
#include <iomanip>
#include <cassert>
#include <random>
#include <thread>
#include "scd.h"

using namespace std;
using namespace scd;
//**********************************************************************

constexpr int
    min_ms = 5,  // tiempo minimo de espera en sleep_for
    max_ms = 20; // tiempo máximo de espera en sleep_for


const int num_fumadores = 3;

//*********************************************************************

int producir_ingrediente()
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_produ(aleatorio<10, 100>());

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

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar(int num_fumador)
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar(aleatorio<20, 200>());

   // informa de que comienza a fumar

   cout << "Fumador " << num_fumador << "  :"
        << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for(duracion_fumar);

   // informa de que ha terminado de fumar

   cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
}
//----------------------------------------------------------------------

// *****************************************************************************
// clase para monitor Estanco, version SU, semántica SC

class Estanco : public HoareMonitor
{
private:
   const int mostrador_vacio = -1;
   int mostrador = mostrador_vacio;
   CondVar ingrediente_disponible[num_fumadores];
   CondVar mostrador_ocupado;

public:                                      // constructor y métodos públicos
   Estanco();                                // constructor
   void ponerIngrediente(int ingrediente);   // poner ingrediente en el mostrador
   void obtenerIngrediente(int num_fumador); // leer ingrediente del mostrador
   void esperarRecogidaIngrediente();
};
// -----------------------------------------------------------------------------

Estanco::Estanco()
{
   // inicializar las variables de condición
   for (int i = 0; i < num_fumadores; i++)
      ingrediente_disponible[i] = newCondVar();
   mostrador_ocupado = newCondVar();
}
// -----------------------------------------------------------------------------

// función para que el estanquero ponga un ingrediente en el mostrador
void Estanco::ponerIngrediente(int ingrediente)
{
   // poner el ingrediente en el mostrador
   cout << "\t\t\t\t\t\t\t\t\tEstanquero : COLOCA INGREDIENTE " << ingrediente << endl;

   // colocar el ingrediente en el mostrador
   mostrador = ingrediente;
	
   // señalar al fumador que el ingrediente está disponible
   ingrediente_disponible[ingrediente].signal();


}
// -----------------------------------------------------------------------------
// función para que el fumador obtenga su ingrediente del mostrador
void Estanco::obtenerIngrediente(int num_fumador)
{
   // esperar a que el ingrediente que corresponde a cada fumador esté disponible
   if (mostrador != num_fumador)
      ingrediente_disponible[num_fumador].wait();

   // indicamos que el mostrador está libre
   mostrador = mostrador_vacio;

   // señalamos al estanquero que el mostrador está libre
   mostrador_ocupado.signal();

   // informa de que ha recogido el ingrediente
   cout << "Fumador " << num_fumador << "  : RECOGE su ingrediente del mostrador." << endl;
}

// -----------------------------------------------------------------------------
// función para que el estanquero espere a que el fumador recoja el ingrediente
void Estanco::esperarRecogidaIngrediente()
{
   // esperar a que el fumador recoja el ingrediente
   if(mostrador != mostrador_vacio)
      mostrador_ocupado.wait();
}
// *****************************************************************************
// funciones de hebras

void funcion_hebra_fumadora(MRef<Estanco> monitor, int num_fumador)
{
   while (true)
   {
      monitor->obtenerIngrediente(num_fumador);
      fumar(num_fumador);
   }
}

// -----------------------------------------------------------------------------

void funcion_hebra_estanquero(MRef<Estanco> monitor)
{
   int ingrediente = -1;
   while (true)
   {
      ingrediente = producir_ingrediente();
      monitor->ponerIngrediente(ingrediente);
      monitor->esperarRecogidaIngrediente();
      

   }
}
// -----------------------------------------------------------------------------

int main()
{
   cout << "--------------------------------------------------------------------" << endl
        << "Problema de los fumadores con monitor SU. " << endl
        << "--------------------------------------------------------------------" << endl
        << flush;

   MRef<Estanco> monitor = Create<Estanco>();

   thread hebra_estanquero(funcion_hebra_estanquero, monitor);
   thread hebra_fumador[num_fumadores];

   for (int i = 0; i < num_fumadores; ++i)
      hebra_fumador[i] = thread(funcion_hebra_fumadora, monitor, i);

   // Esperamos a que terminen las hebras (no lo harán nunca en este caso)
   hebra_estanquero.join();
   for (int i = 0; i < num_fumadores; ++i)
      hebra_fumador[i].join();

   return 0;
}
