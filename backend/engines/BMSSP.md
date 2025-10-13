# Algoritmo BMSSP


## Complejidad Temporal

- **Dijkstra tradicional**: O(m + n log n)
- **BMSSP**: O(m log^(2/3) n) 
- **Mejora**: Significativa para grafos dispersos (sparse graphs)


## Estructura de la Implementación

### Clase BMSSPSolver

#### Métodos Principales:

1. **`findPivots(frontier, B, targetSize)`**
   - Implementa Lemma 3.2 del paper
   - Identifica vértices con árboles de caminos cortos grandes
   - Ejecuta k pasos de Bellman-Ford para calcular tamaños de árbol
   - Selecciona los top targetSize pivotes

2. **`boundedMultiSourceBFS(sources, B, level)`**
   - Core del algoritmo BMSSP
   - Búsqueda desde múltiples fuentes con cota B
   - Usa priority queue parcial (no ordenamiento completo)
   - Retorna nueva frontera y distancia máxima alcanzada

3. **`recursiveBMSSP(S, B, level)`**
   - Implementa Algorithm 3 del paper
   - Procedimiento recursivo principal
   - Pasos:
     1. Encontrar pivotes (reducir frontera)
     2. Ejecutar búsqueda acotada
     3. Verificar condiciones de terminación
     4. Recursión en siguiente nivel

4. **`solve(sources, bound)`**
   - Punto de entrada principal
   - Calcula nivel máximo de recursión: ceil(log n / t)
   - Inicializa estructuras de datos
   - Ejecuta recursiveBMSSP