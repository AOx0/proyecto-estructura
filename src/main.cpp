#include <iostream>
#include <vector>

#include "lib/option.hpp"
#include "lib/result.hpp"

using namespace std;

/// Funcion que intenta pedir un numero al usuario en un rango.
/// En caso de ingresar un numero que no está en rango, se devolverá un mensaje de error.
/// En caso de exito, se devuelve el valor ingresado.
Result<int, string> try_range(int from, int to) {
  int result;
  cin >> result;

  if (result >= to || result < from) {
    return Result<int, string>::Err("Error: El valor no está en el rango especificado");
  } else {
    return Result<int, string>::Ok(result);
  }
}

int main() {
  DB db = DB();

  // Option: Al igual que en rust puede ser un Some o un None.
  // Nos sirve para respresentar la ausencia de un dato de forma explicita,
  // así podemos hacer un mejor manejo de ausencia de datos.
  Option<int> v = Option<int>::Some(4);

  // Lo siguiente crashearía porque no hubo un chequeo de status con v.is_some() o v.is_none()
  // cout << "El valor es " << v.inner() << endl;

  if (v.is_some()) {
    cout << "El valor es " << v.inner() << endl;
  } else {
    cout << "No hay valor" << endl;
  }

  // Result es una versión mejorada de Opcion que permite representar la ausencia de estado junto con
  // un dato que indique de alguna manera qué falló.
  // En este caso la funcion try_range puede devolver un int en caso de exito o un
  // string con un mensaje que comunica la causa del error.
  Result<int, string> resultado = try_range(10, 20);

  if (resultado.is_ok()) {
    cout << "Ingresaste el valor: " << resultado.inner() << endl;
  } else
    cout << resultado.inner_err() << endl;

  return 0;
}

