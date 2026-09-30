[![✗](https://github.com/sofimanzitti/Trabajo-Especial-TLA/actions/workflows/pipeline.yaml/badge.svg?branch=development)](https://github.com/sofimanzitti/Trabajo-Especial-TLA)

# Trabajo Especial TLA — G-147

Frontend (análisis léxico y sintáctico) de **recetas-lang**, un DSL para describir recetas de cocina y combinarlas en menús, escrito con Flex y Bison. El compilador reconoce el lenguaje y construye su AST; automatizar el escalado de porciones y la generación de una lista de compras consolidada es responsabilidad de las próximas etapas del trabajo.

Trabajo Especial de Autómatas, Teoría de Lenguajes y Compiladores (ITBA), grupo G-147: Sol Sommer, Sofía Manzitti, Morena Díaz Macchi y Lucas Crosta. La especificación del Stage I está en [`doc/`](doc), y la sintaxis vigente en la sección [Sintaxis](#sintaxis). Este repositorio está basado en la plantilla [Flex-Bison-Compiler](https://github.com/agustin-golmar/Flex-Bison-Compiler) de la cátedra.

* [Sobre el proyecto](#sobre-el-proyecto)
* [Sintaxis](#sintaxis)
* [Requisitos](#requisitos)
* [Configuración](#configuración)
* [Comandos](#comandos)
* [CI/CD](#cicd)
* [Extensiones recomendadas](#extensiones-recomendadas)

## Sobre el proyecto

`recetas-lang` permite declarar `ingredient`s (con su unidad y, opcionalmente, densidad), `recipe`s (con lo que `requires` y una red de `step`s que declaran qué ingredientes `uses` y de qué otros `step`s `depends`), `substitute`s entre ingredientes, `scale` de una receta a otra cantidad de porciones, `menu`s que `include` recetas escaladas, y `generate shopping list` para un menú.

En esta etapa (Stage II) está hecho el frontend: el analizador léxico (`src/main/c/frontend/lexical-analysis`) y el sintáctico (`src/main/c/frontend/syntactic-analysis`), que arman el AST (`AbstractSyntaxTree.h`/`.c`). El análisis semántico (ingredientes no declarados, ciclos de dependencias, conversión de unidades, etc.) va en el Stage III, así que por ahora se aceptan algunos programas que semánticamente están mal. Por ejemplo, hay tests de aceptación que usan `step`s, recetas o menús que no están declarados en el mismo archivo.

La especificación que está en [`doc/`](doc) es la del Stage I. Después de la devolución cambiamos la sintaxis: sacamos los tipos genéricos, los operadores, las estructuras de control, el `;` al final y la asignación de variables, y los `step`s ahora tienen nombre y dependencias. La sintaxis actual es la de abajo.

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

Los nombres de ingredientes, recetas, pasos y menús pueden tener letras (también con tilde o ñ), números y `_`, pero no pueden empezar con un número ni tener espacios. Por ejemplo: `Azúcar`, `Cumpleaños`, `AceiteDeOliva`. Las palabras clave (`in`, `to`, `list`, `unit`, `cup`, etc.) no se pueden usar como nombres.

### Cantidades y unidades

Las cantidades pueden ser enteros (`4`), decimales (`12.5`), fracciones (`1/2`) o números mixtos (`1 1/2`). Así, "media cucharadita" se escribe `1/2 teaspoon`. Una fracción con denominador `0` da error.

Las unidades son `gram`, `kilogram`, `milliliter`, `liter`, `cup`, `tablespoon`, `teaspoon`, `pinch` y `unit`, y se pueden escribir en singular o en plural. Por ahora no hay forma de escribir cantidades sin número, como "a gusto".

La duración de un `step` va con `takes`, en `minute`/`minutes` o `hour`/`hours` (por ejemplo, `takes 1 1/2 hours`). En el AST se guarda siempre en minutos.

### Escalado

Una receta se puede escalar de dos formas:

* `scale TortaDeManzana to 16 servings`: agrega la receta escalada al HTML de salida. Sirve para cocinar una sola receta sin tener que armar un menú.
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

En el Stage III vamos a chequear que no haya ciclos, que cada `depends on` sea un `step` de la misma receta y que entre todos los `step`s no se use más de lo que dicen los `requires`.

### Recetas como ingredientes

Una receta se puede usar como ingrediente de otra (por ejemplo, un `Caramelo` para un `Flan`). Se pone su nombre en un `requires` o un `uses`, igual que con un ingrediente.

* `serves` es para cuántas personas es la receta. Se usa con `scale` y con `include`.
* `yields` es cuánto sale de la receta (por ejemplo, `yields 200 grams`). Se usa cuando otra receta la pide como ingrediente.
* Si una receta se usa como ingrediente, tiene que tener `yields`. Si otra receta pide `requires 100 gram Caramelo` y el caramelo rinde 200 gramos, se usa la mitad de la receta del caramelo. En este caso `serves` no se tiene en cuenta.
* Una receta puede tener las dos cosas (`serves 4 yields 200 grams`).

Que la receta tenga `yields`, que no haya ciclos entre recetas y que no haya un ingrediente y una receta con el mismo nombre se va a chequear en el Stage III.

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

donde `<program>` es la ruta al archivo que representa su punto de entrada.

### Test

Ejecuta todos los tests unitarios disponibles en la carpeta `src/test/c`:

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
