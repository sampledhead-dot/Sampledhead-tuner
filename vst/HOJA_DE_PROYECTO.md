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
- **1.0.0** (18-11-2026): primera versión del plugin.

## Límites conocidos
- La pestaña **Acordes** de la web no está en el plugin (detección polifónica: pendiente).
- Monofónico: afina una cuerda cada vez.
- Sin firma de código (avisos de Windows/macOS; ver LEEME.txt).
- Probado solo en Linux; Windows/Mac se compilan en GitHub Actions. Las máquinas Mac de GitHub no se estaban asignando
  a esta cuenta el 18-11-2026: el zip de Mac se publicará cuando GitHub las asigne.
- No probado dentro de un DAW real.
