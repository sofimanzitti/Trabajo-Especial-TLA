[![✗](https://github.com/sofimanzitti/Trabajo-Especial-TLA/actions/workflows/pipeline.yaml/badge.svg?branch=development)](https://github.com/sofimanzitti/Trabajo-Especial-TLA)

# Trabajo Especial TLA — G-147

Compilador de **recetas-lang**, un DSL para describir recetas de cocina y combinarlas en menús, escrito en C con Flex y Bison. Reconoce el lenguaje, construye su AST, valida que el programa tenga sentido en el dominio (análisis semántico) y genera los artefactos finales: una lista de compras consolidada en CSV y las recetas escaladas en HTML.

Trabajo Especial de Autómatas, Teoría de Lenguajes y Compiladores (ITBA), grupo G-147: Sol Sommer, Sofía Manzitti, Morena Díaz Macchi y Lucas Crosta. El informe final del Stage III está en [`doc/`](doc), y la sintaxis vigente en la sección [Sintaxis](#sintaxis). Este repositorio está basado en la plantilla [Flex-Bison-Compiler](https://github.com/agustin-golmar/Flex-Bison-Compiler) de la cátedra.

* [Sobre el proyecto](#sobre-el-proyecto)
* [Sintaxis](#sintaxis)
* [Análisis semántico](#análisis-semántico)
* [Artefactos generados](#artefactos-generados)
* [Requisitos](#requisitos)
* [Configuración](#configuración)
* [Comandos](#comandos)
* [CI/CD](#cicd)
* [Extensiones recomendadas](#extensiones-recomendadas)

## Sobre el proyecto

`recetas-lang` permite declarar `ingredient`s (con su unidad y, opcionalmente, densidad), `recipe`s (con lo que `requires` y una red de `step`s que declaran qué ingredientes `uses` y de qué otros `step`s `depends`), `substitute`s entre ingredientes, `scale` de una receta a otra cantidad de porciones, `menu`s que `include` recetas escaladas, y `generate shopping list` para un menú.

El compilador tiene tres fases:

* **Frontend** (`src/main/c/frontend`): el analizador léxico (`lexical-analysis`) y el sintáctico (`syntactic-analysis`), que arman el AST (`AbstractSyntaxTree.h`/`.c`).
* **Análisis semántico** (`src/main/c/backend/semantic-analysis`): recorre el AST, arma la tabla de símbolos y valida las reglas del dominio. Ver [Análisis semántico](#análisis-semántico).
* **Generación de código** (`src/main/c/backend/code-generation`): escala las recetas, consolida los ingredientes de un menú y escribe los archivos de salida. Ver [Artefactos generados](#artefactos-generados).

Si una fase rechaza el programa, las siguientes no se ejecutan y el compilador termina con estado distinto de `0`.

Respecto de la especificación del Stage I, después de la devolución cambiamos la sintaxis: sacamos los tipos genéricos, los operadores, las estructuras de control, el `;` al final y la asignación de variables, y los `step`s ahora tienen nombre y dependencias. La sintaxis actual es la de abajo.

## Sintaxis

Un programa es una lista de sentencias, sin separador. Los comentarios empiezan con `//` y van hasta el final de la línea. Las palabras clave van en minúscula.

| Sentencia | Forma |
| :--- | :--- |
| Ingrediente | `ingredient <Nombre> in <unidad> [density <número>]` |
| Receta | `recipe <Nombre> [serves <número>] [yields <número> <unidad>] { <requires>* <step>* }` |
| Requerimiento | `requires <número> [<unidad>] <Nombre>` |
| Paso | `step <Nombre> [uses <número> [<unidad>] <Nombre>, ...] [depends on <Nombre>, ...] [takes <número> minutes\|hours] { "<descripción>" }` |
| Sustitución | `substitute <Nombre> with <Nombre> ratio <número>` |
| Escalado | `scale <Receta> to <número> servings` |
| Menú | `menu <Nombre> { <include <Receta> scaled to <número> servings>* }` |
| Lista de compras | `generate shopping list for <Menú>` |

Lo que está entre corchetes es opcional y `*` quiere decir cero o más veces. Adentro de una receta van primero todos los `requires` y después los `step`s, y las partes opcionales de `recipe` y `step` tienen que ir en el orden de la tabla. Si en un `requires` o un `uses` no se pone unidad, se usa la del ingrediente. La descripción de un `step` es un string de una sola línea y no admite comillas adentro.

Ejemplo:

```
ingredient Harina in grams
ingredient Manteca in grams
ingredient Huevo in units
ingredient AceiteDeOliva in milliliters density 0.92

recipe TortaDeManzana serves 8 {
    requires 300 Harina
    requires 150 Manteca
    requires 4 Huevo
    step Mezclar uses 300 Harina, 150 Manteca takes 5 minutes { "Mezclar la harina con la manteca" }
    step Batir uses 4 Huevo depends on Mezclar takes 3 minutes { "Agregar los huevos y batir" }
    step Hornear depends on Batir takes 40 minutes { "Hornear la mezcla" }
}

substitute Manteca with AceiteDeOliva ratio 0.75

menu Cumpleaños {
    include TortaDeManzana scaled to 16 servings
}

generate shopping list for Cumpleaños
```

Cuando un programa se rechaza, el compilador muestra la línea y qué esperaba encontrar, por ejemplo: `Line 2: syntax error, unexpected }, expecting description.`

### Nombres

Los nombres de ingredientes, recetas, pasos y menús pueden tener letras latinas (también con tilde o ñ), números y `_`, pero no pueden empezar con un número, tener espacios ni superar los 100 caracteres. Por ejemplo: `Azúcar`, `Cumpleaños`, `AceiteDeOliva`. Las palabras clave (`in`, `to`, `list`, `unit`, `cup`, etc.) no se pueden usar como nombres.

### Cantidades y unidades

Las cantidades pueden ser enteros (`4`), decimales (`12.5`), fracciones (`1/2`) o números mixtos (`1 1/2`). Así, "media cucharadita" se escribe `1/2 teaspoon`. Una fracción con denominador `0`, un número mayor que 1.000.000.000 o un número pegado a letras (`300Harina`) dan error.

Las unidades son `gram`, `kilogram`, `milliliter`, `liter`, `cup`, `tablespoon`, `teaspoon`, `pinch` y `unit`, y se pueden escribir en singular o en plural. Por ahora no hay forma de escribir cantidades sin número, como "a gusto".

La duración de un `step` va con `takes`, en `minute`/`minutes` o `hour`/`hours` (por ejemplo, `takes 1 1/2 hours`). En el AST se guarda siempre en minutos.

### Escalado

Una receta se puede escalar de dos formas:

* `scale TortaDeManzana to 16 servings`: escala una receta sin tener que armar un menú. Esto produce la receta escalada como archivo HTML.
* `include TortaDeManzana scaled to 16 servings`, dentro de un `menu`: escala la receta como parte del menú. Esto es lo que se usa para `generate shopping list`.

El lenguaje no tiene variables. Las recetas, ingredientes y menús tienen nombre y se usan por ese nombre, así que algo como `TortaGrande = scale ...` da error.

### Pasos

Los `step`s de una receta no son una lista en orden, sino un grafo. Cada `step` dice cuánto usa de cada ingrediente (`uses`) y de qué otros `step`s depende (`depends on`). Así se pueden armar varias preparaciones por separado y después juntarlas, como en este budín con masa, relleno y glaseado:

```
step Masa uses 250 Harina, 150 Manteca { "Batir la manteca e integrar la harina" }
step Relleno uses 100 Chocolate, 50 Nueces { "Picar el chocolate y las nueces" }
step Glaseado uses 200 AzucarImpalpable, 30 Limon { "Mezclar el azucar con el jugo de limon" }
step Unir depends on Masa, Relleno { "Incorporar el relleno a la masa" }
step Hornear depends on Unir takes 40 minutes { "Hornear en molde de budin" }
step Glasear depends on Hornear, Glaseado { "Cubrir el budin frio con el glaseado" }
```

El análisis semántico chequea que no haya ciclos, que cada `depends on` sea un `step` de la misma receta y que entre todos los `step`s no se use más de lo que dicen los `requires`.

### Recetas como ingredientes

Una receta se puede usar como ingrediente de otra (por ejemplo, un `Caramelo` para un `Flan`). Se pone su nombre en un `requires` o un `uses`, igual que con un ingrediente.

* `serves` es para cuántas personas es la receta. Se usa con `scale` y con `include`.
* `yields` es cuánto sale de la receta (por ejemplo, `yields 200 grams`). Se usa cuando otra receta la pide como ingrediente.
* Si una receta se usa como ingrediente, tiene que tener `yields`. Si otra receta pide `requires 100 gram Caramelo` y el caramelo rinde 200 gramos, se usa la mitad de la receta del caramelo. En este caso `serves` no se tiene en cuenta.
* Una receta puede tener las dos cosas (`serves 4 yields 200 grams`).

Que la receta tenga `yields`, que no haya ciclos entre recetas y que no haya un ingrediente y una receta con el mismo nombre también lo chequea el análisis semántico.

## Análisis semántico

Todo se tiene que declarar antes de usarse (en el orden del archivo), así que una receta no puede pedir otra que se declara más abajo. Los errores se informan todos juntos, con su línea, por ejemplo: `Line 4: requires in recipe 'Torta': 'Harina' is not a declared ingredient or recipe.`

| Qué se chequea | Detalle |
| :--- | :--- |
| Nombres | Un ingrediente y una receta no pueden compartir nombre ni repetirse. Los menús tienen su propio espacio de nombres. |
| Cantidades | `serves`, `yields`, `density`, `ratio`, `takes`, cada cantidad y cada cantidad de porciones tienen que ser mayores que cero. |
| `requires` | El ingrediente o la receta tiene que estar declarado, no puede repetirse en la misma receta y una receta no puede requerirse a sí misma. |
| Unidades | Las unidades de una misma dimensión (masa, volumen) se convierten solas. Pasar de masa a volumen solo se puede si el ingrediente declara `density` (en g/ml). Las `unit` no se convierten a nada. Una receta usada como ingrediente solo acepta unidades de la dimensión de su `yields`. |
| Recetas como ingredientes | Tienen que declarar `yields`. |
| `step` | Nombres únicos dentro de la receta, `depends on` solo de `step`s de esa receta, sin ciclos (ni de un `step` consigo mismo), y `uses` solo de lo que la receta `requires`. |
| Totales | Lo que usan entre todos los `step`s de un ingrediente no puede superar lo que dice el `requires` (las unidades se convierten antes de sumar). Que sobre sí está permitido. |
| `substitute` | Solo se aplica a los `generate shopping list` que vienen después. Los dos tienen que ser ingredientes declarados y distintos, sin regla repetida y sin ciclos (de cualquier largo). Si tienen distinta dimensión, alguno de los dos tiene que declarar `density`. |
| `scale` e `include` | La receta tiene que estar declarada y declarar `serves`. No se puede repetir un `scale` de la misma receta si las porciones se escriben igual en el nombre del archivo (por ejemplo, 7.996 y 8.004), porque escribiría el mismo archivo. |
| `generate shopping list` | El menú tiene que estar declarado. |

Si una receta tiene `step`s pero uno de sus `requires` no lo usa ningún `step`, el compilador avisa con un warning, pero no la rechaza. Lo mismo pasa con un `density` en un ingrediente medido en `unit`, que no tiene sentido.

## Artefactos generados

El compilador lee el programa por entrada estándar y escribe los archivos en un directorio (por defecto `out/`, o el que se pase como primer argumento: `src/main/bash/run.sh programa.txt mi-directorio`).

| Sentencia | Archivo |
| :--- | :--- |
| `scale <Receta> to <N> servings` | `<Receta>-<N>-servings.html`: la receta escalada, con sus pasos en orden de dependencia. |
| `generate shopping list for <Menú>` | `<Menú>-shopping-list.csv`: la lista de compras consolidada. Y `<Menú>-recipes.html`: las recetas del menú, ya escaladas. |

La lista de compras tiene las columnas `ingrediente,cantidad,unidad,alternativas`, ordenada por nombre. Cada cantidad está en la unidad con la que se declaró el ingrediente, y los ingredientes que se repiten entre recetas se suman. Si una receta se usa como ingrediente, se expande en proporción a lo que se pide respecto de lo que rinde y se suma al resto. Las cantidades de `unit` se redondean para arriba, porque no se compra media unidad. Si hay un `substitute` para un ingrediente, la columna `alternativas` indica cuánto habría que comprar del sustituto (ya multiplicado por el `ratio` y convertido a su unidad), por ejemplo `AceiteDeOliva: 468.75 mililitros`.

El CSV está en UTF-8 sin BOM, separado por comas y con punto decimal. En una planilla configurada en español hay que importarlo indicando esas opciones (si no, puede abrirse en una sola columna o mostrar mal los acentos). Cada HTML lleva su CSS adentro, así que solo hace falta un navegador.

Los archivos HTML muestran, además de los ingredientes y pasos escalados (y, si la receta usa otras recetas como ingredientes, también esas recetas auxiliares en la proporción necesaria), el tiempo total de trabajo y el tiempo mínimo si los `step`s independientes se hacen en paralelo (el camino crítico del grafo de dependencias). Los textos de salida están en español.

## Requisitos

* [Docker v28.3.2](https://www.docker.com/)

## Configuración

Configurá las siguientes variables de entorno para controlar el comportamiento de la aplicación:

| Nombre                | Default | Descripción                                                                                                                                                          |
| :--------------------- | :-----: | :--------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `ENVIRONMENT`         | `Local` | El nombre del entorno activo. Los entornos disponibles son: `Local`, `Development` y `Production`.                                                                    |
| `LOG_IGNORED_LEXEMES` | `true`  | Cuando es `true`, loguea todos los lexemas ignorados por Flex en nivel `DEBUGGING`. Para sacar esos logs de la consola, poné `false`.                                 |
| `LOGGING_LEVEL`       | `ALL`   | El nivel mínimo a loguear en consola. De menor a mayor, los niveles disponibles son: `ALL`, `DEBUGGING`, `INFORMATION`, `WARNING`, `ERROR` y `CRITICAL`.              |

_Docker Compose_ también puede leer estas variables desde un archivo `.env` (ver `compose.yaml`).

## Comandos

### Start

Levanta un contenedor efímero, listo para empezar a desarrollar:

```bash
docker compose run --rm compiler
```

### Build

Compila (o recompila) el compilador completo:

```bash
src/main/bash/build.sh
```

### Run

Compila un programa:

```bash
src/main/bash/run.sh <program>
```

donde `<program>` es la ruta al archivo que representa su punto de entrada. Opcionalmente, se puede agregar el directorio donde se escriben los [artefactos](#artefactos-generados).

### Test

Ejecuta todos los tests disponibles en la carpeta `src/test/c`. En `accept` están los programas que se tienen que aceptar y en `reject` los que se tienen que rechazar (ya sea por sintaxis o por semántica). En `output` hay programas con los archivos que se esperan de ellos (`input` y `expected/`):

```bash
src/main/bash/test.sh
```

### Stop

Cierra sesión, destruye los contenedores efímeros y apaga el cluster:

```bash
exit
docker compose down
```

### Docker

| Comando                                 | Descripción                                                 |
| :--------------------------------------- | :----------------------------------------------------------- |
| `docker builder prune --all`            | Elimina todos los builds y toda la cache de build.           |
| `docker compose --progress=plain build` | Fuerza un build (o rebuild) de las imágenes del cluster.     |
| `docker image prune`                    | Elimina todas las imágenes huérfanas de Docker.               |
| `docker network prune`                  | Elimina las redes de Docker que no se estén usando.           |
| `docker volume prune`                   | Elimina los volúmenes de Docker que no se estén usando.       |

## CI/CD

Para disparar una integración automática en cada push o PR (_Pull Request_), hay que activar _GitHub Actions_ en la pestaña _Settings_. Usar la siguiente configuración:

| Clave                                                       | Valor                                                |
| :----------------------------------------------------------- | :----------------------------------------------------- |
| `Actions permissions`                                       | `Allow all actions and reusable workflows`            |
| `Allow GitHub Actions to create and approve pull requests`  | `false`                                               |
| `Artifact and log retention`                                | `30 days`                                             |
| `Fork pull request workflows from outside collaborators`    | `Require approval for all outside collaborators`      |
| `Workflow permissions`                                       | `Read repository contents and packages permissions`  |

## Extensiones recomendadas

* [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)
* [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)
* [Yash](https://marketplace.visualstudio.com/items?itemName=daohong-emilio.yash)
