# DirectX 11 Engine con Sistema de Cámara Dual (Primera/Tercera Persona)

Un motor gráfico avanzado desarrollado en C++ usando DirectX 11 con un sistema completo de cámara 3D que soporta tanto primera como tercera persona, con arquitectura de game loop separada para control preciso de timing.

## Características

- **Inicialización completa de DirectX 11**: Device, Device Context, Swap Chain, Render Target View, Depth Stencil
- **Sistema de cámara 3D dual**:
  - **Modo Primera Persona**: Control directo estilo FPS
  - **Modo Tercera Persona**: Cámara orbital alrededor del personaje
  - Cambio dinámico entre modos con tecla 'C'
  - Movimiento con WASD + QE (cámara o personaje según modo)
  - Control de vista con mouse
  - Zoom con rueda del mouse (tercera persona)
  - Matrices de vista y proyección optimizadas
  - Restricciones de pitch y límites de zoom
- **Sistema de renderizado**:
  - Vertex y Pixel Shaders inline
  - Constant Buffers para matrices
  - Renderizado de primitivas (triángulo y cubo)
  - Rotación automática de objetos
- **Gestión de ventana Win32**
- **Loop principal del engine con timing preciso**

## Estructura del Proyecto

```
dx11-engine/
├── Engine/
│   ├── Engine.h/.cpp      # Clase principal del motor
│   ├── Camera.h/.cpp      # Sistema de cámara 3D
│   └── Renderer.h/.cpp    # Sistema de renderizado
├── main.cpp               # Punto de entrada
├── CMakeLists.txt         # Configuración de build
└── README.md             # Documentación
```

## Requisitos

- Windows 10/11
- Visual Studio 2019+ o cualquier compilador compatible con C++17
- Windows SDK (para DirectX 11)
- CMake 3.20+

## Compilación

### Usando CMake

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### Usando Visual Studio

1. Abrir "Developer Command Prompt for VS"
2. Navegar al directorio del proyecto
3. Ejecutar:
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019"
```
4. Abrir el archivo `.sln` generado en Visual Studio
5. Compilar (Ctrl+Shift+B)

## Controles

- **WASD**: Mover cámara adelante/atrás/izquierda/derecha
- **Q/E**: Mover cámara arriba/abajo
- **Click izquierdo + Mover mouse**: Mirar alrededor (estilo FPS)
- **ESC**: Salir de la aplicación

## Arquitectura del Engine

### Engine Class
La clase principal que maneja:
- Inicialización de DirectX 11
- Creación de ventana Win32
- Loop principal del juego
- Gestión de mensajes de Windows

### Camera Class
Sistema de cámara 3D que incluye:
- Matrices de vista y proyección
- Vectores de dirección (forward, up, right)
- Controles de movimiento suavizados
- Rotación con restricciones de pitch

### Renderer Class
Sistema de renderizado que maneja:
- Compilación de shaders inline
- Creación de buffers (vertex, index, constant)
- Renderizado de primitivas
- Gestión de estados de DirectX

## Shaders

El engine incluye shaders básicos inline:

**Vertex Shader**:
- Transformación MVP (Model-View-Projection)
- Paso de colores por vértice

**Pixel Shader**:
- Renderizado de colores interpolados

## Primitivas Incluidas

1. **Triángulo**:
   - 3 vértices con colores RGB
   - Posicionado a la izquierda
   - Rotación en Y

2. **Cubo**:
   - 8 vértices, 12 triángulos
   - Colores diferentes por cara
   - Posicionado a la derecha
   - Rotación en múltiples ejes

## Extensiones Posibles

- Sistema de materiales y texturas
- Carga de modelos 3D (OBJ, FBX)
- Sistema de iluminación (Phong, PBR)
- Sistema de partículas
- Post-procesamiento
- Sistema de físicas
- Culling y optimizaciones
- Sistema de recursos
- Editor de escenas

## Notas Técnicas

- Utiliza DirectX Math Library para operaciones matriciales
- Implementa depth testing para renderizado 3D correcto
- Gestión automática de memoria para objetos DirectX
- Timing preciso usando QueryPerformanceCounter
- Compatible con Windows 10/11

## Troubleshooting

**Error de compilación "DirectX not found"**:
- Asegúrate de tener Windows SDK instalado
- Verifica que las rutas de DirectX estén en PATH

**Ventana negra o crash al iniciar**:
- Verifica que tu GPU soporte DirectX 11
- Comprueba que los drivers gráficos estén actualizados

**Rendimiento bajo**:
- El engine está optimizado para aprendizaje, no para rendimiento máximo
- Para producción, implementar culling y batching
