#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "scd.h"

using namespace std ;
using namespace scd ;

//**********************************************************************
// Variables globales

const unsigned 
   num_items = 40 ,   // número de items
   tam_vec   = 10 ;   // tamaño del buffer

const int num_productores = 4;//número de productores
const int num_consumidores = 2;//número de consumidores
unsigned  
   cont_prod[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha producido.
   cont_cons[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha consumido.
   cont_productores[num_productores] = {0} , // para cada hebra productora, número de items que ha producido
   siguiente_dato       = 0 ;  // siguiente dato a producir en 'producir_dato' (solo se usa ahí)

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

Semaphore espera_consumidor = 0;
Semaphore espera_productor = tam_vec;

Semaphore mtx = 1;


unsigned buffer[tam_vec];
int primera_libre = 0;
int primera_ocupada = 0;

unsigned producir_dato(int hebra_productora, int n)
{
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   const unsigned dato_producido = hebra_productora * num_items/num_productores + n;
   cont_prod[dato_producido] ++;
   cont_productores[hebra_productora] ++ ;
   mtx.sem_wait();
   cout << "Productor " << hebra_productora << " produce: " << dato_producido << endl << flush ;
   mtx.sem_signal();
   return dato_producido ;
}
//----------------------------------------------------------------------

void consumir_dato(  int hebra_consumidora, unsigned dato)
{
   assert( dato < num_items );
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.sem_wait();
   cout << "\t\t\t\tConsumidor " << hebra_consumidora << " consume: " << dato << endl;
   mtx.sem_signal();
}


//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." ;
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  if ( cont_prod[i] != 1 )
      {  cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {  cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

//----------------------------------------------------------------------

void  funcion_hebra_productora(int hebra_productora)
{
   for( unsigned i = 0 ; i < num_items/num_productores; i++ )
   {
      int dato = producir_dato(hebra_productora, i) ;
      // completar ........
      sem_wait(espera_productor);
      buffer[primera_libre] = dato;
      primera_libre++;
      primera_libre %= tam_vec;
      sem_signal(espera_consumidor);  
   }
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora(int hebra_consumidora)
{
   for( unsigned i = 0 ; i < num_items/num_consumidores ; i++ )
   {
      int dato ;
      // completar ......
      sem_wait(espera_consumidor);
      dato = buffer[primera_ocupada];
      primera_ocupada++;
      primera_ocupada %= tam_vec;
      sem_signal(espera_productor);
      consumir_dato(hebra_consumidora, dato);
    }
}
//----------------------------------------------------------------------

int main()
{
   cout << "-----------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (solución FIFO)." << endl
        << "------------------------------------------------------------------" << endl
        << flush ;

   thread hebra_productora[num_productores];
   thread hebra_consumidora[num_consumidores];

   for (int i = 0; i < num_productores; i++)
      hebra_productora[i] = thread(funcion_hebra_productora, i);
   for (int i = 0; i < num_consumidores; i++)
      hebra_consumidora[i] = thread(funcion_hebra_consumidora, i);

   for (int i = 0; i < num_productores; i++)
      hebra_productora[i].join();
   for (int i = 0; i < num_consumidores; i++)
      hebra_consumidora[i].join();

          test_contadores();
}
