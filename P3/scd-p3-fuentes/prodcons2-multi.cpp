// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: prodcons2.cpp
// Implementación del problema del productor-consumidor con
// un proceso intermedio que gestiona un buffer finito y recibe peticiones
// en orden arbitrario
// (versión con un único productor y un único consumidor)
//
// Historial:
// Actualizado a C++11 en Septiembre de 2017
// -----------------------------------------------------------------------------

#include <iostream>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <mpi.h>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int
    num_productores = 4,  // número de productores
    num_consumidores = 5, // número de consumidores
    num_procesos_esperado = num_productores + num_consumidores + 1,
    num_items = 20,
    tam_vector = 10;

// Calculo de items para cada consumidor y productor
const int items_por_productor = num_items / num_productores,
          items_por_consumidor = num_items / num_consumidores;

const int id_buffer = num_productores;

int obtener_id_productor(int id_propio_global) // productores entre 0 y num_productores - 1
{
   return id_propio_global;
}

int obtener_id_consumidor(int id_propio_global) // consumidores entre num_productores + 1 y num_procesos
{
   return id_propio_global - num_productores - 1;
}

bool es_productor(int id_propio_global)
{
   return 0 <= id_propio_global && id_propio_global < num_productores;
}

bool es_buffer(int id_propio_global)// buffer tiene id = num_productores
{
   return id_propio_global == num_productores;
}

bool es_consumidor(int id_propio_global)
{
   return num_productores < id_propio_global && id_propio_global < num_productores + num_consumidores + 1;
}

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}
// ---------------------------------------------------------------------
// ptoducir produce los numeros en secuencia (1,2,3,....)
// y lleva espera aleatorio
int producir(int id_productor)
{
   static int contador = items_por_productor * id_productor;
   sleep_for( milliseconds( aleatorio<10,100>()) );
   contador++ ;
   cout << "Productor ha producido valor " << contador << endl << flush;
   return contador ;
}
// ---------------------------------------------------------------------

void funcion_productor(int id_productor)
{
   for (unsigned int i = id_productor * items_por_productor; i < (id_productor + 1) * items_por_productor; i++)
   {
      // producir valor
      int valor_prod = producir(id_productor);
      // enviar valor
      cout << "Productor " << id_productor << " va a enviar valor " << valor_prod << endl << flush;
      // ENVIAMOS POR ETIQUETA 0
      MPI_Ssend(&valor_prod, 1, MPI_INT, id_buffer, 0, MPI_COMM_WORLD);
   }
}
// ---------------------------------------------------------------------

void consumir(int valor_cons, int id_consumidor)
{
   // espera bloqueada
   sleep_for(milliseconds(aleatorio<110, 200>()));
   cout << "                Consumidor " << id_consumidor << " ha consumido valor " << valor_cons << endl << flush;
}
// ---------------------------------------------------------------------

void funcion_consumidor(int id_consumidor)
{
   int peticion,
       valor_rec = 1;
   MPI_Status estado;

   for (unsigned int i = 0; i < items_por_consumidor; i++)
   {
      // ENVIAMOS PETICION POR ETIQUETA 0
      MPI_Ssend(&peticion, 1, MPI_INT, id_buffer, 0, MPI_COMM_WORLD);
      // RECIBIMOS CON ETIQUETA 0
      MPI_Recv(&valor_rec, 1, MPI_INT, id_buffer, 0, MPI_COMM_WORLD, &estado); 
      cout << "                Consumidor " << id_consumidor << " ha recibido valor " << valor_rec << endl << flush;
      consumir(valor_rec, id_consumidor);
   }
}
// ---------------------------------------------------------------------

