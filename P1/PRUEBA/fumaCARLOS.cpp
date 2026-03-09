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

Semaphore fumador[num_fumadores]={0,0,0};
Semaphore mostrador(1);
Semaphore sem_total_fumados(1);
Semaphore sem_sanitaria(0);
Semaphore sem_fum_san(0);

int total_fumados=0;
int fumador_amonestado;

//-------------------------------------------------------------------------
// Función que simula la acción de producir un ingrediente, como un retardo
// aleatorio de la hebra (devuelve número de ingrediente producido)

int producir_ingrediente()
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_produ( aleatorio<10,100>() );

   // informa de que comienza a producir
   cout << "Estanquero : empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
   this_thread::sleep_for( duracion_produ );

   const int num_ingrediente = aleatorio<0,num_fumadores-1>() ;

   // informa de que ha terminado de producir
   cout << "Estanquero : termina de producir ingrediente " << num_ingrediente << endl;

   return num_ingrediente ;
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(  )
{
   while (true) {
     // sem_wait(estanquero);

      int ingrediente = producir_ingrediente();
      sem_wait(mostrador);
      cout << "Estanquero : pone el ingrediente " << ingrediente << " en el mostrador." << endl;
      sem_signal(fumador[ingrediente]);
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

   int num_fum=0;
   while( true )
   {
      sem_wait(fumador[num_fumador]);
      //nuevo
      if (num_fum==5){
         fumador_amonestado = num_fumador;
         sem_signal(sem_sanitaria);
         sem_wait(sem_fum_san);
         cout << "Fumador " << num_fumador << ": he sido amonestado por la hebra sanitaria" << endl;
         num_fum = num_fum%5;
      }

      cout << "Fumador " << num_fumador << "  : retira el ingrediente del mostrador." << endl;
      sem_signal(mostrador);
      fumar( num_fumador );

      //nuevo:
      num_fum++;
      sem_wait(sem_total_fumados);
      total_fumados++;
      int N = total_fumados;
      sem_signal(sem_total_fumados);
      cout << "Fumador "<< num_fumador << ": entre todos los fumadores ya hemos fumado "<<N<<" cigarros" << endl;
   }
}

//----------------------------------------------------------------------


void funcion_hebra_sanitaria(){
    while (true) {
      sem_wait(sem_sanitaria);
      cout << "Hebra sanitaria: el fumador "<< fumador_amonestado <<" ya ha fumado otros 5 cigarros. Es malo para la salud." << endl;
      sem_signal(sem_fum_san);
      cout << "Hebra sanitaria: entre todos los fumadores, han fumado " << total_fumados << " cigarros" << endl;
    }
}



//----------------------------------------------------------------------


int main()
{
   // declarar hebras y ponerlas en marcha
   thread hebra_fumador[num_fumadores];
   for (int i = 0; i < num_fumadores; i++) {
      hebra_fumador[i] = thread(funcion_hebra_fumador, i);
   }

   thread hebra_estanquero(funcion_hebra_estanquero);
   thread hebra_sanitaria(funcion_hebra_sanitaria);

   // Hay que hacer el join???? SI para que el main no acabe antes que las hebras (creo, como para acabar hay que hacer control C pues no se)
   for (int i = 0; i < num_fumadores; i++) {
      hebra_fumador[i].join();
   }
   hebra_estanquero.join();
   hebra_sanitaria.join();

   return 0;


}
