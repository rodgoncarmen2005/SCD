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
    num_productores = 3,  // número de productores
    num_consumidores = 2, // número de consumidores
    num_procesos_esperado = num_productores + num_consumidores + 1,
    num_items = 18,
    tam_vector = 10;

//Calculo de items para cada consumidor y productor
const int items_por_productor = num_items / num_productores,
          items_por_consumidor = num_items / num_consumidores;

const int id_buffer = num_productores,
          id_consumidor_impar = num_productores + 1,
          id_consumidor_par = num_productores + 2;

// Constantes referentes a las etiquetas
const int
    etiq_productor = 1,
    etiq_consumidor = 2,
    etiq_consumidor_par = 3,
    etiq_consumidor_impar = 4;

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
    return 0 <= id_propio && id_propio < num_productores;
}

bool es_buffer(int id_propio)
{
    return id_propio == id_buffer;
}

bool es_consumidor_impar(int id_propio)
{
    return id_propio == id_consumidor_impar;
}

bool es_consumidor_par(int id_propio)
{
    return id_propio == id_consumidor_par;
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
    static int contador = items_por_productor * id_productor;
    sleep_for(milliseconds(aleatorio<10, 100>()));
    contador++;
    cout << "Productor " << id_productor << " ha producido valor " << contador << endl << flush;
    return contador;
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
        MPI_Ssend(&valor_prod, 1, MPI_INT, id_buffer, etiq_productor, MPI_COMM_WORLD);
    }
}
// ---------------------------------------------------------------------

void consumir(int valor_cons, int id_consumidor)
{
    // espera bloqueada
    sleep_for(milliseconds(aleatorio<110, 200>()));
    cout << "                Consumidor " << obtener_id_consumidor(id_consumidor) << " ha consumido valor " << valor_cons << endl << flush;
}
// ---------------------------------------------------------------------

void funcion_consumidor_impar(int id_consumidor)
{
    int peticion,
        valor_rec = 1;
    MPI_Status estado;

    for (unsigned int i = 0; i < items_por_consumidor; i++)
    {
        MPI_Ssend(&peticion, 1, MPI_INT, id_buffer, etiq_consumidor, MPI_COMM_WORLD);
        MPI_Recv(&valor_rec, 1, MPI_INT, id_buffer, etiq_consumidor_impar, MPI_COMM_WORLD, &estado); 
        cout << "                Consumidor " << obtener_id_consumidor(id_consumidor) << " ha recibido valor " << valor_rec << endl << flush;
        consumir(valor_rec, id_consumidor);
    }
}

void funcion_consumidor_par(int id_consumidor)
{
    int peticion,
        valor_rec = 1;
    MPI_Status estado;

    for (unsigned int i = 0; i < items_por_consumidor; i++)
    {
        MPI_Ssend(&peticion, 1, MPI_INT, id_buffer, etiq_consumidor, MPI_COMM_WORLD);
        MPI_Recv(&valor_rec, 1, MPI_INT, id_buffer, etiq_consumidor_par, MPI_COMM_WORLD, &estado);
        cout << "                Consumidor " << obtener_id_consumidor(id_consumidor) << " ha recibido valor " << valor_rec << endl << flush;
        consumir(valor_rec, id_consumidor);
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

    for (unsigned int i = 0; i < num_items*2; i++)
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
            buffer[primera_libre] = valor;
            primera_libre = (primera_libre + 1) % tam_vector;
            num_celdas_ocupadas++;
            cout << "Buffer ha recibido valor " << valor << " del productor " << obtener_id_productor(estado.MPI_SOURCE) << endl;
            break;

        case etiq_consumidor: // si ha sido un consumidor: extraer y enviarle
            valor = buffer[primera_ocupada];
            primera_ocupada = (primera_ocupada + 1) % tam_vector;
            num_celdas_ocupadas--;


            int receptor_id;
            int etiqueta_envio;

            if (valor % 2 != 0)
            { // Valor IMPAR
                receptor_id = id_consumidor_impar;
                etiqueta_envio = etiq_consumidor_impar;
                cout << "                Buffer va a enviar valor " << valor << " al consumidor IMPAR (ID: " << obtener_id_consumidor(receptor_id) << ")" << endl;
            }
            else
            { // Valor PAR
                receptor_id = id_consumidor_par;
                etiqueta_envio = etiq_consumidor_par;
                cout << "                Buffer va a enviar valor " << valor << " al consumidor PAR (ID: " << obtener_id_consumidor(receptor_id) << ")" << endl;
            }

            // Envío al consumidor especializado
            MPI_Send(&valor, 1, MPI_INT, receptor_id, etiqueta_envio, MPI_COMM_WORLD);
            break;
        }
    }
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
        {
            funcion_buffer();
        }
        else if (es_consumidor_impar(id_propio)) // Consumidor IMPAR: ID = num_productores + 1
        {
            funcion_consumidor_impar(id_propio);
        }
        else if (es_consumidor_par(id_propio)) // Consumidor PAR: ID = num_productores + 2
        {
            funcion_consumidor_par(id_propio);
        }
        else // A priori no debe llegar aquí
            cout << "Error: identificador de proceso desconocido" << endl;
    }else
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