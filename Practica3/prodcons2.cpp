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
using namespace std::this_thread ;
using namespace std::chrono ;

const int
   num_procesos_esperado = 10 ,
   tam_vector            = 10,
   etiqueta_prod = 0,
   etiqueta_cons = 1,
   np = 4,
   nc = 5,
   id_buffer             = np,
   num_items             = np*nc*5; //Multiplo de np y nc



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
// ptoducir produce los numeros en secuencia (1,2,3,....)
// y lleva espera aleatorio
int producir(int id)
{
   static int contador = (id * (num_items / np)) ;
   sleep_for( milliseconds( aleatorio<10,100>()) );
   contador++;
   cout << "Productor " << id << " ha producido valor " << contador << endl << flush;
   return contador ;
}
// ---------------------------------------------------------------------

void funcion_productor(int id)
{
   for ( unsigned int i= 0 ; i < num_items/np ; i++ )
   {
      // producir valor
      int valor_prod = producir(id);
      // enviar valor
      cout << "Productor " << id << " va a enviar valor " << valor_prod << endl << flush;
      // la etiqueta ahora importa (etiq_p)
      MPI_Ssend( &valor_prod, 1, MPI_INT, id_buffer, etiqueta_prod, MPI_COMM_WORLD );
   }
}
// ---------------------------------------------------------------------

void consumir( int valor_cons , int id)
{
   // espera bloqueada
   sleep_for( milliseconds( aleatorio<110,200>()) );
   cout << "Consumidor " << id << " ha consumido valor " << valor_cons << endl << flush ;
}
// ---------------------------------------------------------------------

void funcion_consumidor(int id)
{
   int         peticion,
               valor_rec = 1 ;
   MPI_Status  estado ;

   // al dividirse el trabajo consumirán num_items/nc
   for( unsigned int i=0 ; i < num_items/nc; i++ ) 
   {
      // la etiqueta ahora importa (etiq_c)
      MPI_Ssend( &peticion,  1, MPI_INT, id_buffer, etiqueta_cons, MPI_COMM_WORLD);
      MPI_Recv ( &valor_rec, 1, MPI_INT, id_buffer, etiqueta_cons, MPI_COMM_WORLD,&estado );
      cout << "Consumidor " << id << " ha recibido valor " << valor_rec << endl << flush ;
      consumir( valor_rec, id );
   }
}
// ---------------------------------------------------------------------

void funcion_buffer()
{
   int        buffer[tam_vector],      // buffer con celdas ocupadas y vacías
              valor,                   // valor recibido o enviado
              primera_libre       = 0, // índice de primera celda libre
              primera_ocupada     = 0, // índice de primera celda ocupada
              num_celdas_ocupadas = 0, // número de celdas ocupadas
              tag ;                    // etiqueta del proceso aceptable
   MPI_Status estado ;                 // metadatos del mensaje recibido

   for( unsigned int i=0 ; i < num_items*2 ; i++ )
   {
      // 1. determinar si puede enviar solo prod., solo cons, o todos

      if ( num_celdas_ocupadas == 0 )               // si buffer vacío
         tag = etiqueta_prod ;       // $~$ solo prod. (tendrá etiqueta del productor)
      else if ( num_celdas_ocupadas == tam_vector ) // si buffer lleno
         tag = etiqueta_cons ;      // $~$ solo cons. (tendrá etiqueta del consumidor)
      else                                          // si no vacío ni lleno
         tag = MPI_ANY_TAG ;     // $~$ cualquiera (cualquier etiqueta)

      // 2. recibir un mensaje de cualquier proceso, discriminando por etiqueta

      MPI_Recv( &valor, 1, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &estado );

      // 3. procesar el mensaje recibido

      switch( estado.MPI_TAG ) // leer emisor de la eti1ueta en metadatos
      {
         case etiqueta_prod: // si ha sido el productor: insertar en buffer
            buffer[primera_libre] = valor ;
            primera_libre = (primera_libre+1) % tam_vector ;
            num_celdas_ocupadas++ ;
            cout << "Buffer ha recibido valor " << valor << endl ;
            break;

         case etiqueta_cons: // si ha sido el consumidor: extraer y enviarle
            valor = buffer[primera_ocupada] ;
            primera_ocupada = (primera_ocupada+1) % tam_vector ;
            num_celdas_ocupadas-- ;
            cout << "Buffer va a enviar valor " << valor << endl ;
            MPI_Ssend( &valor, 1, MPI_INT, estado.MPI_SOURCE, etiqueta_cons, MPI_COMM_WORLD);
            break;
      }
   }
}

// ---------------------------------------------------------------------

int main( int argc, char *argv[] )
{
   int id_propio, num_procesos_actual;

   // inicializar MPI, leer identif. de proceso y número de procesos
   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );

   if ( num_procesos_esperado == num_procesos_actual )
   {
      // ejecutar la operación apropiada a 'id_propio'
      if ( id_propio >= 0 && id_propio < np )
      {
        funcion_productor(id_propio);
	    cout << "Termina el productor: " << id_propio << endl;
      }

      else if ( id_propio == id_buffer )
      {
        funcion_buffer();
      }
      else
      {
        funcion_consumidor(id_propio-np);
        cout << "Termina el consumidor: "<< id_propio << endl;

      }
   }
   else
   {
      if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
      { cout << "el número de procesos esperados es:    " << num_procesos_esperado << endl
             << "el número de procesos en ejecución es: " << num_procesos_actual << endl
             << "(programa abortado)" << endl ;
      }
   }

   // al terminar el proceso, finalizar MPI
   MPI_Finalize( );
   return 0;
}
