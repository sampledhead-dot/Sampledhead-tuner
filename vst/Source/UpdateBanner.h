// Aviso de versión nueva del plugin (común a los plugins de Sampled Head).
//
// - Solo se comprueba con la ventana del plugin abierta (nunca al escanear plugins ni en el hilo de audio) y como mucho
//   una vez al día por ordenador: el resultado se guarda en <datos de usuario>/SampledHead/<Plugin>.settings.
// - Consulta pública a GitHub (sin cuenta): la última release "vst-vX.Y.Z" del repo del plugin.
// - Si hay una versión más nueva, aparece una franja con «Descargar» (abre la página de la release en el navegador)
//   y «✕» (no volver a avisar de esa versión). El plugin no se actualiza solo: el DAW lo tiene cargado.
// - Sin conexión o si algo falla, no se muestra nada y el plugin funciona igual.
// Pruebas: SH_UPDATE_FAKE=<archivo.json> usa ese JSON en vez de GitHub; SH_UPDATE_SETTINGS=<carpeta> cambia dónde se guarda.
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace shupdate
{
inline juce::String utf8 (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); }

/** ¿b es una versión más nueva que a? (1.0.10 > 1.0.9) */
inline bool newer (const juce::String& a, const juce::String& b)
{
    auto pa = juce::StringArray::fromTokens (a, ".", ""), pb = juce::StringArray::fromTokens (b, ".", "");
    for (int i = 0; i < 3; ++i)
    {
        const int x = pa[i].getIntValue(), y = pb[i].getIntValue();
        if (x != y) return y > x;
    }
    return false;
}

struct Latest { juce::String version, url; };

/** De la lista de releases de GitHub, la versión más alta con etiqueta vst-vX.Y.Z (sin borradores ni pre-releases). */
inline Latest pickLatest (const juce::var& releases)
{
    Latest best;
    if (auto* arr = releases.getArray())
        for (auto& r : *arr)
        {
            const auto tag = r.getProperty ("tag_name", {}).toString();
            if (! tag.startsWith ("vst-v") || (bool) r.getProperty ("draft", false) || (bool) r.getProperty ("prerelease", false)) continue;
            const auto v = tag.fromFirstOccurrenceOf ("vst-v", false, false);
            if (best.version.isEmpty() || newer (best.version, v)) { best.version = v; best.url = r.getProperty ("html_url", {}).toString(); }
        }
    return best;
}

class UpdateBanner : public juce::Component, private juce::Thread, private juce::AsyncUpdater
{
public:
    /** repo: repositorio en sampledhead-dot; plugin: nombre visible; current: versión instalada. */
    UpdateBanner (juce::String repoName, juce::String pluginName, juce::String currentVersion, juce::Colour background, juce::Colour textColour, juce::Colour buttonColour)
        : juce::Thread ("SampledHead update check"), repo (std::move (repoName)), plugin (std::move (pluginName)), current (std::move (currentVersion)),
          bg (background), fg (textColour), accent (buttonColour)
    {
        setVisible (false);
        for (auto* b : { &download, &dismiss }) { addAndMakeVisible (b); b->setColour (juce::TextButton::buttonColourId, accent); b->setColour (juce::TextButton::textColourOffId, bg); }
        dismiss.setColour (juce::TextButton::buttonColourId, bg.brighter (0.25f)); dismiss.setColour (juce::TextButton::textColourOffId, fg);
        dismiss.setTooltip (utf8 ("No volver a avisar de esta versión"));
        download.onClick = [this] { if (url.isNotEmpty()) juce::URL (url).launchInDefaultBrowser(); };
        dismiss.onClick = [this] { ignoreThisVersion(); };
        startThread (juce::Thread::Priority::low);
    }
    ~UpdateBanner() override { signalThreadShouldExit(); cancelPendingUpdate(); stopThread (9000); }

