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
   num_procesos  = num_filo_ten + 1 ;   // número de procesos total (por ahora solo hay filo y ten)


const int etiq_sentarse = 0, etiq_levantarse = 1;
const int id_camarero = num_procesos - 1; //camarero con id 10


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
bool es_filosofo(int id_propio)
{
   return id_propio % 2 == 0;
}

bool es_tenedor(int id_propio)
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

   int solicitud = 1, liberar = 0;

   while (true)
   {
      //solicita al camareo sentarse
      cout << "Filosofo " << id<< " solicita sentarse" << endl;
      MPI_Ssend(&solicitud, 1, MPI_INT, id_camarero, etiq_sentarse, MPI_COMM_WORLD);

      // solicita tenedor izquierdo
      cout << "Filósofo " << id << " solicita ten. izq." << id_ten_izq << endl;
      MPI_Ssend(&solicitud, 1, MPI_INT, obtener_id_propio_tenedor(id_ten_izq), 0, MPI_COMM_WORLD);

      // solicita tenedor derecho
      cout << "Filósofo " << id << " solicita ten. der." << id_ten_der << endl;
      MPI_Ssend(&solicitud, 1, MPI_INT, obtener_id_propio_tenedor(id_ten_der), 0, MPI_COMM_WORLD);

      // come
      cout << "Filósofo " << id << " comienza a comer" << endl;
      sleep_for(milliseconds(aleatorio<300, 500>()));

      // libera tenedor izquierdo
      cout << "Filósofo " << id << " suelta ten. izq. " << id_ten_izq << endl;
      MPI_Ssend(&liberar, 1, MPI_INT, obtener_id_propio_tenedor(id_ten_izq), 0, MPI_COMM_WORLD);

      // libera tenedor derecho
      cout << "Filósofo " << id << " suelta ten. der. " << id_ten_der << endl;
      MPI_Ssend(&liberar, 1, MPI_INT, obtener_id_propio_tenedor(id_ten_der), 0, MPI_COMM_WORLD);

      // solicita al camareo levantarse
      cout << "Filósofo " << id << " solicita levantarse" << endl;
      MPI_Ssend(&liberar, 1, MPI_INT, id_camarero, etiq_levantarse, MPI_COMM_WORLD);

      // piensa
      cout << "Filósofo " << id << " comienza a pensar" << endl;
      sleep_for(milliseconds(aleatorio<300, 500>()));
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
void funcion_camarero()
{
   int solicitud;
   int etiq_aceptable;
   MPI_Status estado;
   int num_filosofos_sentados = 0;
   const int NUM_MAX_FILOSOFOS_SENTADOS = num_filosofos - 1;

   while (true)
   {
      /* si hay menos de 4 filósofos sentados, aceptamos cualquier petición,
      en caso contrario, solo aceptamos peticiones de levantarse.*/
      if (num_filosofos_sentados < NUM_MAX_FILOSOFOS_SENTADOS){
         etiq_aceptable = MPI_ANY_TAG;
      }else{
         etiq_aceptable = etiq_levantarse;
      }
      MPI_Recv(&solicitud, 1, MPI_INT, MPI_ANY_SOURCE, etiq_aceptable, MPI_COMM_WORLD, &estado);

      // según el valor de la solicitud:
      if (estado.MPI_TAG == etiq_sentarse) //hemos recibido un mensaje para sentarse
      {
         num_filosofos_sentados++;
         cout << "      CAMARERO:Filosofo " << obtener_id_filosofo(estado.MPI_SOURCE) << " se sienta. Hay " << num_filosofos_sentados << " filósofos sentados" << endl;
      }
      else if (estado.MPI_TAG == etiq_levantarse) //hemos recibido un mensaje para levantarse
      {
         num_filosofos_sentados--;
         cout << "      CAMERERO:Filosofo " << obtener_id_filosofo(estado.MPI_SOURCE) << " se levanta. Hay " << num_filosofos_sentados << " filósofos sentados" << endl;
      }
   }
}
//----------------------------------------------------------------------

int main( int argc, char** argv )
{
   int id_propio, num_procesos_actual ;

   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );


   if ( num_procesos == num_procesos_actual )
   {
      // ejecutar la función correspondiente a 'id_propio'
      if (id_propio == num_procesos - 1)  // si es el peníltimo proceso 
         funcion_camarero();              //   es el camarero
      else if (id_propio % 2 == 0)        // si el id es par
         funcion_filosofos(obtener_id_filosofo(id_propio)); // es un filósofo
      else                 
         funcion_tenedores(obtener_id_tenedor(id_propio)); // es un tenedor
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
