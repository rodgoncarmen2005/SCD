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
using namespace std::this_thread;
using namespace std::chrono;

const int
    num_productores = 4,  // número de productores
    num_consumidores = 5, // número de consumidores
    num_procesos_esperado = num_productores + num_consumidores + 1,
    num_items = 20,
    tam_vector = 10,
    TERM = -1;

//Calculo de items para cada consumidor y productor
const int items_por_productor = num_items / num_productores,
          items_por_consumidor = num_items / num_consumidores;

const int id_buffer = num_productores;

// Constantes referentes a las etiquetas
const int
    etiq_productor = 1,
    etiq_consumidor = 2;

int obtener_id_productor(int id_propio)
{
    return id_propio;
}

int obtener_id_consumidor(int id_propio)
{
    return id_propio - num_productores - 1;
}

bool es_productor(int id_propio)
{
    return 0 <= id_propio &&
           id_propio < num_productores;
}

bool es_buffer(int id_propio)
{
    return id_propio == num_productores;
}

bool es_consumidor(int id_propio)
{
    return num_productores < id_propio &&
           id_propio < num_productores + num_consumidores + 1;
}

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template <int min, int max>
int aleatorio()
{
    static default_random_engine generador((random_device())());
    static uniform_int_distribution<int> distribucion_uniforme(min, max);
    return distribucion_uniforme(generador);
}
// ---------------------------------------------------------------------
// ptoducir produce los numeros en secuencia (1,2,3,....)
// y lleva espera aleatorio
int producir(int id_productor)
{
    sleep_for(milliseconds(aleatorio<10, 100>()));
    int valor = aleatorio<0, 9>();
    cout << "Productor " << id_productor << " ha producido valor " << valor << endl << flush;
    return valor;

}
// ---------------------------------------------------------------------

void funcion_productor(int id_productor)
{
    for (unsigned int i = id_productor * items_por_productor; i < (id_productor + 1) * items_por_productor; i++)
    {
        // producir valor
        int valor_prod = producir(id_productor);
        int rec;
        MPI_Status estado;

        // enviar valor
        cout << "Productor " << id_productor << " va a enviar valor " << valor_prod << endl << flush;
        MPI_Ssend(&valor_prod, 1, MPI_INT, id_buffer, etiq_productor, MPI_COMM_WORLD);
        MPI_Recv(&rec, 1, MPI_INT, id_buffer, 0, MPI_COMM_WORLD, &estado);
        if (rec == TERM)
        {
            break;
        }
        MPI_Ssend(&valor_prod, 1, MPI_INT, id_buffer, 0, MPI_COMM_WORLD);
    }
    cout << "                                Termina productor " << obtener_id_productor(id_productor) << endl;
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
        MPI_Ssend(&peticion, 1, MPI_INT, id_buffer, etiq_consumidor, MPI_COMM_WORLD);
        MPI_Recv(&valor_rec, 1, MPI_INT, id_buffer, etiq_consumidor, MPI_COMM_WORLD, &estado); // Esta etiqueta no sería necesaria
        if (valor_rec == TERM)
        {
            break;
        }
        cout << "                Consumidor " << id_consumidor << " ha recibido valor " << valor_rec << endl << flush;
        consumir(valor_rec, id_consumidor);
        cout << "                                Termina consumidor " << id_consumidor << endl;
    }
}
// ---------------------------------------------------------------------

