
#include <iostream>
#include <iomanip>
#include <cassert>
#include <random>
#include <thread>
#include "scd.h"

using namespace std;
using namespace scd;

// *****************************************************************************
// clase para monitor de lectores-escritores, versión SU, semántica SC

int nLectores = 4;
int nEscritores = 2;

constexpr int 	min_ms = 250, max_ms = 1000;

class Lec_Esc : public HoareMonitor
{
private:
    int n_lec;
    bool escrib;

    CondVar lectura, escritura;

public:                      // constructor y métodos públicos
    Lec_Esc();               // constructor
    void ini_lectura();      // inicio de la lectura
    void fin_lectura();      // fin de la lectura
    void ini_escritura();    // inicio de la escritura
    void fin_escritura();    // fin de la escritura
};
// -----------------------------------------------------------------------------
Lec_Esc::Lec_Esc()
{
    n_lec = 0;
    escrib = false;
    lectura = newCondVar();
    escritura = newCondVar();
}
// -----------------------------------------------------------------------------
void Lec_Esc::ini_lectura()
{
    if (escrib)//si hay un escritor
        lectura.wait(); //esperar
    n_lec++;//incrementar el numero de lectores
    cout << "Inicia lectura. Hay " << n_lec << " lectores" << endl;
    lectura.signal();// desbloqueo en cadena de posibles lectores bloqueados
}
// -----------------------------------------------------------------------------
void Lec_Esc::fin_lectura()
{
    n_lec--;//decrementar el numero de lectores
    cout << "\t\t\t\t\t\t\t\t\tTermina lectura. Hay " << n_lec << " lectores" << endl;
    if (n_lec == 0)//si es el último lector
        escritura.signal();//desbloquear a un escritor
}
// -----------------------------------------------------------------------------
void Lec_Esc::ini_escritura()
{
    if (n_lec > 0 || escrib)//si hay lectores o un escritor
        escritura.wait();//esperar
    escrib = true;//marcar que hay un escritor
    cout << "Comienza a escribir. Hay " << n_lec << " lectores" << endl;
}
// -----------------------------------------------------------------------------
void Lec_Esc::fin_escritura()
{
    escrib = false;//registrar que ya no hay un escritor
    //DAMOS PRIORIDAD A LOS LECTORES
    cout << "\t\t\t\t\t\t\t\t\tTermina de escribir. Hay " << n_lec << " lectores" << endl;
    if(!lectura.empty())//si hay lectores esperando
        lectura.signal();//desbloquear a un lector
    else
        escritura.signal();//desbloquear a un escritor
}

// *****************************************************************************
// funciones para los hebras de los lectores
void Lector(MRef<Lec_Esc> monitor){
    while (true) {
        monitor->ini_lectura();
        // sección de lectura
	chrono::milliseconds duracion(aleatorio < min_ms, max_ms > ());
	this_thread::sleep_for(duracion);
        monitor->fin_lectura();
        // sección fuera del monitor
	this_thread::sleep_for(duracion);
    }
}
// -----------------------------------------------------------------------------
// función para los hebras de los escritores
void Escritor(MRef<Lec_Esc> monitor){
    while (true) {
        monitor->ini_escritura();
        // sección de escritura
        chrono::milliseconds duracion(aleatorio < min_ms, max_ms > ());
	this_thread::sleep_for(duracion);
        monitor->fin_escritura();
        // sección fuera del monitor
	this_thread::sleep_for(duracion);
    }
}

// *****************************************************************************
// función main, crea hebras y monitor
int main()
{
    cout << "--------------------------------------------------------------------" << endl
         << "Problema de lectores-escritores únicos. " << endl
         << "--------------------------------------------------------------------" << endl
         << flush;

    // crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
    MRef<Lec_Esc> monitor = Create<Lec_Esc>();

    // crear y lanzar las hebras
   thread hebras_escritoras[nEscritores], hebras_lectoras[nLectores];
   for(int i=0; i<nEscritores; i++) hebras_escritoras[i] = thread(Escritor, monitor);
   for(int i=0; i<nLectores; i++) hebras_lectoras[i] = thread(Lector, monitor);

    // esperar a que terminen las hebras
    
   for(int i=0; i<nEscritores; i++)
   		hebras_escritoras[i].join();
   for(int i=0; i<nLectores; i++)
   		hebras_lectoras[i].join();

}
