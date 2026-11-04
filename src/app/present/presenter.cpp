// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/present/presenter.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <vector>

#include "app/present/embedded.hpp"
#include "app/present/gl.hpp"
#include "app/present/preset.hpp"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#include "stb_image.h"

namespace rah {

namespace {

// ---------------------------------------------------------------------------------------------
// OpenGL
// ---------------------------------------------------------------------------------------------

constexpr GLuint kAttrPos = 0, kAttrTex = 1, kAttrColor = 2;
// VertexCoord [0,1]² → Clip-Raum; Ausgabe ins Fenster mit gespiegeltem y (Bildzeile 0 = oben)
constexpr float kMvp[16] = {2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 1, 0, -1, -1, 0, 1};
constexpr float kMvpFlip[16] = {2, 0, 0, 0, 0, -2, 0, 0, 0, 0, 1, 0, -1, 1, 0, 1};

constexpr const char* kBlitSrc = R"(#version 130
#if defined(VERTEX)
in vec4 VertexCoord;
in vec4 TexCoord;
uniform mat4 MVPMatrix;
out vec2 uv;
void main() { gl_Position = MVPMatrix * VertexCoord; uv = TexCoord.xy; }
#elif defined(FRAGMENT)
uniform sampler2D Texture;
in vec2 uv;
out vec4 FragColor;
void main() { FragColor = vec4(texture(Texture, uv).rgb, 1.0); }
#endif
)";

struct Samplers {
    GLuint s[2][2] = {};  // [linear][border]
};

GLuint compileProgram(gl::Api& g, const ShaderSource& src, std::string* err) {
    auto compile = [&](GLenum type, const std::string& text) -> GLuint {
        const GLuint sh = g.CreateShader(type);
        const char* p = text.c_str();
        g.ShaderSource(sh, 1, &p, nullptr);
        g.CompileShader(sh);
        GLint ok = 0;
        g.GetShaderiv(sh, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[2048] = {};
            g.GetShaderInfoLog(sh, sizeof log, nullptr, log);
            if (err) *err = std::string(type == GL_VERTEX_SHADER ? "Vertex: " : "Fragment: ") + log;
            g.DeleteShader(sh);
            return 0;
        }
        return sh;
    };
    const GLuint vs = compile(GL_VERTEX_SHADER, src.vertex);
    if (!vs) return 0;
    const GLuint fs = compile(GL_FRAGMENT_SHADER, src.fragment);
    if (!fs) {
        g.DeleteShader(vs);
        return 0;
    }
    const GLuint prog = g.CreateProgram();
    g.AttachShader(prog, vs);
    g.AttachShader(prog, fs);
    g.BindAttribLocation(prog, kAttrPos, "VertexCoord");
    g.BindAttribLocation(prog, kAttrTex, "TexCoord");
    g.BindAttribLocation(prog, kAttrColor, "COLOR");
    g.LinkProgram(prog);
    g.DeleteShader(vs);
    g.DeleteShader(fs);
    GLint ok = 0;
    g.GetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048] = {};
        g.GetProgramInfoLog(prog, sizeof log, nullptr, log);
        if (err) *err = std::string("Link: ") + log;
        g.DeleteProgram(prog);
        return 0;
    }
    return prog;
}

struct TexRef {
    GLint tex = -1, texSize = -1, inSize = -1;
};

struct Pass {
    PassDesc desc;
    GLuint prog = 0;
    GLint mvp = -1, frameDir = -1, frameCount = -1, outSize = -1;
    TexRef source, orig;
    std::vector<std::pair<int, TexRef>> prev;  // PassPrevN (N ≥ 1)
    std::vector<std::pair<int, TexRef>> abs;   // PassN (N ≥ 1: Ausgabe von Durchgang N−1)
    std::vector<std::pair<std::size_t, GLint>> luts;
    GLuint fbo = 0, tex = 0;
    int w = 0, h = 0;
};

