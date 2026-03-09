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

int cigarros_totales = 0;
int fumador_amonestado = -1;


//
Semaphore mostr_vacio(1); // inicialmente el mostrador está vacio
Semaphore ingr0(0);
Semaphore ingr1(0);
Semaphore ingr2(0);
Semaphore varcigarro(1);
Semaphore sanitaria(0);
Semaphore seguir_fumando(0);


mutex mtx_cout; // Mutex para que no se entrelacen los couts

//-------------------------------------------------------------------------
// Función que simula la acción de producir un ingrediente, como un retardo
// aleatorio de la hebra (devuelve número de ingrediente producido)

int producir_ingrediente()
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_produ( aleatorio<10,100>() );

   // informa de que comienza a producir
   mtx_cout.lock();
   cout << "\t\t\t\t\t\t\t\t\tEstanquero : empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;
   mtx_cout.unlock();
   // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
   this_thread::sleep_for( duracion_produ );

   const int num_ingrediente = aleatorio<0,num_fumadores-1>() ;

   // informa de que ha terminado de producir
   mtx_cout.lock();
   cout << "\t\t\t\t\t\t\t\t\tEstanquero : termina de producir ingrediente " << num_ingrediente << endl;
   mtx_cout.unlock();
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
         mtx_cout.lock();
         cout << "\t\t\t\t\t\t\t\t\tEstanquero : COLOCA INGREDIENTE 0" << endl;
         mtx_cout.unlock();
         sem_signal(ingr0);
      }else if (ingrediente == 1){
         mtx_cout.lock();
         cout << "\t\t\t\t\t\t\t\t\tEstanquero : COLOCA INGREDIENTE  1" << endl;
         mtx_cout.unlock();
         sem_signal(ingr1);
      }else{
         mtx_cout.lock();
         cout << "\t\t\t\t\t\t\t\t\tEstanquero : COLOCA INGREDIENTE  2" << endl;
         mtx_cout.unlock();
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
   int cigarros = 0;
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
      
      //prueba
      if (cigarros == 5){
      	fumador_amonestado = num_fumador;
      	sem_signal(sanitaria);
      	sem_wait(seguir_fumando);
      	cout << "Fumador "<< num_fumador << ": he sido amonestado por la hebra sanitaria." << endl;
        cigarros = 0;
      }
      
      cout << "Fumador " << num_fumador << "  :"
          << " RETIRA INGREDIENTE DEL MOSTRADOR" << endl;
      sem_signal(mostr_vacio);
      fumar(num_fumador);
      cigarros++;
      
      //prueba
      sem_wait(varcigarro);
      cigarros_totales++;
      int N = cigarros_totales;
      sem_signal(varcigarro);
      cout << "Fumador " << num_fumador << " : en total hemos fumado " << N << " cigarros." << endl;
   }
}

//----------------------------------------------------------------------

void  funcion_hebra_sanitaria()
{
   while(true)
   {
   	sem_wait(sanitaria);
	cout << "Hebra sanitaria: el fumador " << fumador_amonestado << " ya ha fumado otros 5 cigarros. Es malo para la salud." << endl;
	sem_signal(seguir_fumando);
	int N = cigarros_totales;
	cout << "Hebra sanitaria: entre todos los fumadores, han fumado " << N << " cigarros." << endl;
   }
}


int main()
{
   // declarar hebras y ponerlas en marcha
   // ......
   thread hebra_estanquero(funcion_hebra_estanquero);
   thread hebra_fumador[num_fumadores];
   thread hebra_sanitaria(funcion_hebra_sanitaria);

   for (int i = 0; i < num_fumadores; ++i)
      hebra_fumador[i] = thread(funcion_hebra_fumador, i);
   // esperar a que terminen las hebras (no pasa nunca)
   for(int i = 0; i < num_fumadores; ++i)
      hebra_fumador[i].join();
   hebra_estanquero.join();
   hebra_sanitaria.join();
   return 0;
}
