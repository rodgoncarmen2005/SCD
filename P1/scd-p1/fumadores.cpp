#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "scd.h"

using namespace std ;
using namespace scd ;

// numero de fumadores 

const int num_fumadores = 3 ;
const int num_iteraciones = 10 ;

//
Semaphore mostr_vacio = 1; // inicialmente el mostrador está vacio
Semaphore ingr0 = 0;
Semaphore ingr1 = 0;
Semaphore ingr2 = 0;


//-------------------------------------------------------------------------
// Función que simula la acción de producir un ingrediente, como un retardo
// aleatorio de la hebra (devuelve número de ingrediente producido)

int producir_ingrediente()
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_produ( aleatorio<10,100>() );

   // informa de que comienza a producir
   cout << "\t\t\t\t\t\t\t\t\tEstanquero : empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;
   // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
   this_thread::sleep_for( duracion_produ );

   const int num_ingrediente = aleatorio<0,num_fumadores-1>() ;

   // informa de que ha terminado de producir
   cout << "\t\t\t\t\t\t\t\t\tEstanquero : termina de producir ingrediente " << num_ingrediente << endl;
   return num_ingrediente ;
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(  )
{
   int ingrediente = -1;
   //for ( unsigned i = 0 ; i < num_iteraciones ; i++ )
   while (true)
   {
      ingrediente = producir_ingrediente( ) ;
      sem_wait(mostr_vacio);
      // poner el ingrediente en el mostrador
      if (ingrediente == 0){
         cout << "\t\t\t\t\t\t\t\t\tEstanquero : COLOCA INGREDIENTE 0" << endl;
         sem_signal(ingr0);
      }else if (ingrediente == 1){
         cout << "\t\t\t\t\t\t\t\t\tEstanquero : COLOCA INGREDIENTE  1" << endl;
         sem_signal(ingr1);
      }else{
         cout << "\t\t\t\t\t\t\t\t\tEstanquero : COLOCA INGREDIENTE  2" << endl;
         sem_signal(ingr2);;
      }

   }
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar

    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar

    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;

}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador )
{
   while(true)
   {
      if (num_fumador == 0){
         sem_wait(ingr0);
      }
      else if (num_fumador == 1){
         sem_wait(ingr1);
      }
      else{
         sem_wait(ingr2);
      }
      cout << "Fumador " << num_fumador << "  :"
          << " RETIRA INGREDIENTE DEL MOSTRADOR" << endl;
      sem_signal(mostr_vacio);
      fumar(num_fumador);
   }
}

//----------------------------------------------------------------------

int main()
{
   // declarar hebras y ponerlas en marcha
   // ......
   thread hebra_estanquero(funcion_hebra_estanquero);
   thread hebra_fumador[num_fumadores];

   for (int i = 0; i < num_fumadores; ++i)
      hebra_fumador[i] = thread(funcion_hebra_fumador, i);
   // esperar a que terminen las hebras (no pasa nunca)
   for(int i = 0; i < num_fumadores; ++i)
      hebra_fumador[i].join();
   hebra_estanquero.join();
   return 0;
}
