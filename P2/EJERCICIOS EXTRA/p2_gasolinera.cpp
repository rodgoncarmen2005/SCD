#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "scd.h"

using namespace std;
using namespace scd;

const int
    num_coches_diesel = 5,
    num_coches_gasolina = 5;

//--------------------------------------------------------------------------------

class Gasolinera : public HoareMonitor
{
private:
    static const int 
        num_surtidores_diesel = 2,
        num_surtidores_gasolina = 2;
    int                           
        surtidores_en_uso,
        surtidores_diesel_ocupados,
        surtidores_gasolina_ocupados;
    CondVar                       
        surt_diesel,                
        surt_gasolina;

public:                       // constructor y métodos públicos
    Gasolinera();             // constructor
    void entra_coche_diesel(int num_coche);   // entrar a repostar diesel
    void sale_coche_diesel(int num_coche);    // salir de repostar diesel
    void entra_coche_gasolina(int num_coche);  // entrar a repostar gasolina
    void sale_coche_gasolina(int num_coche);   // salir de repostar gasolina
};
// -----------------------------------------------------------------------------
Gasolinera::Gasolinera()
{
    surt_diesel = newCondVar();
    surt_gasolina = newCondVar();
    surtidores_en_uso = 0;
    surtidores_diesel_ocupados = 0;
    surtidores_gasolina_ocupados = 0;
}
// -----------------------------------------------------------------------------
void Gasolinera::entra_coche_diesel(int num_coche)
{
    if(surtidores_diesel_ocupados == num_surtidores_diesel)
        surt_diesel.wait();
    surtidores_diesel_ocupados++;
    surtidores_en_uso++;
    cout<<"Coche diesel "<< num_coche <<" entra a repostar. Surtidores en uso: "<<surtidores_en_uso<<endl;
}

void Gasolinera::sale_coche_diesel(int num_coche)
{
    surtidores_diesel_ocupados--;
    surtidores_en_uso--;
    cout<<"\t\t\t\t\t\t\t\tCoche diesel "<< num_coche <<" sale de repostar. Surtidores en uso: "<<surtidores_en_uso<<endl;
    surt_diesel.signal();
}

void Gasolinera::entra_coche_gasolina(int num_coche)
{
    if(surtidores_gasolina_ocupados == num_surtidores_gasolina)
        surt_gasolina.wait();
    surtidores_gasolina_ocupados++;
    surtidores_en_uso++;
    cout<<"Coche gasolina "<< num_coche <<" entra a repostar. Surtidores en uso: "<<surtidores_en_uso<<endl;
}

void Gasolinera::sale_coche_gasolina(int num_coche)
{
    surtidores_gasolina_ocupados--;
    surtidores_en_uso--;
    cout << "\t\t\t\t\t\t\t\tCoche gasolina " << num_coche << " sale de repostar. Surtidores en uso: " << surtidores_en_uso << endl;
    surt_gasolina.signal();
}
//--------------------------------------------------------------------------------
void funcion_hebra_diesel( MRef<Gasolinera> monitor, int num_coche)
{
    while(true)
    {
        monitor->entra_coche_diesel(num_coche);

        this_thread::sleep_for(chrono::milliseconds(aleatorio<100, 500>()));

        monitor->sale_coche_diesel(num_coche);

        this_thread::sleep_for(chrono::milliseconds(aleatorio<400, 600>()));
    }
}
//--------------------------------------------------------------------------------
void funcion_hebra_gasolina( MRef<Gasolinera> monitor, int num_coche)
{
    while(true)
    {
        monitor->entra_coche_gasolina(num_coche);
        this_thread::sleep_for(chrono::milliseconds(aleatorio<100, 500>()));
        monitor->sale_coche_gasolina(num_coche);
        this_thread::sleep_for(chrono::milliseconds(aleatorio<400, 600>()));
    }
}
//--------------------------------------------------------------------------------
int main()
{
    cout << "--------------------------------------------------------" << endl
         << "Problema de la gasolinera. Monitor con semántica Hoare." << endl
         << "--------------------------------------------------------" << endl;

    MRef<Gasolinera> monitor = Create<Gasolinera>();
    thread hebras_diesel[num_coches_diesel];
    thread hebras_gasolina[num_coches_gasolina];

    for(int i=0; i<num_coches_diesel; i++)
        hebras_diesel[i] = thread(funcion_hebra_diesel, monitor, i);
    for(int i=0; i<num_coches_gasolina; i++)
        hebras_gasolina[i] = thread(funcion_hebra_gasolina, monitor, i);

    for(int i=0; i<num_coches_diesel; i++)
        hebras_diesel[i].join();
    for(int i=0; i<num_coches_gasolina; i++)
        hebras_gasolina[i].join(); 
    
    return 0;
}


