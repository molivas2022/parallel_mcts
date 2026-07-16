# Paralelización en CPU de Monte Carlo Tree Search


Este repositorio contiene un framework computacional desarrollado en C++ para comparar distintas estrategias de paralelización del algoritmo Monte Carlo Tree Search (MCTS).

Todas las implementaciones son probadas utilizando como entorno el juego de tablero Hex.

---

## Requisitos previos

* Compilador de C++ con soporte para el estándar **C++20** y la API **OpenMP**.
* Herramienta `make` para la compilación.
* Entorno Linux/Unix.
* (Opcional) **Python 3** con las librerías `pandas`, `matplotlib` y `seaborn` para la visualización de resultados mediante `plot.py`.

---

## Instrucciones de ejecución

El proyecto incluye un script automatizado (`run_all.sh`) que compila el código de forma óptima y ejecuta todos los experimentos descritos en el código base.

1. **Otorgar permisos**
```bash
chmod +x run_all.sh
```

2. **Ejecutar los experimentos**
```bash
./run_all.sh
```

### Visualización de resultados

Tras finalizar la ejecución, el programa generará archivos `.csv` (`match_raw.csv` y `oracle_raw.csv`) con los datos recopilados. Puede levantar un menú interactivo para graficar el desempeño ejecutando:

```bash
python3 plot.py
```