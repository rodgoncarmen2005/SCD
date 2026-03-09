// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 2. Introducción a los monitores en C++11.
//
// Archivo: prodcons1_su_mu_lifo.cpp

// -----------------------------------------------------------------------------------


#include <iostream>
#include <iomanip>
#include <cassert>
#include <mutex>
#include <random>
#include <thread>
#include "scd.h"

using namespace std ;
using namespace scd ;

constexpr int
   num_items = 20 ;   // número de items a producir/consumir
int
   siguiente_dato = 0; // siguiente valor a devolver en 'producir_dato'

//Variables para configurar el número de productores y consumidores
const int num_productores = 4, num_consumidores = 2;


constexpr int               
   min_ms    = 5,     // tiempo minimo de espera en sleep_for
   max_ms    = 20 ;   // tiempo máximo de espera en sleep_for

int cont_productores[num_productores] = {0};  // para cada hebra productora, número de items que ha producido

mutex mtx_cout ; 
                // mutex de escritura en pantalla
unsigned
   cont_prod[num_items] = {0}, // contadores de verificación: producidos
   cont_cons[num_items] = {0}; // contadores de verificación: consumidos

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

unsigned producir_dato(int hebra_productora, int n)
{
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   const unsigned dato_producido = hebra_productora * num_items/num_productores + n;
   cont_prod[dato_producido] ++;
   cont_productores[hebra_productora] ++ ;
   mtx_cout.lock();
   cout << "Productor " << hebra_productora << " produce: " << dato_producido << endl << flush ;
   mtx_cout.unlock();
   return dato_producido ;
}
//----------------------------------------------------------------------

void consumir_dato(  int hebra_consumidora, unsigned dato)
{
   assert( dato < num_items );
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx_cout.lock();
   cout << "\t\t\t\tConsumidor " << hebra_consumidora << " consume: " << dato << endl;
   mtx_cout.unlock();
}
//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." << endl ;

   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      if ( cont_prod[i] != 1 )
      {
         cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {
         cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

// *****************************************************************************
// clase para monitor buffer, version LIFO, semántica SC, multiples prod/cons

class ProdConsMU : public HoareMonitor
{
 private:
 static const int           // constantes ('static' ya que no dependen de la instancia)
   num_celdas_total = 10;   //   núm. de entradas del buffer
 int                        // variables permanentes
   buffer[num_celdas_total],//   buffer de tamaño fijo, con los datos
   primera_libre ;          //   indice de celda de la próxima inserción ( == número de celdas ocupadas)

 CondVar                    // colas condicion:
   ocupadas,                //  cola donde espera el consumidor (n>0)
   libres ;                 //  cola donde espera el productor  (n<num_celdas_total)

 public:                    // constructor y métodos públicos
   ProdConsMU() ;             // constructor
   int  leer();                // extraer un valor (sentencia L) (consumidor)
   void escribir( int valor ); // insertar un valor (sentencia E) (productor)
} ;
// -----------------------------------------------------------------------------

ProdConsMU::ProdConsMU(  )
{
   primera_libre = 0 ;
   ocupadas      = newCondVar();
   libres        = newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int ProdConsMU::leer(  )
{
   // esperar bloqueado hasta que 0 < primera_libre
   if ( primera_libre == 0 )
      ocupadas.wait();

   //cout << "leer: ocup == " << primera_libre << ", total == " << num_celdas_total << endl ;
   assert( 0 < primera_libre  );

   // hacer la operación de lectura, actualizando estado del monitor
   primera_libre-- ;
   const int valor = buffer[primera_libre] ;
   
   // señalar al productor que hay un hueco libre, por si está esperando
   libres.signal();
   // devolver valor
   return valor ;
}
// -----------------------------------------------------------------------------

void ProdConsMU::escribir( int valor )
{
   // esperar bloqueado hasta que primera_libre < num_celdas_total
   if ( primera_libre == num_celdas_total )
      libres.wait();

   //cout << "escribir: ocup == " << primera_libre << ", total == " << num_celdas_total << endl ;
   assert( primera_libre < num_celdas_total );

   // hacer la operación de inserción, actualizando estado del monitor
   buffer[primera_libre] = valor ;
   primera_libre++ ;

   // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   ocupadas.signal();
}
// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora( MRef<ProdConsMU> monitor, int num_hebra )
{
   for( unsigned i = 0 ; i < num_items/num_productores ; i++ )//cada hebra produce su parte
   {
      int valor = producir_dato( num_hebra, i ) ;
      monitor->escribir( valor );
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora( MRef<ProdConsMU>  monitor , int num_hebra)
{
   for( unsigned i = 0 ; i < num_items/num_consumidores ; i++ )//cada hebra consume su parte
   {
      int valor = monitor->leer();
      consumir_dato(num_hebra, valor ) ;
   }
}
// -----------------------------------------------------------------------------

int main()
{
   cout << "--------------------------------------------------------------------" << endl
        << "Problema del productor-consumidor múltiples (Monitor SU, buffer LIFO). " << endl
        << "--------------------------------------------------------------------" << endl
        << flush ;

   // crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
   MRef<ProdConsMU> monitor = Create<ProdConsMU>() ;

   // crear y lanzar las hebras
   
   thread hebras_consumidoras[num_consumidores], hebras_productoras[num_productores];
   for(int i=0; i<num_consumidores; i++) hebras_consumidoras[i] = thread(funcion_hebra_consumidora, monitor, i);
   for(int i=0; i<num_productores; i++) hebras_productoras[i] = thread(funcion_hebra_productora, monitor, i);

   // esperar a que terminen las hebras
   for(int i=0; i<num_consumidores; i++)
   		hebras_consumidoras[i].join();
   for(int i=0; i<num_productores; i++)
   		hebras_productoras[i].join();

   test_contadores() ;
}