void funcion_buffer()
{
    int buffer[tam_vector],      // buffer con celdas ocupadas y vacías
        valor,                   // valor recibido o enviado
        primera_libre = 0,       // índice de primera celda libre
        primera_ocupada = 0,     // índice de primera celda ocupada
        num_celdas_ocupadas = 0, // número de celdas ocupadas
        etiq_aceptable;          // identificador de emisor aceptable
    MPI_Status estado;           // metadatos del mensaje recibido


    bool terminar = false;

    for (unsigned int i = 0; i < num_items * 2 && !terminar; i++)
    {
        // 1. determinar si puede enviar solo prod., solo cons, o todos

        if (num_celdas_ocupadas == 0)               // si buffer vacío
            etiq_aceptable = etiq_productor;        // $~~~$ solo prod.
        else if (num_celdas_ocupadas == tam_vector) // si buffer lleno
            etiq_aceptable = etiq_consumidor;       // $~~~$ solo cons.
        else                                        // si no vacío ni lleno
            etiq_aceptable = MPI_ANY_TAG;           // $~~~$ cualquiera

        // 2. recibir un mensaje del emisor o emisores aceptables

        MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_aceptable, MPI_COMM_WORLD, &estado);

        // 3. procesar el mensaje recibido

        switch (estado.MPI_TAG)
        { // leer emisor del mensaje en metadatos

        case etiq_productor: // si ha sido un productor: insertar en buffer

            MPI_Ssend(&valor, 1, MPI_INT, estado.MPI_SOURCE, 0, MPI_COMM_WORLD);
            MPI_Recv(&valor, 1, MPI_INT, estado.MPI_SOURCE, 0, MPI_COMM_WORLD, &estado);

            // Dos valores iguales consecutivos recibidos
            if (valor == buffer[(primera_libre - 1) % tam_vector])
            {
                cout << "                                Empezamos a terminar el programa" << endl;
                terminar = true;
                int aux;
                MPI_Request request;
                // terminar procesos productores
                for (int i = 0; i < id_buffer; i++)
                {
                    MPI_Irecv(&aux, 1, MPI_INT, i, MPI_ANY_TAG, MPI_COMM_WORLD, &request);
                    MPI_Irecv(&aux, 1, MPI_INT, i, MPI_ANY_TAG, MPI_COMM_WORLD, &request);
                    aux = TERM;
                    MPI_Ssend(&aux, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                }
                // terminar procesos consumidores
                for (int i = id_buffer + 1; i < num_procesos_esperado; i++)
                {
                    MPI_Irecv(&aux, 1, MPI_INT, i, MPI_ANY_TAG, MPI_COMM_WORLD, &request);
                    aux = TERM;
                    MPI_Ssend(&aux, 1, MPI_INT, i, etiq_consumidor, MPI_COMM_WORLD);
                }
            }
            if (!terminar)
            {
                buffer[primera_libre] = valor;
                primera_libre = (primera_libre + 1) % tam_vector;
                num_celdas_ocupadas++;
                cout << "Buffer ha recibido valor " << valor << endl;
            }
            break;
            

        case etiq_consumidor: // si ha sido un consumidor: extraer y enviarle
            if (!terminar)
            {
                valor = buffer[primera_ocupada];
                primera_ocupada = (primera_ocupada + 1) % tam_vector;
                num_celdas_ocupadas--;
                cout << "Buffer va a enviar valor " << valor << endl;
                MPI_Ssend(&valor, 1, MPI_INT, estado.MPI_SOURCE, etiq_consumidor, MPI_COMM_WORLD);
            }
            break;
        }
    }
    cout << "                                Termina buffer" << endl;
}

// ---------------------------------------------------------------------

int main(int argc, char *argv[])
{
    int id_propio,
        num_procesos_actual;

    // inicializar MPI, leer identif. de proceso y número de procesos
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id_propio);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procesos_actual);

    if (num_procesos_esperado == num_procesos_actual)
    {
        // ejecutar la operación apropiada a 'id_propio'
        if (es_productor(id_propio))
        {
            funcion_productor(obtener_id_productor(id_propio));
        }
        else if (es_buffer(id_propio))
            funcion_buffer();
        else if (es_consumidor(id_propio))
        {
            funcion_consumidor(obtener_id_consumidor(id_propio));
        }
        else // A priori no debe llegar aquí
            cout << "Error: identificador de proceso desconocido" << endl;
    }
    else
    {

        if (id_propio == 0)
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