class Chain {
public:
    Chain(gl::Api& g, const Samplers& s) : g_(g), smp_(s) {}
    ~Chain() {
        for (auto& p : passes_) {
            if (p.prog) g_.DeleteProgram(p.prog);
            if (p.fbo) g_.DeleteFramebuffers(1, &p.fbo);
            if (p.tex) g_.DeleteTextures(1, &p.tex);
        }
        for (auto& l : luts_)
            if (l.tex) g_.DeleteTextures(1, &l.tex);
    }

    bool build(const PresetDesc& desc, std::string* err) {
        for (const auto& l : desc.luts) {
            Lut lut;
            lut.linear = l.linear;
            const auto data = embeddedFile(l.path);
            int w = 0, h = 0, n = 0;
            stbi_uc* px = data ? stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(data->data()),
                                                       static_cast<int>(data->size()), &w, &h, &n, 4)
                               : nullptr;
            if (!px) {
                if (err) *err = "LUT " + l.path + " nicht lesbar";
                return false;
            }
            g_.GenTextures(1, &lut.tex);
            g_.BindTexture(GL_TEXTURE_2D, lut.tex);
            g_.PixelStorei(GL_UNPACK_ALIGNMENT, 1);
            g_.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
            g_.PixelStorei(GL_UNPACK_ALIGNMENT, 4);
            g_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
            stbi_image_free(px);
            lut.name = l.name;
            luts_.push_back(lut);
        }
        for (std::size_t i = 0; i < desc.passes.size(); ++i) {
            Pass p;
            p.desc = desc.passes[i];
            const auto text = embeddedFile(p.desc.shader);
            if (!text) {
                if (err) *err = p.desc.shader + " nicht eingebettet";
                return false;
            }
            const ShaderSource src = prepareShader(*text);
            std::string e;
            p.prog = compileProgram(g_, src, &e);
            if (!p.prog) {
                if (err) *err = p.desc.shader + ": " + e;
                passes_.push_back(std::move(p));
                return false;
            }
            const GLuint pr = p.prog;
            auto loc = [&](const std::string& n) { return g_.GetUniformLocation(pr, n.c_str()); };
            auto ref = [&](const std::string& n) { return TexRef{loc(n + "Texture"), loc(n + "TextureSize"), loc(n + "InputSize")}; };
            p.mvp = loc("MVPMatrix");
            p.frameDir = loc("FrameDirection");
            p.frameCount = loc("FrameCount");
            p.outSize = loc("OutputSize");
            p.source = TexRef{loc("Texture"), loc("TextureSize"), loc("InputSize")};
            p.orig = ref("Orig");
            for (int n = 1; n <= 16; ++n) {
                TexRef r = ref("PassPrev" + std::to_string(n));
                if (r.tex >= 0 || r.texSize >= 0 || r.inSize >= 0) p.prev.emplace_back(n, r);
                TexRef a = ref("Pass" + std::to_string(n));
                if (a.tex >= 0 || a.texSize >= 0 || a.inSize >= 0) p.abs.emplace_back(n, a);
            }
            for (std::size_t k = 0; k < luts_.size(); ++k)
                if (GLint l = loc(luts_[k].name); l >= 0) p.luts.emplace_back(k, l);
            // Parameter: Standard aus dem Shader, Überschreibung aus dem Preset
            g_.UseProgram(pr);
            for (const auto& prm : src.params) {
                const auto it = desc.params.find(prm.name);
                if (GLint l = loc(prm.name); l >= 0) g_.Uniform1f(l, it != desc.params.end() ? it->second : prm.def);
            }
            g_.UseProgram(0);
            g_.GenFramebuffers(1, &p.fbo);
            g_.GenTextures(1, &p.tex);
            passes_.push_back(std::move(p));
        }
        return true;
    }

    // Größen festlegen (bei Fenster- oder Bildänderung)
    bool resize(int srcW, int srcH, int vpW, int vpH, std::string* err) {
        if (srcW == srcW_ && srcH == srcH_ && vpW == vpW_ && vpH == vpH_) return true;
        srcW_ = srcW, srcH_ = srcH, vpW_ = vpW, vpH_ = vpH;
        int w = srcW, h = srcH;
        for (std::size_t i = 0; i < passes_.size(); ++i) {
            Pass& p = passes_[i];
            const bool last = i + 1 == passes_.size();
            auto dim = [&](ScaleType t, float s, int src, int vp) {
                if (t == ScaleType::Unset) t = last ? ScaleType::Viewport : ScaleType::Source;
                switch (t) {
                case ScaleType::Viewport: return static_cast<int>(std::lround(static_cast<float>(vp) * s));
                case ScaleType::Absolute: return static_cast<int>(std::lround(s));
                default: return static_cast<int>(std::lround(static_cast<float>(src) * s));
                }
            };
            w = std::clamp(dim(p.desc.typeX, p.desc.scaleX, w, vpW), 1, 8192);
            h = std::clamp(dim(p.desc.typeY, p.desc.scaleY, h, vpH), 1, 8192);
            p.w = w;
            p.h = h;
            g_.BindTexture(GL_TEXTURE_2D, p.tex);
            if (p.desc.floatFb) g_.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
            else g_.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            g_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
            g_.BindFramebuffer(GL_FRAMEBUFFER, p.fbo);
            g_.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, p.tex, 0);
            if (g_.CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                g_.BindFramebuffer(GL_FRAMEBUFFER, 0);
                if (err) *err = "Framebuffer unvollständig (" + p.desc.shader + ")";
                srcW_ = -1;
                return false;
            }
        }
        g_.BindFramebuffer(GL_FRAMEBUFFER, 0);
        return true;
    }

    // Alle Durchgänge; Ergebnis: Textur und Größe der letzten Ausgabe
    void run(GLuint frameTex, int fw, int fh, unsigned frameCount, GLuint* outTex, int* outW, int* outH) {
        struct Out {
            GLuint tex;
            int w, h;
            bool linear;  // Abtastung durch den Durchgang, der sie als Quelle liest
        };
        std::vector<Out> outs;  // outs[0] = Original, outs[k] = Ausgabe von Durchgang k−1
        outs.push_back({frameTex, fw, fh, passes_.empty() ? false : passes_[0].desc.filterLinear});
        for (std::size_t i = 0; i < passes_.size(); ++i) {
            const Pass& p = passes_[i];
            g_.BindFramebuffer(GL_FRAMEBUFFER, p.fbo);
            g_.Viewport(0, 0, p.w, p.h);
            g_.UseProgram(p.prog);
            if (p.mvp >= 0) g_.UniformMatrix4fv(p.mvp, 1, GL_FALSE, kMvp);
            if (p.frameDir >= 0) g_.Uniform1i(p.frameDir, 1);
            if (p.frameCount >= 0) g_.Uniform1i(p.frameCount, static_cast<GLint>(frameCount));
            if (p.outSize >= 0) g_.Uniform2f(p.outSize, float(p.w), float(p.h));
            int unit = 0;
            auto bind = [&](const TexRef& r, const Out& o, bool linear) {
                if (r.texSize >= 0) g_.Uniform2f(r.texSize, float(o.w), float(o.h));
                if (r.inSize >= 0) g_.Uniform2f(r.inSize, float(o.w), float(o.h));
                if (r.tex < 0) return;
                g_.ActiveTexture(GL_TEXTURE0 + unit);
                g_.BindTexture(GL_TEXTURE_2D, o.tex);
                g_.BindSampler(unit, smp_.s[linear ? 1 : 0][p.desc.clampToBorder ? 1 : 0]);
                g_.Uniform1i(r.tex, unit);
                ++unit;
            };
            const Out& src = outs.back();
            bind(p.source, src, p.desc.filterLinear);
            bind(p.orig, outs[0], outs[0].linear);
            for (const auto& [n, r] : p.prev) {
                const int k = std::max(0, static_cast<int>(outs.size()) - n);  // Ausgabe von Durchgang i−N
                bind(r, outs[k], outs[k].linear);
            }
            for (const auto& [n, r] : p.abs)
                if (n < static_cast<int>(outs.size())) bind(r, outs[n], outs[n].linear);
            for (const auto& [k, l] : p.luts) {
                g_.ActiveTexture(GL_TEXTURE0 + unit);
                g_.BindTexture(GL_TEXTURE_2D, luts_[k].tex);
                g_.BindSampler(unit, smp_.s[luts_[k].linear ? 1 : 0][0]);
                g_.Uniform1i(l, unit);
                ++unit;
            }
            g_.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            for (int u = unit - 1; u >= 0; --u) {
                g_.ActiveTexture(GL_TEXTURE0 + u);
                g_.BindSampler(u, 0);
            }
            const bool nextLinear = i + 1 < passes_.size() ? passes_[i + 1].desc.filterLinear : false;
            outs.push_back({p.tex, p.w, p.h, nextLinear});
        }
        g_.BindFramebuffer(GL_FRAMEBUFFER, 0);
        g_.UseProgram(0);
        *outTex = outs.back().tex;
        *outW = outs.back().w;
        *outH = outs.back().h;
    }

