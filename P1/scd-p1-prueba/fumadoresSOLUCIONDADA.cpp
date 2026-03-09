// SCD
// Implementación del enunciado de la variante de los fumadores del ensayo de examen de la P1

#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "scd.h"

using namespace std ;
using namespace scd ;

// Número de fumadores 
const int num_fumadores = 3 ;

// semáforo para esperar a que el mostrador quede vacío.
Semaphore mostr_vacio = 1;

// Vector de semáforos para esperar que esté cada ingrediente disponibles
// (se inicializan a 0 en 'main')
std::vector<Semaphore> ingr_disp ; 

// Semáforo para bloquear a la hebra sanitaria cuando no tiene que actuar
Semaphore sanidad = 0;  

// Semáforo para que el fumador espere a la hebra sanitaria
Semaphore poder_fumar = 0;  

// Número de veces que fuman sin recibir a la hebra sanitaria
int cigarrillos[num_fumadores] ; 

// Fumador detenido por la hebra sanitaria actualmente (solo puede ser uno)
int fumador_vicioso;

// Contador de total de cigarrillos fumados entre todos los fumadores:   
int total_cigarrillos = 0;

// Semáforo para incrementar la variable 'total_cigarrillos' en exclusión mutua
Semaphore mutex_total_cigarrillos = 1 ;

//-------------------------------------------------------------------------

int producir_ingrediente()
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_produ( aleatorio<10,90>() );

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
	int i;	
	
	while (true) {
		i = producir_ingrediente();
		
		sem_wait(mostr_vacio);
		cout << "Estanquero: puesto ingrediente " << i << " en el mostrador" << endl;
		sem_signal(ingr_disp[i]);
	}
}

// ------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<100,300>() );

   // informa de que comienza a fumar
   cout << "Fumador " << num_fumador << ": "
        << "empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar
   cout << "Fumador " << num_fumador << ": termina de fumar, comienza espera de ingrediente " << num_fumador << endl;

}

// ---------------------------------------------------------------------
// Función que ejecuta la hebra del fumador

void  funcion_hebra_fumador( int num_fumador )
{	
   while( true )
   {
      // Esperar a su ingrediente
      sem_wait(ingr_disp[num_fumador]);
         
      // Comprueba si el fumador ya ha fumado 5 cigarrillos "seguidos"
      if( cigarrillos[num_fumador] == 5 ) {

            // Registrar el numero de fumador vicioso
            // (no necesita EM ya que ningún otro fumador puede llegar aquí, 
            //  ya que el estanquero no ha quitado todavía este  ingrediente del 
            //  mostrador)
            fumador_vicioso = num_fumador;

            // Sincronización con la hebra sanitaria 
            sem_signal(sanidad);    // libera a la hebra sanitaria
            sem_wait(poder_fumar);  // espera a que la hebra sanitaria lo libere
            cout << "Fumador " << num_fumador << ": amonestado por la hebra sanitaria" << endl;

            // Reiniciar cuenta de cigarrillos fumados por este fumador
            cigarrillos[num_fumador] = 0;  
         
      }
      
      // Retirar el ingrediente del mostrador y fumar (igual que antes)
      cout << "Fumador " << num_fumador << ": retirando ingrediente" << endl;
      sem_signal(mostr_vacio);
      fumar(num_fumador);

      // Incrementar el contador total de cigarrillos fumados, e imprimir mensaje.
      // Es necesario hacerlo en EM ya que no se pueden solapar los incrementos. 
      // Además, el 'cout' se hace en la misma EM para asegurarnos que se imprime
      // el mismo valor resultado del incremento, y no otro posterior.

      sem_wait( mutex_total_cigarrillos );
         total_cigarrillos++ ; 
         cout << "Fumador " << num_fumador << ": entre todos los fumadores hemos fumado " << total_cigarrillos << " cigarros" << endl;
      sem_signal( mutex_total_cigarrillos );

      
      // Registrar un cigarrilo más fumado por este fumador 
      cigarrillos[num_fumador]++; 
      cout << "Fumador " << num_fumador << ": he fumado " << cigarrillos[num_fumador] << " cigarrillos seguidos" << endl;

      
   }
}

// ---------------------------------------------------------------------
// Función que ejecuta la hebra sanitaria

void funcion_hebra_sanitaria()
{

	while(true) {
	
      // Esperar a que algún fumador al despierte después de haber fumado 5 veces e imprimir mensaje.
		sem_wait( sanidad ); 
		cout << "Hebra sanitaria: el fumador " << fumador_vicioso << " ya ha fumado otros 5 cigarros"  << endl;
		
      // Liberar al fumador que ha despertado a la hebra sanitaria
		sem_signal( poder_fumar );  

      // Informar del total de cigarros fumados hasta ahora
      // No necesita EM para leer la variable 'total_cigarrillos' ya que la mera lectura es atómica.
      cout << "Hebra sanitaria: entre todos los fumadores, han fumado " << total_cigarrillos << " cigarros" << endl ;
      
	}
}

//----------------------------------------------------------------------

int main()
{
   cout << "-----------------------------------------------------------------" << endl
        << "Ensayo de examen de la práctica 1                                " << endl 
        << "-----------------------------------------------------------------" << endl
        << flush ;

   // Inicializar el vector de semáforos y los contadores 
   for( int i=0; i<num_fumadores; i++ ) {
   	ingr_disp.push_back( Semaphore(0) );
   	cigarrillos[i] = 0;
   }
        
   // Declaración de hebras
   thread hebra_estanquero(funcion_hebra_estanquero),
   		 hebras_fumadoras[num_fumadores], 
   		 hebra_sanitaria(funcion_hebra_sanitaria);
   	
   // Poner en marcha las hebras fumadoras	  
   for(int i=0; i<num_fumadores; i++)
   		hebras_fumadoras[i] = thread(funcion_hebra_fumador, i);
   
   
   // Esperar a que terminan las hebras
   hebra_estanquero.join();
   hebra_sanitaria.join();
   for(int i=0; i<num_fumadores; i++)
   		hebras_fumadoras[i].join();
              
        
}





