// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Seminario 2. Introducción a los monitores en C++11.
//
// archivo: prodconsLIFO_su.cpp
// Ejemplo de un monitor en C++11 con semántica SU, para el problema
// del productor/consumidor, con varios productores y consumidores.
// Opcion LIFO (stack)
//
// Historial:
// Creado en Julio de 2017
// -----------------------------------------------------------------------------


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

constexpr int
   num_items  = 80 ;     // número de items a producir/consumir

const int np = 5;	//num hebras productoras - divisores de num_items
const int nc = 5;	//num hebras consumidoras - divisores de num_items

int p = num_items / np; //cuanto produce cada productor
int c = num_items / nc; //cuando produce cada consumidor

int compartido[np] {};	//array que indica cuantos items ha producido cada hebra
						//productora

mutex
   mtx ;                 // mutex de escritura en pantalla
unsigned
   cont_prod[num_items], // contadores de verificación: producidos
   cont_cons[num_items]; // contadores de verificación: consumidos

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

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato(int i)
{
	
	int aux;

   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();

	aux = i * p + compartido [i]; //producir valor entre p y ip+p-1

	compartido[i]++;

   cout << "producido: " << aux << endl << flush ;
   mtx.unlock();
   cont_prod[aux] ++ ;
   return aux ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato )
{
   if ( num_items <= dato )
   {
      cout << " dato === " << dato << ", num_items == " << num_items << endl ;
      assert( dato < num_items );
   }
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "                  consumido: " << dato << endl ;
   mtx.unlock();
}
//----------------------------------------------------------------------

void ini_contadores()
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  cont_prod[i] = 0 ;
      cont_cons[i] = 0 ;
   }
}

//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." << flush ;

   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      if ( cont_prod[i] != 1 )
      {
         cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {
         cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

// *****************************************************************************
// *****************************************************************************
// *****************************************************************************
// *****************************************************************************
// clase para monitor buffer, version LIFO, semántica SU, un prod. y un cons.

class MProdConsSU : public HoareMonitor
{
   private:
   static const int           // constantes:
   num_celdas_total = 10;   //  núm. de entradas del buffer	

	int                        // variables permanentes
   	buffer[num_celdas_total],//  buffer de tamaño fijo, con los datos
   	pos_lectura ,          //  indice de celda de la próxima inserción
	pos_escritura,			//indice de celda para leer
	elementos;				//nos dice cuantos elementos tiene el vector en el momento


	CondVar ocupadas ;  // cola donde espera el consumidor (n>0)
	CondVar libres;		//  cola donde espera el productor  (n<num_celdas_total)

   public:
   MProdConsSU() ; // constructor
   int  leer();                // extraer un valor (sentencia L) (consumidor)
   void escribir( int valor ); // insertar un valor (sentencia E) (productor)
};

// -----------------------------------------------------------------------------

MProdConsSU::MProdConsSU(  )
{
   pos_lectura = 0 ;
   pos_escritura = 0;
   elementos = 0;
   libres = newCondVar();
   ocupadas = newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int MProdConsSU::leer(  )
{

   // esperar bloqueado hasta que 0 < num_celdas_ocupadas
   while ( elementos == 0 )
      ocupadas.wait();

   // hacer la operación de lectura, actualizando estado del monitor
   assert( 0 < elementos  );
   elementos-- ;
   const int valor = buffer[pos_lectura];

	pos_lectura = (pos_lectura + 1) % num_celdas_total;

   // señalar al productor que hay un hueco libre, por si está esperando
   libres.signal();

   // devolver valor
   return valor ;
}
// -----------------------------------------------------------------------------

void MProdConsSU::escribir( int valor )
{

   // esperar bloqueado hasta que num_celdas_ocupadas < num_celdas_total
   while ( elementos == num_celdas_total )
      libres.wait();

   //cout << "escribir: ocup == " << num_celdas_ocupadas << ", total == " << num_celdas_total << endl ;
   assert( elementos < num_celdas_total );

   // hacer la operación de inserción, actualizando estado del monitor
   buffer[pos_escritura] = valor ;
   elementos++ ;

	pos_escritura = (pos_escritura + 1) % num_celdas_total;

   // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   ocupadas.signal();
}
// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora( MRef<MProdConsSU> monitor, int j)
{
   for( unsigned i = 0 ; i < p ; i++ )
   {
      int valor = producir_dato(j) ;
      monitor->escribir( valor );
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora( MRef<MProdConsSU> monitor )
{
   for( unsigned i = 0 ; i < c ; i++ )
   {
      int valor = monitor->leer();
      consumir_dato( valor ) ;
   }
}
// -----------------------------------------------------------------------------

int main()
{
   cout << "-------------------------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (5 prod/cons, Monitor SU , buffer LIFO). " << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;

   MRef<MProdConsSU> monitor = Create<MProdConsSU>();

   thread hebra_productora[np],
          hebra_consumidora[nc];

	for(int i = 0 ; i < np ; i++)
		hebra_productora[i] = thread(funcion_hebra_productora, monitor, i);

	for(int i = 0; i < nc ; i++)
		hebra_consumidora[i] = thread(funcion_hebra_consumidora, monitor); 
	
	for(int i = 0 ; i < np ; i++)
		hebra_productora[i].join();

	for(int i = 0; i < nc ; i++)
		hebra_consumidora[i].join();

   // comprobar que cada item se ha producido y consumido exactamente una vez
   test_contadores() ;
}
