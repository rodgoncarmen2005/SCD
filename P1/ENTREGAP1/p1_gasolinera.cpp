#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "scd.h"

using namespace std;
using namespace scd;

//**********************************************************************
// Variables globales
const int
    num_coches_diesel=5,
    num_coches_gasolina=5,
    num_surtidores_diesel=2,
    num_surtidores_gasolina=2;

    Semaphore surt_diesel=num_surtidores_diesel;
    Semaphore surt_gasolina=num_surtidores_gasolina;
    Semaphore mutex_couts=1;
    int surtidores_en_uso=0;
    Semaphore proteccion_variable_contador=1;


//--------------------------------------------------------------------------------
    void repostar(int tipo, int id )
    {
        if(tipo==0){
            mutex_couts.sem_wait();
            cout << "coche "<< id <<", de tipo  diesel comienza a repostar "<< endl << flush ;
            mutex_couts.sem_signal();

            this_thread::sleep_for(chrono::milliseconds(aleatorio<100, 500>()));

            mutex_couts.sem_wait();
            cout << "\t\t\t\t\t\t\t\tcoche " << id << ", de tipo diesel termina de repostar " << endl << flush;
            mutex_couts.sem_signal();
        }

        if(tipo==1){
            mutex_couts.sem_wait();
            cout << "coche "<< id <<", de tipo gasolina comienza a repostar "<<endl << flush ;
            mutex_couts.sem_signal();

            this_thread::sleep_for(chrono::milliseconds(aleatorio<100, 500>()));

            mutex_couts.sem_wait();
            cout << "\t\t\t\t\t\t\t\tcoche " << id << ", de tipo gasolina termina de repostar " << endl << flush;
            mutex_couts.sem_signal();
        }
    }


    void funcion_hebra_diesel( int id)
    {
        while(true)
        {
            surt_diesel.sem_wait();

            proteccion_variable_contador.sem_wait();
                surtidores_en_uso++;
                mutex_couts.sem_wait();
                    cout << "Surtidores en uso: " << surtidores_en_uso << endl;
                mutex_couts.sem_signal();
            proteccion_variable_contador.sem_signal();
            

            repostar(0,id);

            
            proteccion_variable_contador.sem_wait();
                surtidores_en_uso--;
                mutex_couts.sem_wait();
                    cout << "Surtidores en uso: " << surtidores_en_uso << endl;
                mutex_couts.sem_signal();
            proteccion_variable_contador.sem_signal();
            
            surt_diesel.sem_signal();

            this_thread::sleep_for(chrono::milliseconds(aleatorio<400, 600>()));
        }
    }




    void funcion_hebra_gasolina( int id)
    {
        while(true)
        {
            surt_gasolina.sem_wait();

            proteccion_variable_contador.sem_wait();
                surtidores_en_uso++;
                mutex_couts.sem_wait();
                    cout<<"Surtidores en uso: "<<surtidores_en_uso<<endl;
                mutex_couts.sem_signal();
            proteccion_variable_contador.sem_signal();
            

            repostar(1,id);

            
            proteccion_variable_contador.sem_wait();
            surtidores_en_uso--;
                mutex_couts.sem_wait();
                    cout<<"Surtidores en uso: "<<surtidores_en_uso<<endl;
                mutex_couts.sem_signal();
            proteccion_variable_contador.sem_signal();
            
            surt_gasolina.sem_signal();

            this_thread::sleep_for(chrono::milliseconds(aleatorio<400, 600>()));
        }
    }


//----------------------------------------------------------------------------------

int main()
{
    cout << "-----------------------------------------------------------------" << endl
    << "Problema de la gasolinera" << endl
    << "------------------------------------------------------------------" << endl
    << flush;

    thread hebras_diesel[num_coches_diesel];
    thread hebras_gasolina[num_coches_gasolina];

    for(int i=0;i<num_coches_diesel;i++){
        hebras_diesel[i]=thread(funcion_hebra_diesel,i);
    }

    for(int i=0;i<num_coches_gasolina;i++){
        hebras_gasolina[i]=thread(funcion_hebra_gasolina,i);
    }


    for(int i=0;i<num_coches_diesel;i++){
        hebras_diesel[i].join();
    }

    for(int i=0;i<num_coches_gasolina;i++){
        hebras_gasolina[i].join();
    }

    return 0;

}
