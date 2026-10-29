// SPDX-License-Identifier: GPL-3.0-or-later
// Animierte Ebene (CAnimLayer : CLayer, 0x15C8 Bytes, vtable 0x0046B8E0) mit A16- oder AVI-Bild.
// Abspielsteuerung, Bildtakt, Bereich, Ereignisse und Ton-Cues (docs/ANIMATION.md).
// Videoebenen: docs/VIDEO.md. Lippensync (+0x484): docs/formats/ger.md.
#pragma once

#include <array>
#include <memory>

#include "engine/aviimage.hpp"
#include "engine/event.hpp"
#include "engine/layer.hpp"
#include "formats/tcc.hpp"

namespace rah {

// Ton-Cue (+0x498, 100 × 0x2C Bytes, docs/AUDIO.md)
struct AnimCue {
    int type = 0;   // +0x00: 0 frei, 1 einfach, 2 erweitert
    int frame = 0;  // +0x04
    AudioBuffer* buffer = nullptr;  // +0x08
    int action = 0; // +0x0C
    int vol = 0, volMs = 0, freq = 0, freqMs = 0;  // +0x10…+0x1C
    float x = 0, y = 0, z = 0;                     // +0x20…+0x28
};

class AnimLayer : public Layer {
public:
    static constexpr int kMaxCues = 100;

    explicit AnimLayer(EngineGlobals& g);

    int period = 100;          // +0x2B0 ms je Bild
    // +0x2B4/+0x2B8: von der Engine nur genullt, Schauplätze speichern dort ihren Zustand
    // (Loc05: Haltung/letzter Übergang von Loui; auch 0x0040A190, 0x0041CA50)
    int userState = 0;         // +0x2B4
    int userState2 = 0;        // +0x2B8
    int frame = 0;             // +0x2C0 aktuelle Bildnummer
    int frameEventFrame = 0;   // +0x2CC
    int stepCount = 0;         // +0x2D0
    int stepTarget = 0;        // +0x2D4
    int rangeFrom = -1;        // +0x2D8
    int rangeTo = -1;          // +0x2DC
    bool reverse = false;      // +0x2E0
    bool endPending = false;   // +0x2E4
    std::uint32_t nextTime = 0;// +0x2E8
    std::array<AnimCue, kMaxCues> cues{};
    // Lippensync: Daten +0x484 (gehören der Sprachausgabe, g_LipSyncData), Rate +0x488,
    // Index +0x48C, Bildoffset +0x490, Audio +0x494
    const std::vector<std::uint16_t>* lipData = nullptr;
    int lipRate = 0, lipIndex = 0, lipOffset = 0;
    AudioBuffer* lipAudio = nullptr;
    A16Anim anim;              // Bild +0x0C (CImageA16)
    std::unique_ptr<AviImage> avi;  // Bild +0x0C (CImageAvi), Typ 3

    // orig: Rent-A-Hero.exe 0x00442500 (Slot 2) + 0x004425B0 (Slot 3)
    bool load(const FoundFile& file, std::uint16_t colorKey, std::string* error) override;
    // orig: Rent-A-Hero.exe 0x00442AA0 CAnimLayer_Update
    bool update(EngineGlobals& g) override;
    const TccPlanes& planes() const override { return anim.planes(); }
    void applyImageColor(int r, int g, int b) override;
    // orig: Rent-A-Hero.exe 0x00446C90 (AVI: Bild-Slot 3 → CImageAvi_Draw in die Ebenenoberfläche)
    bool renderToSurface() override;
    // Slot 1 → CImageAvi_Draw mit +0x38C: Primäroberfläche (g.screen), Rechteck + (0, 60)
    bool drawImageDirect(Surface565& s, const Rect& r) override;

    // orig: Rent-A-Hero.exe 0x00442730 CAnimLayer_NextFrame
    bool nextFrame();
    // orig: Rent-A-Hero.exe 0x00442950 – Nachhol-Schritt (A16: kein Bildwechsel)
    bool catchUp();
    // orig: Rent-A-Hero.exe 0x004429D0 CAnimLayer_OnEnd
    void onEnd();
    // orig: Rent-A-Hero.exe 0x004430F0 CAnimLayer_Start(modus)
    void start(unsigned mode);
    // orig: Rent-A-Hero.exe 0x00443030 CAnimLayer_PlayEndEvent(modus, ev)
    void playEndEvent(unsigned mode, int ev);
    // orig: Rent-A-Hero.exe 0x004434A0 – nur Ende-Ereignis setzen (läuft weiter); ev 0: nichts
    void setEndEvent(int ev);
    // orig: Rent-A-Hero.exe 0x00443300 – Schrittzähler +0x2D0 setzen
    void setStepCount(int n) { stepCount = n; }
    // orig: Rent-A-Hero.exe 0x00443530 – Zähl-Ereignis +0x1D0 (Typ 4) bei Schritt n (+0x2D4);
    // ev 0: nichts
    void setCountEvent(int ev, int n);
    // orig: Rent-A-Hero.exe 0x00442E70 CAnimLayer_PlayRange(modus, ev, von, bis)
    void playRange(unsigned mode, int ev, int from, int to);
    // orig: Rent-A-Hero.exe 0x00442F90 CAnimLayer_PlayRangeNoEvent(modus, von, bis)
    void playRangeNoEvent(unsigned mode, int from, int to);
    // orig: Rent-A-Hero.exe 0x00443310 CAnimLayer_SeekFrame
    bool seekFrame(int n);
    // orig: Rent-A-Hero.exe 0x004433B0 CAnimLayer_ShowFrame
    void showFrame(int n);
    // orig: Rent-A-Hero.exe 0x00443400 CAnimLayer_SetFrameEvent(ev, bild)
    void setFrameEvent(int ev, int atFrame);
    // orig: Rent-A-Hero.exe 0x00443260 (vtable +0x24): x ≠ 0 → Endmodus; x = 0 → anhalten
    virtual void stop(int x);
    // orig: Rent-A-Hero.exe 0x00442BB0 / 0x00442C10 / 0x00442C70 / 0x00442D10
    void addCue(const AnimCue& c);
    void clearCues() { cues.fill({}); }
    // orig: Rent-A-Hero.exe 0x00442D30 CAnimLayer_RunCues
    void runCues(int atFrame);
    // orig: Rent-A-Hero.exe 0x00442EF0 CAnimLayer_SetLipSync(daten, rate, audio, bildoffset)
    void setLipSync(const std::vector<std::uint16_t>* data, int rate, AudioBuffer* audio, int offset);

    // Bild-Slot 10 (+0x28) SetColor(r, g, b, 0): nur LUT (CActor_SetPosition)
    void setImageColorNoDecode(int r, int g, int b);
    const TccColorAdd& imageColor() const { return anim.color(); }

protected:
    // Bild-vtable: Slot 4 Next, Slot 7 Seek, Slot 6 Rewind (A16 bzw. AVI)
    bool imageNext();
    bool imageSeek(int n);
    void imageRewind();
    void afterImageCall();  // Alpha-Rechteck nach jedem Dekodieren (0x0043B8A7)
    EngineGlobals& g_;

private:
    std::uint32_t seenDecodes_ = 0;
};

}  // namespace rah
