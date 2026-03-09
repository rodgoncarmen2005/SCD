// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 2. EJERCICIOS EXTRA. IMPRESORA.
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
   num_items = 40 ;   // número de items a producir/consumir
int
   siguiente_dato = 0; // siguiente valor a devolver en 'producir_dato'
   
   const int num_productores = 4, num_consumidores = 2;
constexpr int               
   min_ms    = 5,     // tiempo minimo de espera en sleep_for
   max_ms    = 20 ;   // tiempo máximo de espera en sleep_for

int cont_productores[num_productores] = {0};  // para cada hebra productora, número de items que ha producido

mutex mtx;        // mutex para la salida por pantalla
unsigned
   cont_prod[num_items] = {0}, // contadores de verificación: producidos
   cont_cons[num_items] = {0}; // contadores de verificación: consumidos

//**********************************************************************
// funciones para producir y consumir datos
//----------------------------------------------------------------------

unsigned producir_dato(int hebra_productora, int n)
{
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   const unsigned dato_producido = hebra_productora * num_items/num_productores + n;
   cont_prod[dato_producido] ++;
   cont_productores[hebra_productora] ++ ;
   mtx.lock();
   cout << "Productor " << hebra_productora << " produce: " << dato_producido << endl << flush ;
   mtx.unlock();
   return dato_producido ;
}
//----------------------------------------------------------------------