    /** «✕»: no volver a avisar de esta versión. */
    void ignoreThisVersion() { if (auto s = settings()) { s->setValue ("ignorada", latest); s->saveIfNeeded(); } setVisible (false); if (auto* p = getParentComponent()) p->resized(); }
    /** Para las pruebas: ¿hay aviso? ¿de qué versión? */
    bool hasUpdate() const { return isVisible(); }
    juce::String latestVersion() const { return latest; }
    void waitForCheck (int ms) { waitForThreadToExit (ms); handleUpdateNowIfNeeded(); }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (bg); g.fillRoundedRectangle (r, 6.0f);
        g.setColour (accent); g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.5f);
        g.setColour (fg); g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawFittedText (utf8 ("Nueva versión de ") + plugin + " (v" + latest + utf8 (") · tienes la v") + current,
                          getLocalBounds().reduced (10, 0).withTrimmedRight (download.getWidth() + dismiss.getWidth() + 14), juce::Justification::centredLeft, 1);
    }
    void resized() override
    {
        auto r = getLocalBounds().reduced (4);
        dismiss.setBounds (r.removeFromRight (28)); r.removeFromRight (4);
        download.setBounds (r.removeFromRight (96));
    }

private:
    std::unique_ptr<juce::PropertiesFile> settings() const
    {
        juce::PropertiesFile::Options o;
        o.applicationName = plugin.removeCharacters (" "); o.filenameSuffix = ".settings";
        o.folderName = "SampledHead"; o.osxLibrarySubFolder = "Application Support";
        const auto dir = juce::SystemStats::getEnvironmentVariable ("SH_UPDATE_SETTINGS", {});
        if (dir.isNotEmpty()) return std::make_unique<juce::PropertiesFile> (juce::File (dir).getChildFile (o.applicationName + o.filenameSuffix), o);
        return std::make_unique<juce::PropertiesFile> (o);
    }

    void run() override
    {
        auto s = settings();
        const auto now = juce::Time::currentTimeMillis();
        const auto fake = juce::SystemStats::getEnvironmentVariable ("SH_UPDATE_FAKE", {});
        juce::String v = s->getValue ("ultima"), u = s->getValue ("url");
        if (fake.isNotEmpty() || now - s->getValue ("comprobado", "0").getLargeIntValue() > 24LL * 3600 * 1000)
        {
            juce::String body;
            if (fake.isNotEmpty()) body = juce::File (fake).loadFileAsString();
            else
            {
                auto opts = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                                .withConnectionTimeoutMs (8000)
                                .withExtraHeaders ("User-Agent: SampledHead-plugin\r\nAccept: application/vnd.github+json")
                                .withProgressCallback ([this] (int, int) { return ! threadShouldExit(); });
                if (auto in = juce::URL ("https://api.github.com/repos/sampledhead-dot/" + repo + "/releases?per_page=30").createInputStream (opts))
                    body = in->readEntireStreamAsString();
            }
            if (threadShouldExit()) return;
            const auto found = pickLatest (juce::JSON::parse (body));
            if (found.version.isNotEmpty())
            {
                v = found.version; u = found.url;
                s->setValue ("ultima", v); s->setValue ("url", u); s->setValue ("comprobado", juce::String (now));
                s->saveIfNeeded();
            }
        }
        const bool show = v.isNotEmpty() && newer (current, v) && s->getValue ("ignorada") != v;
        const juce::ScopedLock sl (lock);
        latestFound = v; urlFound = u; showFound = show;
        if (! threadShouldExit()) triggerAsyncUpdate();
    }
    void handleAsyncUpdate() override
    {
        { const juce::ScopedLock sl (lock); latest = latestFound; url = urlFound; }
        setVisible (showFound);
        if (auto* p = getParentComponent()) p->resized();
        repaint();
    }

    const juce::String repo, plugin, current;
    const juce::Colour bg, fg, accent;
    juce::TextButton download { "Descargar" }, dismiss { juce::String (juce::CharPointer_UTF8 ("\xe2\x9c\x95")) };
    juce::CriticalSection lock;
    juce::String latestFound, urlFound, latest, url;
    bool showFound = false;
};
} // namespace shupdate
