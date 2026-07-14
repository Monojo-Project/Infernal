# API y formato de módulos

## Módulos `.fire`

Un módulo `.fire` se compila dentro del ejecutable desde `config/infernal/fire`. Se importa por su nombre sin la extensión (`import test` para `test.fire`). Las funciones declaradas en el módulo se registran con el prefijo del módulo para evitar colisiones.

## Binarios embebidos

Los archivos de `config/infernal/bins` se convierten en datos binarios y se invocan con `!nombre argumentos!`. El intérprete extrae cada ejecución a un archivo temporal con permisos `0700`, valida la escritura completa y elimina el archivo después de ejecutarlo.

## Compatibilidad

La API de módulos es la sintaxis `.fire` y el nombre del binario; no existe todavía una ABI C pública para extensiones dinámicas. Los módulos externos deben distribuirse recompilando el ejecutable con la misma edición del intérprete. Los cambios incompatibles se registran en `CHANGELOG.md`.
