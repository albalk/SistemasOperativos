# SISTEMASENTREGA1
Entrega 1 de S.operativos por Alba López y Óscar Marcos


Las cabeceras de las funciones son distintas debido a que en nuestro codigo almacena la etiqueta, la linea, bloque y palabra en la misma varibale "direccion" y la varibale direcciones cortadas la pasamos entre las funciones anidadas para ir modificando la informacion obtenida y terminar metiendola en la variable direccion.

Encontramos problemas con la funcion math.h en linux, mediante la busqueda en internet encontramos que en especifico esta libreia en ubuntu da error, y que se soluciona añadiendo -lm al final del codigo de compilacion. FUENTE: https://xbalban.wordpress.com/2014/08/16/no-compila-error-con-libreria-math-h-en-gnulinux-solucion/ 
///gcc MEMsym.c -o MEMsym.exe -lm///

A la hora de ejecutar el programa, se printean por pantalla los aciertos y los fallos, pero en vez de mostrarse en una linea, se divide en dos. Hemos mirado en el código y no hay ningún \n o \t que pueda dividir la frase en dos. Quitando esto, funciona bien.

Todo el proyecto fue realizado por los dos integrantes a la vez, ibamos alternando quien escribia el codigo y quein escribia el commit correspondiente, pero TODO esta dessarrollado con las ideas de los integrantes
