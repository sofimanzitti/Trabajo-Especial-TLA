[![✗](https://github.com/sofimanzitti/Trabajo-Especial-TLA/actions/workflows/pipeline.yaml/badge.svg?branch=development)](https://github.com/sofimanzitti/Trabajo-Especial-TLA)

# Trabajo Especial TLA — G-147

Frontend (análisis léxico y sintáctico) de **recetas-lang**, un DSL para describir recetas de cocina y combinarlas en menús, escrito con Flex y Bison. El compilador reconoce el lenguaje y construye su AST; automatizar el escalado de porciones y la generación de una lista de compras consolidada es responsabilidad de las próximas etapas del trabajo.

Trabajo Especial de Autómatas, Teoría de Lenguajes y Compiladores (ITBA), grupo G-147: Sol Sommer, Sofía Manzitti, Morena Díaz Macchi y Lucas Crosta. La especificación completa del lenguaje está en [`doc/`](doc). Este repositorio está basado en la plantilla [Flex-Bison-Compiler](https://github.com/agustin-golmar/Flex-Bison-Compiler) de la cátedra.

* [Sobre el proyecto](#sobre-el-proyecto)
* [Requisitos](#requisitos)
* [Configuración](#configuración)
* [Comandos](#comandos)
* [CI/CD](#cicd)
* [Extensiones recomendadas](#extensiones-recomendadas)

## Sobre el proyecto

`recetas-lang` permite declarar `ingredient`s (con su unidad y, opcionalmente, densidad), `recipe`s (con lo que `requires` y una serie de `step`s que declaran qué ingredientes `uses` y de qué otros `step`s `depends`), `substitute`s entre ingredientes, `scale` de una receta a otra cantidad de porciones, `menu`s que `include` recetas escaladas, y `generate shopping list` para un menú. El detalle de la gramática y las decisiones de diseño (por qué no hay tipos genéricos, por qué `requires` es un diccionario y no un set, etc.) está documentado en la Especificación bajo [`doc/`](doc).

Esta etapa (Stage II) cubre únicamente el frontend: el analizador léxico (`src/main/c/frontend/lexical-analysis`) y sintáctico (`src/main/c/frontend/syntactic-analysis`) que arman el AST (`AbstractSyntaxTree.h`/`.c`). El análisis semántico (ingredientes no declarados, ciclos de dependencias, conversión real de unidades, etc.) queda para Stage III — por diseño, algunos programas semánticamente inválidos son aceptados en esta etapa.

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