void funcion_buffer()
{
   int buffer[tam_vector],             // buffer con celdas ocupadas y vacías
       valor,                          // valor recibido o enviado
       primera_libre = 0,              // índice de primera celda libre
       primera_ocupada = 0,            // índice de primera celda ocupada
       num_celdas_ocupadas = 0,        // número de celdas ocupadas
       id_emisor_aceptable;            // identificador de emisor aceptable
   MPI_Status estado ;                 // metadatos del mensaje recibido


   int id_min, id_max;

   for( unsigned int i=0 ; i < num_items*2 ; i++ )
   {
      // 1. determinar si puede enviar solo prod., solo cons, o todos

      if ( num_celdas_ocupadas == 0 ){ // si buffer vacío
         id_min = 0;                   // $~~~$ solo prod.
         id_max = num_productores -1; 
      }                          
      else if (num_celdas_ocupadas == tam_vector) {// si buffer lleno
         id_min = num_productores + 1;             // $~~~$ solo cons.
         id_max = num_productores + num_consumidores;
      }
      else{ // si no vacío ni lleno
         id_min = 0;                   // TODOS
         id_max = num_productores + num_consumidores;
      }

      int mensaje_encontrado = 0; //FLAG
      id_emisor_aceptable = -1;
      while (id_emisor_aceptable == -1)
      {
         for (int id = id_min; id <= id_max; id++)
         {
            if (id != id_buffer) // evitar sondeo a sí mismo
               // sondeo de mensajes entre los procesos aceptables
               MPI_Iprobe(id, MPI_ANY_TAG, MPI_COMM_WORLD, &mensaje_encontrado, &estado);
            if (mensaje_encontrado)
            {
               id_emisor_aceptable = id; //id del proceso que tenía un mensaje pendiente
               break;
            }
         }
      }

      // 2. recibir un mensaje del emisor o emisores aceptables

      MPI_Recv(&valor, 1, MPI_INT, id_emisor_aceptable, 0, MPI_COMM_WORLD, &estado);

      // 3. procesar el mensaje recibido

      int id = estado.MPI_SOURCE;

      if (id < num_productores) //según el id del mensaje recibido sabemos si es de un productor o de un consumidor
      { // productor
         buffer[primera_libre] = valor;
         primera_libre = (primera_libre + 1) % tam_vector;
         num_celdas_ocupadas++;
         cout << "Buffer ha recibido valor " << valor << endl;
      }
      else
      {
         // consumidor
         valor = buffer[primera_ocupada];
         primera_ocupada = (primera_ocupada + 1) % tam_vector;
         num_celdas_ocupadas--;
         cout << "                Buffer va a enviar valor " << valor << endl;
         MPI_Ssend(&valor, 1, MPI_INT, id, 0, MPI_COMM_WORLD);
      }
   }
}

// ---------------------------------------------------------------------

int main( int argc, char *argv[] )
{
   int id_propio_global,
       num_procesos_actual;

   // inicializar MPI, leer identif. de proceso y número de procesos
   MPI_Init(&argc, &argv);
   MPI_Comm_rank(MPI_COMM_WORLD, &id_propio_global);
   MPI_Comm_size(MPI_COMM_WORLD, &num_procesos_actual);

   if (num_procesos_esperado == num_procesos_actual)
   {
      // ejecutar la operación apropiada a 'id_propio'
      if (es_productor(id_propio_global))
      {
         funcion_productor(obtener_id_productor(id_propio_global));
      }
      else if (es_buffer(id_propio_global))
         funcion_buffer();
      else if (es_consumidor(id_propio_global))
      {
         funcion_consumidor(obtener_id_consumidor(id_propio_global));
      }
      else 
         cout << "Error: identificador de proceso desconocido" << endl;
   }
   else
   {

      if (id_propio_global == 0)
      { // solo el primero escribe error, indep. del rol
         cout << "el número de procesos esperados es:    " << num_procesos_esperado << endl
              << "el número de procesos en ejecución es: " << num_procesos_actual << endl
              << "(programa abortado)" << endl;
      }
   }

   // al terminar el proceso, finalizar MPI
   MPI_Finalize();
   return 0;
}