private:
    struct Lut {
        std::string name;
        GLuint tex = 0;
        bool linear = false;
    };
    gl::Api& g_;
    const Samplers& smp_;
    std::vector<Pass> passes_;
    std::vector<Lut> luts_;
    int srcW_ = -1, srcH_ = -1, vpW_ = -1, vpH_ = -1;
};

std::string chainKey(const DisplaySettings& d) {
    const FilterInfo* crt = findFilter(crtFilters(), d.crt);
    const std::string main = (crt && crt->preset) ? "crt:" + d.crt : d.filter;
    return main + (d.deband ? "+deband" : "");
}

class GlPresenter final : public Presenter {
public:
    GlPresenter(SDL_Window* w, SDL_GLContext c, gl::Api g) : win_(w), ctx_(c), g_(g) {}
    ~GlPresenter() override {
        cache_.clear();
        if (frameTex_) g_.DeleteTextures(1, &frameTex_);
        if (blit_) g_.DeleteProgram(blit_);
        if (vbo_) g_.DeleteBuffers(1, &vbo_);
        if (vao_) g_.DeleteVertexArrays(1, &vao_);
        for (auto& a : smp_.s)
            for (GLuint s : a)
                if (s) g_.DeleteSamplers(1, &s);
        SDL_GL_DestroyContext(ctx_);
    }

