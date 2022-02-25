#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <condition_variable>
#include <random>
#include <mutex>
#include "HoareMonitor.h"

using namespace std ;
using namespace HM;

const int num_lectores = 3;
const int num_escritores = 3;

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

//-------------------------------------------------------------------------
// Función que hace que una hebra haga una espera aleatoria@i

void EsperaAleatorio()
{
	chrono::milliseconds duracion( aleatorio<100,500>() );
 	this_thread::sleep_for( duracion );	
}

//-------------------------------------------------------------------------
// Función que simula la acción de escribir, como un retardo
// aleatorio de la hebra 

void escribir(int escritor)
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_escrib( aleatorio<10,100>() );

   // informa de que comienza a producir
   cout << "Se está escribiendo. Escritor: "<< escritor << "	" << 				
	duracion_escrib.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
   this_thread::sleep_for( duracion_escrib );

   // informa de que ha terminado de producir
   cout << "Escritor " << escritor << " ha terminado" << endl;
}

//-------------------------------------------------------------------------
// Función que simula la acción de leer, como un retardo aleatoria de la hebra

void leer(int lector)
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_leer( aleatorio<20,200>() );

   // informa de que comienza a fumar

    cout << "Lector " << lector << "  :"
          << " empieza a leer (" << duracion_leer.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_leer );

   // informa de que ha terminado de fumar

    cout << "Lector " << lector << "  : termina de leer." << endl;
}

// *****************************************************************************
// *****************************************************************************
// *****************************************************************************
// *****************************************************************************
// clase para monitor estanco, semántica SU.

class Lec_Esc : public HoareMonitor
{
   private:
	int	n_lec;
	bool escrib;

	CondVar lectura; 
	CondVar escritura;	

   public:
   Lec_Esc() ; // constructor
   void ini_lectura();                
   void fin_lectura();
   void ini_escritura();
   void fin_escritura();

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
void Lec_Esc :: ini_lectura()                
{
	if (escrib)
		lectura.wait();

	n_lec++;

	lectura.signal();
}

// -----------------------------------------------------------------------------
void Lec_Esc :: fin_lectura()
{
	n_lec--;

	if (n_lec == 0)
		escritura.signal();
}

// -----------------------------------------------------------------------------
void Lec_Esc :: ini_escritura()
{
	if (n_lec > 0 or escrib)
		escritura.wait();
	
	escrib = true;
}

//----------------------------------------------------------------------
 void Lec_Esc :: fin_escritura()
{
	escrib = false;

					//Si hay lectores, tienen prioridad sobre otro escritor
	if (lectura.get_nwt() != 0)	 //SI el numero de procesos esperando es != 0
		lectura.signal();
	else
		escritura.signal();
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_lector(MRef<Lec_Esc> monitor, int num_lector)
{
	while(true)
	{
		EsperaAleatorio();
		monitor->ini_lectura();
		leer(num_lector);
		monitor->fin_lectura();
	}
}

//----------------------------------------------------------------------
void  funcion_hebra_escritor(MRef<Lec_Esc> monitor, int num_escritor)
{
   while( true )
   {
		EsperaAleatorio();
		monitor->ini_escritura();
		escribir(num_escritor);
		monitor->fin_escritura();
   }
}

// -----------------------------------------------------------------------------

int main()
{
   cout << "-------------------------------------------------------------------------------" << endl
        << "Problema de los lectores y escritores. Monitor SU " << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;

   MRef<Lec_Esc> monitor = Create<Lec_Esc>();

   thread hebras_lectoras[num_lectores],
          hebras_escritoras[num_escritores];

	for(int i = 0; i < num_lectores ; i++)
		hebras_lectoras[i] = thread(funcion_hebra_lector, monitor, i); 

	for(int i = 0; i < num_escritores ; i++)
		hebras_escritoras[i] = thread(funcion_hebra_escritor, monitor, i);
	

	for(int i = 0; i < num_lectores ; i++)
		hebras_lectoras[i].join();

	for(int i = 0; i < num_escritores ; i++)
		hebras_escritoras[i].join();

}
