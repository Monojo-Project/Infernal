# Contribuir a Infernal

1. Crea una rama para cada cambio lógico.
2. Ejecuta `make test` antes de abrir una propuesta.
3. Para cambios de bajo nivel, ejecuta también `make sanitize` y repite `make test`.
4. Incluye una prueba que reproduzca cada corrección de un error.
5. Mantén los cambios compatibles con C11 y las plataformas POSIX documentadas en el README.

Las incidencias deben indicar versión, plataforma, script mínimo reproducible, salida esperada y salida obtenida. No incluyas secretos ni comandos con datos sensibles.