    bool init(std::string* err) {
        // Viereck als Dreiecksstreifen: VertexCoord, TexCoord, COLOR (je vec4)
        const float v[] = {
            0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1,  //
            1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1,  //
            0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1,  //
            1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1,
        };
        g_.GenVertexArrays(1, &vao_);
        g_.BindVertexArray(vao_);
        g_.GenBuffers(1, &vbo_);
        g_.BindBuffer(GL_ARRAY_BUFFER, vbo_);
        g_.BufferData(GL_ARRAY_BUFFER, sizeof v, v, GL_STATIC_DRAW);
        const GLsizei stride = 12 * sizeof(float);
        g_.VertexAttribPointer(kAttrPos, 4, GL_FLOAT, GL_FALSE, stride, nullptr);
        g_.VertexAttribPointer(kAttrTex, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(4 * sizeof(float)));
        g_.VertexAttribPointer(kAttrColor, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(8 * sizeof(float)));
        g_.EnableVertexAttribArray(kAttrPos);
        g_.EnableVertexAttribArray(kAttrTex);
        g_.EnableVertexAttribArray(kAttrColor);
        g_.BindVertexArray(0);
        for (int lin = 0; lin < 2; ++lin)
            for (int bord = 0; bord < 2; ++bord) {
                GLuint& s = smp_.s[lin][bord];
                g_.GenSamplers(1, &s);
                const GLint f = lin ? GL_LINEAR : GL_NEAREST;
                const GLint wrap = bord ? GL_CLAMP_TO_BORDER : GL_CLAMP_TO_EDGE;
                g_.SamplerParameteri(s, GL_TEXTURE_MIN_FILTER, f);
                g_.SamplerParameteri(s, GL_TEXTURE_MAG_FILTER, f);
                g_.SamplerParameteri(s, GL_TEXTURE_WRAP_S, wrap);
                g_.SamplerParameteri(s, GL_TEXTURE_WRAP_T, wrap);
                const float black[4] = {0, 0, 0, 1};
                g_.SamplerParameterfv(s, GL_TEXTURE_BORDER_COLOR, black);
            }
        blit_ = compileProgram(g_, prepareShader(kBlitSrc), err);
        if (!blit_) return false;
        blitMvp_ = g_.GetUniformLocation(blit_, "MVPMatrix");
        blitTex_ = g_.GetUniformLocation(blit_, "Texture");
        g_.GenTextures(1, &frameTex_);
        SDL_GL_SetSwapInterval(0);  // wie bisher: keine Wartezeit auf die Bildwiederholung
        const char* ver = reinterpret_cast<const char*>(g_.GetString(GL_VERSION));
        const char* rend = reinterpret_cast<const char*>(g_.GetString(GL_RENDERER));
        info_ = std::string("OpenGL ") + (ver ? ver : "?") + " – " + (rend ? rend : "?");
        return true;
    }

