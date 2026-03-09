// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: filosofos-plantilla.cpp
// Implementación del problema de los filósofos (sin camarero).
// Plantilla para completar.
//
// Historial:
// Actualizado a C++11 en Septiembre de 2017
// -----------------------------------------------------------------------------


#include <mpi.h>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <iostream>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int
   num_filosofos = 5 ,              // número de filósofos 
   num_filo_ten  = 2*num_filosofos, // número de filósofos y tenedores 
   num_procesos  = num_filo_ten ;   // número de procesos total (por ahora solo hay filo y ten)


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
bool es_filosofo(int id_propio) //filósofos con id par
{
   return id_propio % 2 == 0;
}

bool es_tenedor(int id_propio) //tenedores con id impar
{
   return id_propio % 2 == 1;
}

int obtener_id_filosofo(int id_propio)
{
   return id_propio / 2;
}

int obtener_id_propio_filosofo(int id_filosofo)
{
   return id_filosofo * 2;
}

int obtener_id_tenedor(int id_propio)
{
   return (id_propio - 1) / 2;
}


int obtener_id_propio_tenedor(int id_tenedor)
{
   return id_tenedor * 2 + 1;
}


//----------------------------------------------------------------------------------------------

void funcion_filosofos( int id )
{
   int id_ten_izq = id, // id. tenedor izq.
       id_ten_der = (id + num_filosofos - 1) % num_filosofos; // id. tenedor der.
   // hemos cambiado la función para calcular el tenedor derecho porque para usar el id lógico de los tenedores

   int solicitud = 1;

   while (true)
   {
      // solicita tenedor izquierdo
      cout << "Filósofo " << id << " solicita ten. izq." << id_ten_izq << endl;
      MPI_Ssend(&solicitud, 1, MPI_INT, obtener_id_propio_tenedor(id_ten_izq), 0, MPI_COMM_WORLD);

      // solicita tenedor derecho
      cout << "Filósofo " << id << " solicita ten. der." << id_ten_der << endl;
      MPI_Ssend(&solicitud, 1, MPI_INT, obtener_id_propio_tenedor(id_ten_der), 0, MPI_COMM_WORLD);

      // come
      cout << "Filósofo " << id << " comienza a comer" << endl;
      sleep_for(milliseconds(aleatorio<10, 100>()));

      // libera tenedor izquierdo
      cout << "Filósofo " << id << " suelta ten. izq. " << id_ten_izq << endl;
      MPI_Ssend(&solicitud, 1, MPI_INT, obtener_id_propio_tenedor(id_ten_izq), 0, MPI_COMM_WORLD);

      // libera tenedor derecho
      cout << "Filósofo " << id << " suelta ten. der. " << id_ten_der << endl;
      MPI_Ssend(&solicitud, 1, MPI_INT, obtener_id_propio_tenedor(id_ten_der), 0, MPI_COMM_WORLD);

      // piensa
      cout << "Filósofo " << id << " comienza a pensar" << endl;
      sleep_for(milliseconds(aleatorio<10, 100>()));
   }
}
// ---------------------------------------------------------------------

void funcion_tenedores( int id )
{
  int valor, id_filosofo ;  // valor recibido, identificador del filósofo
  MPI_Status estado ;       // metadatos de las dos recepciones


  while ( true )
  {
     // ...... recibir petición de cualquier filósofo (completar)
     MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &estado);
     // ...... guardar en 'id_filosofo' el id. del emisor (completar)
     id_filosofo = estado.MPI_SOURCE;
     cout <<"              Ten. " <<id <<" ha sido cogido por filo. " << obtener_id_filosofo(id_filosofo) <<endl;

     // ...... recibir liberación de filósofo 'id_filosofo' (completar)
     MPI_Recv(&valor, 1, MPI_INT, id_filosofo, 0, MPI_COMM_WORLD, &estado);
     cout << "              Ten. " << id << " ha sido liberado por filo. " << obtener_id_filosofo(id_filosofo) << endl;
  }
}
// ---------------------------------------------------------------------

int main( int argc, char** argv )
{
   int id_propio, num_procesos_actual ;

   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );


   if ( num_procesos == num_procesos_actual )
   {
      // ejecutar la función correspondiente a 'id_propio'
      if ( id_propio % 2 == 0 )          // si es par
         funcion_filosofos(obtener_id_filosofo(id_propio)); //   es un filósofo
      else                               // si es impar
         funcion_tenedores(obtener_id_tenedor(id_propio)); //   es un tenedor
   }
   else
   {
      if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
      { cout << "el número de procesos esperados es:    " << num_procesos << endl
             << "el número de procesos en ejecución es: " << num_procesos_actual << endl
             << "(programa abortado)" << endl ;
      }
   }

   MPI_Finalize( );
   return 0;
}

// ---------------------------------------------------------------------
