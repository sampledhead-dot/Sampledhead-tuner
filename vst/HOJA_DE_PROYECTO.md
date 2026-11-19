# SampledHead Tuner VST — Hoja de proyecto

**Versión actual:** 1.0.0 (18-11-2026) · descargas en Releases (`vst-v1.0.0`)

## Descripción
Versión plugin del afinador SampledHead Tuner (la web está en la raíz de este repositorio). Efecto VST3 (Windows/Mac),
AU (Mac) y app Standalone. Se inserta en la pista del instrumento: el audio pasa sin cambios (o se silencia) y la
interfaz muestra nota, cents, Hz y una aguja de ±50 cents.

## Funciones (igual que la pestaña «Afinador» de la web)
- Instrumentos: Guitarra, Bajo, Ukelele, Banjo y Cromático.
- Las **52 afinaciones** de la web (generadas desde su lista `TUN`), capo 0–12.
- Cuerda **automática** (la más cercana) o **manual** (pulsando la cuerda).
- La de referencia 415–466 Hz; tolerancia (zona verde) 1–10 cents; puerta de ruido −80…−20 dB.
- Nombres C D E o Do Re Mi; alteraciones auto / ♯ / ♭.
- **Tono de referencia** (seno a −18 dBFS de la cuerda elegida o la última detectada) y **Silenciar salida**.
- Todo es parámetro del plugin: se guarda con el proyecto del DAW y es automatizable.
- **Guardar informe (.txt)** en Documentos/SampledHead Tuner/ (versión, host, formato, sistema, audio, última lectura,
  parámetros y registro).

## Arquitectura
- `Source/TunerDSP.h` — detector YIN causal con correlación por FFT (mismo algoritmo que FuzzHead), rango 26 Hz–4,2 kHz
  (La0 a Do8), análisis cada ~10 ms, interpolación parabólica y refinado sobre varios periodos en notas agudas.
- `Source/TunerLogic.h` — cuerda objetivo, cents y nombres de nota (sin audio; lo usan procesador, interfaz y pruebas).
- `Source/Tunings.h` — tabla de afinaciones (generada desde la web).
- `Source/PluginProcessor.*` — parámetros, mediana de 5 lecturas + suavizado, paso de audio, silencio y tono de referencia.
- `Source/PluginEditor.*` — interfaz con la estética de la web (azul noche y verde azulado).
- `tests/TestMain.cpp` — pruebas offline; con `TUNER_VST3=ruta` además carga el `.vst3` compilado como un DAW.
- `../.github/workflows/build-vst.yml` — pruebas en Linux y compilación Windows/Mac; publica zips en Releases.

## Pruebas (18-11-2026, Linux)
- 278/278 cuerdas de las 52 afinaciones detectadas con la nota correcta; error máximo 0,06 cents (seno).
- Cuerdas pulsadas simuladas: error < 0,02 cents respecto a la afinación real de la señal.
- Cromático C1–C8 15/15; La 432; capo; modo manual; silencio; tono de referencia 82,41 Hz; puerta; paso directo exacto.
- Estado guardado/restaurado; ~19× tiempo real en un núcleo; informe; captura del editor.
- `.vst3` escaneado, instanciado, con audio procesado y editor abierto desde un host JUCE.

## Reglas permanentes
- Versionado semántico: `project(... VERSION)` en `CMakeLists.txt` y `kVersion` en `PluginProcessor.h` (iguales).
- Si cambian las afinaciones de la web, regenerar `Tunings.h` desde la lista `TUN` de `index.html`.
- Probar antes de entregar con `TunerTests` (y la carga del `.vst3`).

## Historial
- **v1.1.0** (18-11-2026): aviso de versión nueva en la ventana del plugin.
- **1.0.0** (18-11-2026): primera versión del plugin.

## Límites conocidos
- La pestaña **Acordes** de la web no está en el plugin (detección polifónica: pendiente).
- Monofónico: afina una cuerda cada vez.
- Sin firma de código (avisos de Windows/macOS; ver LEEME.txt).
- Probado solo en Linux; Windows/Mac se compilan en GitHub Actions. Las máquinas Mac de GitHub no se estaban asignando
  a esta cuenta el 18-11-2026: el zip de Mac se publicará cuando GitHub las asigne.
- No probado dentro de un DAW real.

## Aviso de versión nueva (v1.1.0)
`Source/UpdateBanner.h` (común a los plugins de Sampled Head):
- Solo con la ventana abierta (nunca al escanear ni en el hilo de audio) y como mucho una vez al día por ordenador. Lo guarda en `<datos de usuario>/SampledHead/<Plugin>.settings`: `%APPDATA%` en Windows, `~/Library/Application Support` en Mac.
- Pide a la API pública de GitHub la lista de releases del repo y elige la más alta con etiqueta `vst-vX.Y.Z`, sin contar borradores ni pre-releases.
- Si es más nueva que la instalada, aparece una franja abajo a la derecha con «Descargar» (abre la release en el navegador) y «✕» (no volver a avisar de esa versión).
- El plugin no se reemplaza solo: el DAW lo tiene cargado y está en una carpeta del sistema.
- Usa la red del sistema: WinINet en Windows y NSURLSession en Mac. En Linux (solo pruebas) está sin curl y no consulta nada.
- Pruebas: `SH_UPDATE_FAKE=<json>` y `SH_UPDATE_SETTINGS=<carpeta>`. Las pruebas cubren comparar versiones, elegir la release, que el aviso aparezca, que «✕» lo oculte y que con la misma versión no avise.
- **Importante al publicar:** la versión del plugin está en `CMakeLists.txt` (`project(... VERSION x.y.z)`, que también da nombre a la release) y en `kVersion` del procesador. Hay que cambiar las dos.

## Mac (19-11-2026)
- Los plugins para Mac se compilan en las máquinas **Intel** de GitHub (`macos-15-intel`). El binario es **universal** (Intel + Apple Silicon), con VST3, AU y Standalone y firma ad-hoc.
- Con las máquinas Apple Silicon (`macos-15`) las tareas nunca empezaban. GitHub lo explicaba en las anotaciones de cada tarea: «The job was not acquired by Runner of type hosted even after multiple attempts» y «Due to capacity constraints, jobs targeting macOS arm64 runners may experience longer queue times». Era falta de capacidad de GitHub, no un problema de la cuenta.
- Ahora se compila para Mac siempre. Al lanzarlo a mano se puede desmarcar «mac».
- Sin certificado de Apple (Developer ID, 99 €/año), la primera vez macOS pide permiso: Ajustes del Sistema → Privacidad y seguridad → «Abrir igualmente», o `xattr -cr` sobre el plugin.
- No probado en un Mac real ni en Logic, Ableton u otro DAW de Mac.