    Kind kind() const override { return Kind::OpenGL; }
    std::string info() const override { return info_; }

    void imguiInit() override {
        ImGui_ImplSDL3_InitForOpenGL(win_, ctx_);
        ImGui_ImplOpenGL3_Init("#version 130");
    }
    void imguiShutdown() override {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
    }
    void imguiNewFrame() override {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
    }

    void setFrame(const std::uint16_t* px, int w, int h) override {
        const std::size_t n = static_cast<std::size_t>(w) * h;
        if (w == fw_ && h == fh_ && last_.size() == n && std::memcmp(last_.data(), px, n * 2) == 0) return;
        g_.BindTexture(GL_TEXTURE_2D, frameTex_);
        g_.PixelStorei(GL_UNPACK_ALIGNMENT, 2);
        if (w != fw_ || h != fh_) {
            g_.TexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, px);
            g_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        } else {
            g_.TexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, px);
        }
        g_.PixelStorei(GL_UNPACK_ALIGNMENT, 4);
        last_.assign(px, px + n);
        fw_ = w;
        fh_ = h;
        dirty_ = true;
    }
    void clearFrame() override {
        fw_ = fh_ = 0;
        last_.clear();
        dirty_ = true;
    }

    bool setDisplay(const DisplaySettings& d, std::string* err) override {
        display_ = d;
        dirty_ = true;
        chain_ = nullptr;
        std::string e;
        bool linear = false;
        const auto desc = buildChain(d, &linear, &e);
        directLinear_ = linear;
        outLinear_ = true;
        if (!desc) {
            if (!e.empty()) return fail(e, err);
            outLinear_ = linear;
            return true;
        }
        const FilterInfo* crt = findFilter(crtFilters(), d.crt);
        const FilterInfo* sc = findFilter(scalerFilters(), d.filter);
        if (!(crt && crt->preset) && !(sc && sc->preset)) outLinear_ = linear;  // nur Entbänderung
        const std::string key = chainKey(d);
        auto it = cache_.find(key);
        if (it == cache_.end()) {
            auto c = std::make_unique<Chain>(g_, smp_);
            if (!c->build(*desc, &e)) {
                cache_[key] = nullptr;
                return fail(e, err);
            }
            it = cache_.emplace(key, std::move(c)).first;
        }
        if (!it->second) return fail("Shader konnte nicht übersetzt werden", err);
        chain_ = it->second.get();
        return true;
    }

    void present(bool withImGui, SDL_Surface** grab) override {
        int dw = 0, dh = 0;
        SDL_GetWindowSizeInPixels(win_, &dw, &dh);
        if (!withImGui && !grab && !dirty_ && dw == dw_ && dh == dh_) return;
        dw_ = dw;
        dh_ = dh;
        dirty_ = false;
        out_ = computeOutRect(dw, dh, fw_ ? fw_ : 640, fh_ ? fh_ : 480, display_.integerScale);
        g_.Disable(GL_SCISSOR_TEST);
        g_.Disable(GL_BLEND);
        g_.Disable(GL_DEPTH_TEST);
        g_.Disable(GL_CULL_FACE);
        g_.BindFramebuffer(GL_FRAMEBUFFER, 0);
        g_.Viewport(0, 0, dw, dh);
        g_.ClearColor(0, 0, 0, 1);
        g_.Clear(GL_COLOR_BUFFER_BIT);
        if (fw_ > 0 && out_.w > 0) {
            g_.BindVertexArray(vao_);
            GLuint tex = frameTex_;
            int tw = fw_, th = fh_;
            bool linear = directLinear_;
            if (chain_) {
                std::string e;
                if (chain_->resize(fw_, fh_, out_.w, out_.h, &e)) {
                    chain_->run(frameTex_, fw_, fh_, frameCount_, &tex, &tw, &th);
                    linear = outLinear_ && (tw != out_.w || th != out_.h);
                } else {
                    std::fprintf(stderr, "Bildfilter: %s\n", e.c_str());
                    chain_ = nullptr;
                }
            }
            ++frameCount_;
            g_.BindFramebuffer(GL_FRAMEBUFFER, 0);
            g_.Viewport(out_.x, dh - out_.y - out_.h, out_.w, out_.h);
            g_.UseProgram(blit_);
            g_.UniformMatrix4fv(blitMvp_, 1, GL_FALSE, kMvpFlip);
            g_.ActiveTexture(GL_TEXTURE0);
            g_.BindTexture(GL_TEXTURE_2D, tex);
            g_.BindSampler(0, smp_.s[linear ? 1 : 0][0]);
            g_.Uniform1i(blitTex_, 0);
            g_.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            g_.BindSampler(0, 0);
            g_.UseProgram(0);
            g_.BindVertexArray(0);
            g_.Viewport(0, 0, dw, dh);
        }
        if (withImGui) ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (grab) {
            *grab = SDL_CreateSurface(dw, dh, SDL_PIXELFORMAT_RGBA32);
            if (*grab) {
                std::vector<std::uint8_t> buf(static_cast<std::size_t>(dw) * dh * 4);
                g_.PixelStorei(GL_PACK_ALIGNMENT, 1);
                g_.ReadPixels(0, 0, dw, dh, GL_RGBA, GL_UNSIGNED_BYTE, buf.data());
                for (int y = 0; y < dh; ++y)
                    std::memcpy(static_cast<std::uint8_t*>((*grab)->pixels) + static_cast<std::size_t>(y) * (*grab)->pitch,
                                buf.data() + static_cast<std::size_t>(dh - 1 - y) * dw * 4, static_cast<std::size_t>(dw) * 4);
            }
        }
        SDL_GL_SwapWindow(win_);
    }

    void windowToFrame(float wx, float wy, float* fx, float* fy) const override {
        const float d = SDL_GetWindowPixelDensity(win_);
        const float px = wx * d, py = wy * d;
        const int fw = fw_ ? fw_ : 640, fh = fh_ ? fh_ : 480;
        *fx = out_.w > 0 ? (px - static_cast<float>(out_.x)) * static_cast<float>(fw) / static_cast<float>(out_.w) : px;
        *fy = out_.h > 0 ? (py - static_cast<float>(out_.y)) * static_cast<float>(fh) / static_cast<float>(out_.h) : py;
    }