void consumir_dato(  int hebra_consumidora, unsigned dato)
{
   assert( dato < num_items );
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "\t\t\t\tConsumidor " << hebra_consumidora << " consume: " << dato << endl;
   mtx.unlock();
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
// clase para monitor buffer, version FIFO, semántica SC, multiples prod/cons

class Impresora : public HoareMonitor
{
 private:
 static const int// constantes ('static' ya que no dependen de la instancia)
   num_celdas_total = 10;//   núm. de entradas del buffer
 int// variables permanentes
   buffer[num_celdas_total],//buffer de tamaño fijo, con los datos
   primera_libre,//indice de celda de la próxima inserción ( == número de celdas ocupadas)
   primera_ocupada,//indice de celda de la próxima extracción
   num_ocupadas,//número de celdas ocupadas
   num_multiplos_totales,// número total de múltiplos de 5 producidos
   num_multiplos;// número de múltiplos de 5 desde la última llamada al método de la impresora
 CondVar                    // colas condicion:
   ocupadas,                //  cola donde espera el consumidor (n>0)
   libres,                  //  cola donde espera el productor  (n<num_celdas_total)
   num_multiplos_cond;      //  cola donde espera la impresora si no hay múltiplos de 5 nuevos

 public:                    // constructor y métodos públicos
   Impresora();             // constructor
   int leer();              // extraer un valor (sentencia L) (consumidor)
   void escribir(int valor);// insertar un valor (sentencia E) (productor)ç
   bool metodo_impresora(); // método de la impresora
} ;
// -----------------------------------------------------------------------------

Impresora::Impresora()
{
   primera_libre = 0 ;
   primera_ocupada = 0 ;
   num_ocupadas = 0 ;
   num_multiplos_totales = 0 ;
   num_multiplos = 0 ;
   ocupadas      = newCondVar();
   libres        = newCondVar();
   num_multiplos_cond = newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int Impresora::leer()
{
   // esperar bloqueado hasta que 0 < primera_libre
   if ( num_ocupadas == 0 )
      ocupadas.wait();

   //cout << "leer: ocup == " << primera_libre << ", total == " << num_celdas_total << endl ;
   assert( 0 <= primera_libre  );

   // hacer la operación de lectura, actualizando estado del monitor
   const int valor = buffer[primera_ocupada];
   primera_ocupada++;
   primera_ocupada = primera_ocupada % num_celdas_total;
   num_ocupadas--;
   // señalar al productor que hay un hueco libre, por si está esperando
   libres.signal();
   // devolver valor
   return valor ;
}
// -----------------------------------------------------------------------------

void Impresora::escribir(int valor)
{
   // esperar bloqueado hasta que primera_libre < num_celdas_total
   if (num_ocupadas == num_celdas_total)
      libres.wait();

   //cout << "escribir: ocup == " << primera_libre << ", total == " << num_celdas_total << endl ;
   assert( primera_libre <= num_celdas_total );

   // hacer la operación de inserción, actualizando estado del monitor
   buffer[primera_libre] = valor ;
   // Comprobar si el valor es múltiplo de 5
   if (valor % 5 == 0){
      num_multiplos++;
      num_multiplos_totales++;
      if (num_multiplos ==1){
         num_multiplos_cond.signal();
      }
   }
   // actualizar índice de primera libre y contador de ocupadas
   primera_libre++ ;
   primera_libre = primera_libre % num_celdas_total;
   num_ocupadas++;

   // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   ocupadas.signal();
}

bool Impresora::metodo_impresora(){
   
   int n = num_items/5;// número total de múltiplos de 5 que se van a producir

   // Si no se han producido todos los múltiplos de 5 y tenemos un múltiplo nuevo
   if ((num_multiplos_totales != n) && (num_multiplos > 0)){
      cout << "Se han impreso " << num_multiplos << " múltiplos de 5 desde la llamada anterior." << endl;
      cout << "Total de múltiplos de 5 impresos: " << num_multiplos_totales << endl;
      num_multiplos = 0;
      return true;
   // Si no se han producido todos los múltiplos de 5 y no tenemos múltiplos nuevos
   }else if((num_multiplos_totales != n) && (num_multiplos == 0)){
      num_multiplos_cond.wait();
      cout << "Se han impreso " << num_multiplos << " múltiplos de 5 desde la llamada anterior." << endl;
      cout << "Total de múltiplos de 5 impresos: " << num_multiplos_totales << endl;
      num_multiplos = 0;
      return true;
   // Si ya se han producido todos los múltiplos de 5
   }else{
      return false;
   } 
}

// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora(MRef<Impresora> monitor, int num_hebra)
{
   for( unsigned i = 0 ; i < num_items/num_productores ; i++ ) // cada hebra produce su parte
   {
      int valor = producir_dato( num_hebra, i ) ;
      monitor->escribir( valor );
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora(MRef<Impresora> monitor, int num_hebra)
{
   for( unsigned i = 0 ; i < num_items/num_consumidores ; i++ )// cada hebra consume su parte
   {
      int valor = monitor->leer();
      consumir_dato(num_hebra, valor ) ;
   }
}
// -----------------------------------------------------------------------------
void funcion_hebra_impresora(MRef<Impresora> monitor)
{
   bool seguir;// variable para controlar si la impresora debe seguir funcionando
   do{
      seguir = monitor->metodo_impresora();// llamar al método de la impresora
   }while (seguir);
}

int main()
{
   cout << "--------------------------------------------------------------------" << endl
        << "Problema del productor-consumidor múltiples (Monitor SU, buffer LIFO). " << endl
        << "--------------------------------------------------------------------" << endl
        << flush ;

   // crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
   MRef<Impresora> monitor = Create<Impresora>();

   // crear y lanzar las hebras
   
   thread hebras_consumidoras[num_consumidores];
   thread hebras_productoras[num_productores];
   thread hebra_impresora;
   for(int i=0; i<num_consumidores; i++) hebras_consumidoras[i] = thread(funcion_hebra_consumidora, monitor, i);
   for(int i=0; i<num_productores; i++) hebras_productoras[i] = thread(funcion_hebra_productora, monitor, i);
   hebra_impresora = thread(funcion_hebra_impresora, monitor);

   // esperar a que terminen las hebras
   for(int i=0; i<num_consumidores; i++)
   		hebras_consumidoras[i].join();
   for(int i=0; i<num_productores; i++)
   		hebras_productoras[i].join();

   hebra_impresora.join();
   // comprobar que cada item se ha producido y consumido exactamente una vez
   test_contadores() ;
}
