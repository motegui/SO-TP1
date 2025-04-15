# SO-TP1
Juego ChompChamps desarrollado en lenguaje C, que consta de 3 archivos (master.c, player.c, view.c).

## Autores:
- Matias Rinaldo (60357)
- Germán Delgado (61070)
- María Otegui (61204)

## Requerimientos
Se requiere tener docker instalado para la ejecución del programa

El programa se debe compilar y ejecutar con los siguientes comandos:
```
user@linux:~$ docker pull agodio/itba-so-multi-platform:3.0
user@linux:~$ cd ./path/to/SO-TP1
user@linux:~$ docker run -v "${PWD}:/root" --privileged -ti agodio itba-so-multi-platform:3.0 
root@docker:~$ cd root
```

## Instrucciones de Compilación
Para compilar el programa de manera limpia y sencilla, dentro del contenedor de docker, ejecutar el siguiente:
```
root@docker:~$ make clean && make
```

## Ejecucion del Programa
Para ejecutar el programa, se tiene el siguiente ejemplo:
```
./master -w <int> -h <int> -d <int> -t <int> -v ./view -p ./player <player 2>...<player 9>
```
    
Para buscar fugas de memoria y chequeo de File Descriptors, ejecutar el siguiente comando
```
valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes -s ./master -w <int> -h <int> -d <int> -t <int> -v ./view -p ./player <player 2>...<player 9>
``` 

### Parámetros 
<code>-w</code>: Ancho del tablero (mínimo 10)
<code>-h</code>: Alto del tablero (mínimo 10)
<code>-d</code>: Delay entre turnos (en milisegundos)
<code>-t</code>: Tiempo máximo de espera sin movimientos válidos (timeout, en segundos)
<code>-v</code>: Ruta al ejecutable de la vista
<code>-p</code>: Lista de rutas a los ejecutables de jugadores (mínimo 1, máximo 9)