private:
    bool fail(const std::string& e, std::string* err) {
        std::fprintf(stderr, "Bildfilter %s: %s\n", displayLabel(display_).c_str(), e.c_str());
        if (err) *err = e;
        const FilterInfo* sc = findFilter(scalerFilters(), display_.filter);
        directLinear_ = outLinear_ = sc && sc->linear;
        chain_ = nullptr;
        return false;
    }

    SDL_Window* win_;
    SDL_GLContext ctx_;
    gl::Api g_;
    Samplers smp_;
    GLuint vao_ = 0, vbo_ = 0, blit_ = 0, frameTex_ = 0;
    GLint blitMvp_ = -1, blitTex_ = -1;
    std::map<std::string, std::unique_ptr<Chain>> cache_;
    Chain* chain_ = nullptr;
    bool directLinear_ = false, outLinear_ = false, dirty_ = true;
    int fw_ = 0, fh_ = 0, dw_ = 0, dh_ = 0;
    std::vector<std::uint16_t> last_;
    unsigned frameCount_ = 0;
    OutRect out_;
    std::string info_;
};

// ---------------------------------------------------------------------------------------------
// SDL_Renderer (ohne Shader): Original/Weich, ganzzahlig oder einpassen
// ---------------------------------------------------------------------------------------------

