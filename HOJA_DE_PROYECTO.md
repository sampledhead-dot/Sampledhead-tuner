# SampledHead Tuner — HOJA DE PROYECTO (v1.03)

## Descripción
App web (PWA, un solo `index.html`) de afinador + detector de acordes, de la serie SampledHead. Pestañas: Afinador (guitarra, bajo, ukelele, banjo, cromático) y Acordes (directo y análisis de archivo, escalas 60+ con favoritas). ES/EN, guía de usuario integrada.

## Arquitectura
- `index.html` (todo incrustado), `manifest.webmanifest`, `sw.js` (caché `sampledhead-tuner-v1.01`, stale-while-revalidate).
- Fuentes de desarrollo: app0 (informe de errores), theory, dsp, app1–app4, style.css; `build.py` los une.
- DSP: YIN por FFT (afinador); picos espectrales → cromagrama con compensación de afinación → plantillas de acordes + peso de bajo + histéresis (directo) / Viterbi (archivo).
- Línea de bajo: paso bajo 6.º orden 400 Hz → diezmado ≈11 kHz → YIN (ventana 0,15 s, salto 0,04 s) → cuantización a semitonos con histéresis → corrección de octava por presencia de fundamental (Goertzel) → notas.
- Repo: `sampledhead-dot/Sampledhead-tuner` (web en raíz; VST3/AU en `vst/`, solo afinador, no afectado). Las apps Windows/Mac/Android de `SH-apps` cargan el contenido del último commit.

## Historial
- **1.04** (24-11-2026): botón ☰ en la cabecera con el **menú común «☰ Menú»** de todas las apps (sh-core 1.5.0, `SH.ui.menu`): mismas secciones desplegables y en el mismo orden — Proyecto · Importar · Exportar · Otras apps · Copia de tus datos · Ajustes · Ayuda (guía e informe .txt) —; recuerda la sección abierta; los botones de siempre siguen haciendo lo mismo (el menú los pulsa); «⇄ Apps» va dentro (Otras apps, con el número de recibidos en el botón ☰).
- **Web v1.03** (19-11-2026): conexión con todas las apps de Sampled Head (sh-core): botón «⇄ Apps» con «Recibido» y «Enviar a…»; en Windows/Mac usa la carpeta Documentos/SampledHead. Lo recibido se analiza en Acordes (archivo).
- **Web v1.02** (19-11-2026) — zona segura: la cabecera (y, donde las hay, las ventanas a pantalla completa y la barra inferior) deja libre la franja de la hora, la batería y la cámara del móvil con `env(safe-area-inset-*)`. Comprobado en Chrome simulando un móvil con 48 px arriba y 24 px abajo: ningún botón tapado. En ordenador no cambia nada. La barra de pestañas pegajosa baja lo mismo.
- 1.00: afinador, detector de acordes, escalas, ES/EN, guía.
- 1.01: línea de bajo (piano-roll, lista, notas más usadas, escuchar, exportación); selector de afinación (p. ej. Open D) en los diagramas de acordes; informe de errores .txt automático + botón «Descargar informe (.txt)»; pie con versión; ajustes de maquetación en escritorio.

## Reglas permanentes
- Versión en el pie; entrega `sampledhead-tuner-vN.zip` con carpeta, standalone y esta hoja.
- Informe de error automático y manual (sin contenido de archivos).
- Probar en navegador headless y decir qué no se probó.

## Límites conocidos
- El bajo es monofónico (una nota cada vez); duda en cambios de acorde; octavas es lo más delicado. En bajos sintéticos acierta 80–90 % según el caso; con música real no medido.
- Líneas de bajo caminantes pueden confundir el nombre del acorde (motor previo, sin cambios).
- No probado en dispositivo real, instrumentos reales ni en las compilaciones Windows/Mac/Android.
