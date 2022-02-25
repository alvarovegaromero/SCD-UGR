
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

const int num_fumadores = 3;	//num de fumadores

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

//-------------------------------------------------------------------------
// Función que simula la acción de producir un ingrediente, como un retardo
// aleatorio de la hebra (devuelve número de ingrediente producido)

int producir_ingrediente()
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_produ( aleatorio<10,100>() );

   // informa de que comienza a producir
   cout << "Estanquero : empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
   this_thread::sleep_for( duracion_produ );

   const int num_ingrediente = aleatorio<0,num_fumadores-1>() ;

   // informa de que ha terminado de producir
   cout << "Estanquero : termina de producir ingrediente " << num_ingrediente << endl;

   return num_ingrediente ;
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar

    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar

    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
}

// *****************************************************************************
// *****************************************************************************
// *****************************************************************************
// *****************************************************************************
// clase para monitor estanco, semántica SU.

class Estanco : public HoareMonitor
{
   private:
	int	mostrador;

	CondVar mostrador_vacio; 
	CondVar esta_mi_ig[num_fumadores];	

   public:
   Estanco() ; // constructor

   void ponerIngrediente(int ponerIngrediente);                
   void esperarRecogidaIngrediente();
   void obtenerIngrediente(int ig);

};

// -----------------------------------------------------------------------------

Estanco::Estanco(  )
{
   mostrador = -1;	//Mostrador vacio inicialmente
   mostrador_vacio = newCondVar();

	for(int i = 0 ; i < num_fumadores ; i++)
   		esta_mi_ig[i] = newCondVar();

}
// -----------------------------------------------------------------------------
void Estanco :: ponerIngrediente(int ig)
{
	mostrador = ig;	//Colocamos el ingrediente en el mostrador
	
	esta_mi_ig[ig].signal(); //Llamamos al fumador ig para que lo recoja
}

// -----------------------------------------------------------------------------
void Estanco :: esperarRecogidaIngrediente()
{
	if(mostrador != -1) //Si está vacío el mostrador
		mostrador_vacio.wait();	
	
}

// -----------------------------------------------------------------------------
void Estanco :: obtenerIngrediente(int ig)
{
	if(mostrador != ig)		//Si no está el ingrediente, esperar a que esté
		esta_mi_ig[ig].wait();

	mostrador = -1;	//El mostrador se encuentra vacío
	
	mostrador_vacio.signal();	//Señalar que esta vacio el mostrador
	
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(MRef<Estanco> monitor)
{	
	int ingrediente;

	while(true){
		ingrediente = producir_ingrediente();
		monitor->ponerIngrediente(ingrediente);
		cout << "Se ha puesto en el mostrador el igrediente: " << ingrediente << endl;
		monitor->esperarRecogidaIngrediente();
		

	}
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador(MRef<Estanco> monitor , int num_fumador )
{
   while( true )
   {

	monitor->obtenerIngrediente(num_fumador);
	cout << "Se ha retirado del mostrador el ingrediente: " << num_fumador << endl;

	fumar(num_fumador);
   }
}

// -----------------------------------------------------------------------------

int main()
{
   cout << "-------------------------------------------------------------------------------" << endl
        << "Problema de los fumadores. Monitor SU " << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;

   MRef<Estanco> monitor = Create<Estanco>();

   thread hebra_estanquero,
          hebras_fumadores[num_fumadores];

	hebra_estanquero = thread(funcion_hebra_estanquero, monitor);

	for(int i = 0; i < num_fumadores ; i++)
		hebras_fumadores[i] = thread(funcion_hebra_fumador, monitor, i); 
	
	hebra_estanquero.join();

	for(int i = 0; i < num_fumadores ; i++)
		hebras_fumadores[i].join();

}