class SdlPresenter final : public Presenter {
public:
    SdlPresenter(SDL_Window* w, SDL_Renderer* r) : win_(w), ren_(r) {}
    ~SdlPresenter() override {
        if (tex_) SDL_DestroyTexture(tex_);
        SDL_DestroyRenderer(ren_);
    }
    Kind kind() const override { return Kind::SdlRenderer; }
    std::string info() const override { return std::string("SDL_Renderer ") + SDL_GetRendererName(ren_) + " (ohne Bildfilter)"; }
    void imguiInit() override {
        ImGui_ImplSDL3_InitForSDLRenderer(win_, ren_);
        ImGui_ImplSDLRenderer3_Init(ren_);
    }
    void imguiShutdown() override {
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
    }
    void imguiNewFrame() override {
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
    }
    void setFrame(const std::uint16_t* px, int w, int h) override {
        const std::size_t n = static_cast<std::size_t>(w) * h;
        if (w == fw_ && h == fh_ && last_.size() == n && std::memcmp(last_.data(), px, n * 2) == 0) return;
        if (!tex_ || w != fw_ || h != fh_) {
            if (tex_) SDL_DestroyTexture(tex_);
            tex_ = SDL_CreateTexture(ren_, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, w, h);
            applyScaleMode();
        }
        SDL_UpdateTexture(tex_, nullptr, px, w * 2);
        last_.assign(px, px + n);
        fw_ = w;
        fh_ = h;
        dirty_ = true;
    }
    void clearFrame() override {
        fw_ = fh_ = 0;
        last_.clear();
        dirty_ = true;
    }
    bool setDisplay(const DisplaySettings& d, std::string* err) override {
        display_ = d;
        dirty_ = true;
        applyScaleMode();
        const FilterInfo* sc = findFilter(scalerFilters(), d.filter);
        const FilterInfo* crt = findFilter(crtFilters(), d.crt);
        if ((sc && sc->preset) || (crt && crt->preset) || d.deband) {
            if (err) *err = "Bildfilter brauchen OpenGL 3.3";
            return false;
        }
        return true;
    }
    void present(bool withImGui, SDL_Surface** grab) override {
        int dw = 0, dh = 0;
        SDL_GetRenderOutputSize(ren_, &dw, &dh);
        if (!withImGui && !grab && !dirty_ && dw == dw_ && dh == dh_) return;
        dw_ = dw;
        dh_ = dh;
        dirty_ = false;
        out_ = computeOutRect(dw, dh, fw_ ? fw_ : 640, fh_ ? fh_ : 480, display_.integerScale);
        SDL_SetRenderDrawColor(ren_, 0, 0, 0, 255);
        SDL_RenderClear(ren_);
        if (tex_ && fw_ > 0) {
            const SDL_FRect dst{float(out_.x), float(out_.y), float(out_.w), float(out_.h)};
            SDL_RenderTexture(ren_, tex_, nullptr, &dst);
        }
        if (withImGui) {
            // Maßstab selbst setzen: sonst skaliert das Backend die Beschneidung vor und SDL3 noch
            // einmal (falsch bei gebrochener Pixeldichte, z. B. 1,1)
            const ImVec2 fb = ImGui::GetDrawData()->FramebufferScale;
            SDL_SetRenderScale(ren_, fb.x, fb.y);
            ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), ren_);
            SDL_SetRenderScale(ren_, 1.0f, 1.0f);
        }
        if (grab) {
            SDL_Surface* s = SDL_RenderReadPixels(ren_, nullptr);
            *grab = s ? SDL_ConvertSurface(s, SDL_PIXELFORMAT_RGBA32) : nullptr;
            if (s) SDL_DestroySurface(s);
        }
        SDL_RenderPresent(ren_);
    }
    void windowToFrame(float wx, float wy, float* fx, float* fy) const override {
        float px = wx, py = wy;
        int ww = 0, wh = 0, dw = 0, dh = 0;
        SDL_GetWindowSize(win_, &ww, &wh);
        SDL_GetRenderOutputSize(ren_, &dw, &dh);
        if (ww > 0 && wh > 0) {
            px = wx * static_cast<float>(dw) / static_cast<float>(ww);
            py = wy * static_cast<float>(dh) / static_cast<float>(wh);
        }
        const int fw = fw_ ? fw_ : 640, fh = fh_ ? fh_ : 480;
        *fx = out_.w > 0 ? (px - static_cast<float>(out_.x)) * static_cast<float>(fw) / static_cast<float>(out_.w) : px;
        *fy = out_.h > 0 ? (py - static_cast<float>(out_.y)) * static_cast<float>(fh) / static_cast<float>(out_.h) : py;
    }

private:
    void applyScaleMode() {
        const FilterInfo* sc = findFilter(scalerFilters(), display_.filter);
        if (tex_) SDL_SetTextureScaleMode(tex_, (sc && sc->linear) ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);
    }
    SDL_Window* win_;
    SDL_Renderer* ren_;
    SDL_Texture* tex_ = nullptr;
    int fw_ = 0, fh_ = 0, dw_ = 0, dh_ = 0;
    bool dirty_ = true;
    std::vector<std::uint16_t> last_;
    OutRect out_;
};

}  // namespace

std::unique_ptr<Presenter> Presenter::create(SDL_Window* win, bool tryGl, std::string* log) {
    if (tryGl && (SDL_GetWindowFlags(win) & SDL_WINDOW_OPENGL)) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        if (SDL_GLContext ctx = SDL_GL_CreateContext(win)) {
            SDL_GL_MakeCurrent(win, ctx);
            gl::Api g;
            const char* missing = nullptr;
            if (g.load(&missing)) {
                auto p = std::make_unique<GlPresenter>(win, ctx, g);
                std::string err;
                if (p->init(&err)) {
                    p->setDisplay(DisplaySettings{}, nullptr);
                    return p;
                }
                if (log) *log = "OpenGL: " + err;
            } else {
                if (log) *log = std::string("OpenGL: ") + missing + " fehlt";
                SDL_GL_DestroyContext(ctx);
            }
        } else if (log) {
            *log = std::string("OpenGL 3.3: ") + SDL_GetError();
        }
    }
    SDL_Renderer* ren = SDL_CreateRenderer(win, nullptr);
    if (!ren) {
        if (log) *log += std::string(log->empty() ? "" : "; ") + "SDL_Renderer: " + SDL_GetError();
        return nullptr;
    }
    auto p = std::make_unique<SdlPresenter>(win, ren);
    p->setDisplay(DisplaySettings{}, nullptr);
    return p;
}

}  // namespace rah
