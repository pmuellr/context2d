
/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "SkBenchmark.h"
#include "SkBitmap.h"
#include "SkPaint.h"
#include "SkCanvas.h"
#include "SkColorPriv.h"
#include "SkRandom.h"
#include "SkString.h"

static const char* gConfigName[] = {
    "ERROR", "a1", "a8", "index8", "565", "4444", "8888"
};

static int conv6ToByte(int x) {
    return x * 0xFF / 5;
}

static int convByteTo6(int x) {
    return x * 5 / 255;
}

static uint8_t compute666Index(SkPMColor c) {
    int r = SkGetPackedR32(c);
    int g = SkGetPackedG32(c);
    int b = SkGetPackedB32(c);

    return convByteTo6(r) * 36 + convByteTo6(g) * 6 + convByteTo6(b);
}

static void convertToIndex666(const SkBitmap& src, SkBitmap* dst) {
    SkColorTable* ctable = new SkColorTable(216);
    SkPMColor* colors = ctable->lockColors();
    // rrr ggg bbb
    for (int r = 0; r < 6; r++) {
        int rr = conv6ToByte(r);
        for (int g = 0; g < 6; g++) {
            int gg = conv6ToByte(g);
            for (int b = 0; b < 6; b++) {
                int bb = conv6ToByte(b);
                *colors++ = SkPreMultiplyARGB(0xFF, rr, gg, bb);
            }
        }
    }
    ctable->unlockColors(true);
    dst->setConfig(SkBitmap::kIndex8_Config, src.width(), src.height());
    dst->allocPixels(ctable);
    ctable->unref();

    SkAutoLockPixels alps(src);
    SkAutoLockPixels alpd(*dst);

    for (int y = 0; y < src.height(); y++) {
        const SkPMColor* srcP = src.getAddr32(0, y);
        uint8_t* dstP = dst->getAddr8(0, y);
        for (int x = src.width() - 1; x >= 0; --x) {
            *dstP++ = compute666Index(*srcP++);
        }
    }
}

/*  Variants for bitmaps

    - src depth (32 w+w/o alpha), 565, 4444, index, a8
    - paint options: filtering, dither, alpha
    - matrix options: translate, scale, rotate, persp
    - tiling: none, repeat, mirror, clamp

 */

class BitmapBench : public SkBenchmark {
    SkBitmap    fBitmap;
    SkPaint     fPaint;
    bool        fIsOpaque;
    bool        fForceUpdate; //bitmap marked as dirty before each draw. forces bitmap to be updated on device cache
    bool        fIsVolatile;
    SkBitmap::Config fConfig;
    SkString    fName;
    enum { BICUBIC_DUR_SCALE = 20 };
    enum { N = SkBENCHLOOP(15 * BICUBIC_DUR_SCALE) };
    enum { W = 128 };
    enum { H = 128 };
public:
    BitmapBench(void* param, bool isOpaque, SkBitmap::Config c,
                bool forceUpdate = false, bool bitmapVolatile = false)
        : INHERITED(param)
        , fIsOpaque(isOpaque)
        , fForceUpdate(forceUpdate)
        , fIsVolatile(bitmapVolatile)
        , fConfig(c) {
    }

protected:
    virtual const char* onGetName() {
        fName.set("bitmap");
        fName.appendf("_%s%s", gConfigName[fConfig],
                      fIsOpaque ? "" : "_A");
        if (fForceUpdate)
            fName.append("_update");
        if (fIsVolatile)
            fName.append("_volatile");

        return fName.c_str();
    }

    virtual void onPreDraw() {
        SkBitmap bm;

        if (SkBitmap::kIndex8_Config == fConfig) {
            bm.setConfig(SkBitmap::kARGB_8888_Config, W, H);
        } else {
            bm.setConfig(fConfig, W, H);
        }

        bm.allocPixels();
        bm.eraseColor(fIsOpaque ? SK_ColorBLACK : 0);

        onDrawIntoBitmap(bm);

        if (SkBitmap::kIndex8_Config == fConfig) {
            convertToIndex666(bm, &fBitmap);
        } else {
            fBitmap = bm;
        }

        if (fBitmap.getColorTable()) {
            fBitmap.getColorTable()->setIsOpaque(fIsOpaque);
        }
        fBitmap.setIsOpaque(fIsOpaque);
        fBitmap.setIsVolatile(fIsVolatile);
    }

    virtual void onDraw(SkCanvas* canvas) {
        SkIPoint dim = this->getSize();
        SkRandom rand;

        SkPaint paint(fPaint);
        this->setupPaint(&paint);

        const SkBitmap& bitmap = fBitmap;
        const SkScalar x0 = SkIntToScalar(-bitmap.width() / 2);
        const SkScalar y0 = SkIntToScalar(-bitmap.height() / 2);

        int count = N;
#ifdef SK_RELEASE
        // in DEBUG, N is always 1
        if (paint.getFlags() & SkPaint::kBicubicFilterBitmap_Flag) {
            count /= BICUBIC_DUR_SCALE;
        }
#endif
        for (int i = 0; i < count; i++) {
            SkScalar x = x0 + rand.nextUScalar1() * dim.fX;
            SkScalar y = y0 + rand.nextUScalar1() * dim.fY;

            if (fForceUpdate)
                bitmap.notifyPixelsChanged();

            canvas->drawBitmap(bitmap, x, y, &paint);
        }
    }

    virtual float onGetDurationScale() SK_OVERRIDE {
        SkPaint paint;
        this->setupPaint(&paint);
#ifdef SK_DEBUG
        return 1;
#else
        return (paint.getFlags() & SkPaint::kBicubicFilterBitmap_Flag) ?
                (float)BICUBIC_DUR_SCALE : 1;
#endif
    }

    virtual void onDrawIntoBitmap(const SkBitmap& bm) {
        const int w = bm.width();
        const int h = bm.height();

        SkCanvas canvas(bm);
        SkPaint p;
        p.setAntiAlias(true);
        p.setColor(SK_ColorRED);
        canvas.drawCircle(SkIntToScalar(w)/2, SkIntToScalar(h)/2,
                          SkIntToScalar(SkMin32(w, h))*3/8, p);

        SkRect r;
        r.set(0, 0, SkIntToScalar(w), SkIntToScalar(h));
        p.setStyle(SkPaint::kStroke_Style);
        p.setStrokeWidth(SkIntToScalar(4));
        p.setColor(SK_ColorBLUE);
        canvas.drawRect(r, p);
    }

private:
    typedef SkBenchmark INHERITED;
};

/** Explicitly invoke some filter types to improve coverage of acceleration
    procs. */

enum Flags {
    kScale_Flag             = 1 << 0,
    kRotate_Flag            = 1 << 1,
    kBilerp_Flag            = 1 << 2,
    kBicubic_Flag           = 1 << 3,
};

static bool isBilerp(uint32_t flags) {
    return (flags & (kBilerp_Flag | kBicubic_Flag)) == (kBilerp_Flag);
}

static bool isBicubic(uint32_t flags) {
    return (flags & (kBilerp_Flag | kBicubic_Flag)) == (kBilerp_Flag | kBicubic_Flag);
}

class FilterBitmapBench : public BitmapBench {
    uint32_t    fFlags;
    SkString    fFullName;
    enum { N = SkBENCHLOOP(300) };
public:
    FilterBitmapBench(void* param, bool isOpaque, SkBitmap::Config c,
                      bool forceUpdate, bool isVolitile, uint32_t flags)
        : INHERITED(param, isOpaque, c, forceUpdate, isVolitile)
        , fFlags(flags) {
    }

protected:
    virtual const char* onGetName() {
        fFullName.set(INHERITED::onGetName());
        if (fFlags & kScale_Flag) {
            fFullName.append("_scale");
        }
        if (fFlags & kRotate_Flag) {
            fFullName.append("_rotate");
        }
        if (isBilerp(fFlags)) {
            fFullName.append("_bilerp");
        } else if (isBicubic(fFlags)) {
            fFullName.append("_bicubic");
        }

        return fFullName.c_str();
    }

    virtual void onDraw(SkCanvas* canvas) {
        SkISize dim = canvas->getDeviceSize();
        if (fFlags & kScale_Flag) {
            const SkScalar x = SkIntToScalar(dim.fWidth) / 2;
            const SkScalar y = SkIntToScalar(dim.fHeight) / 2;

            canvas->translate(x, y);
            // just enough so we can't take the sprite case
            canvas->scale(SK_Scalar1 * 99/100, SK_Scalar1 * 99/100);
            canvas->translate(-x, -y);
        }
        if (fFlags & kRotate_Flag) {
            const SkScalar x = SkIntToScalar(dim.fWidth) / 2;
            const SkScalar y = SkIntToScalar(dim.fHeight) / 2;

            canvas->translate(x, y);
            canvas->rotate(SkIntToScalar(35));
            canvas->translate(-x, -y);
        }

        uint32_t orMask = 0;
        uint32_t clearMask = SkPaint::kFilterBitmap_Flag | SkPaint::kBicubicFilterBitmap_Flag;
        if (fFlags & kBilerp_Flag) {
            orMask |= SkPaint::kFilterBitmap_Flag;
        }
        if (fFlags & kBicubic_Flag) {
            orMask |= SkPaint::kBicubicFilterBitmap_Flag;
        }
        this->setPaintMasks(orMask, clearMask);

        INHERITED::onDraw(canvas);
    }

private:
    typedef BitmapBench INHERITED;
};

/** Verify optimizations that test source alpha values. */

class SourceAlphaBitmapBench : public BitmapBench {
public:
    enum SourceAlpha { kOpaque_SourceAlpha, kTransparent_SourceAlpha,
                       kTwoStripes_SourceAlpha, kThreeStripes_SourceAlpha};
private:
    SkString    fFullName;
    SourceAlpha fSourceAlpha;
public:
    SourceAlphaBitmapBench(void* param, SourceAlpha alpha, SkBitmap::Config c,
                bool forceUpdate = false, bool bitmapVolatile = false)
        : INHERITED(param, false, c, forceUpdate, bitmapVolatile)
        , fSourceAlpha(alpha) {
    }

protected:
    virtual const char* onGetName() {
        fFullName.set(INHERITED::onGetName());

        if (fSourceAlpha == kOpaque_SourceAlpha) {
                fFullName.append("_source_opaque");
        } else if (fSourceAlpha == kTransparent_SourceAlpha) {
                fFullName.append("_source_transparent");
        } else if (fSourceAlpha == kTwoStripes_SourceAlpha) {
                fFullName.append("_source_stripes_two");
        } else if (fSourceAlpha == kThreeStripes_SourceAlpha) {
                fFullName.append("_source_stripes_three");
        }

        return fFullName.c_str();
    }

    virtual void onDrawIntoBitmap(const SkBitmap& bm) SK_OVERRIDE {
        const int w = bm.width();
        const int h = bm.height();

        if (kOpaque_SourceAlpha == fSourceAlpha) {
            bm.eraseColor(SK_ColorBLACK);
        } else if (kTransparent_SourceAlpha == fSourceAlpha) {
            bm.eraseColor(0);
        } else if (kTwoStripes_SourceAlpha == fSourceAlpha) {
            bm.eraseColor(0);

            SkCanvas canvas(bm);
            SkPaint p;
            p.setAntiAlias(false);
            p.setStyle(SkPaint::kFill_Style);
            p.setColor(SK_ColorRED);

            // Draw red vertical stripes on transparent background
            SkRect r;
            for (int x = 0; x < w; x+=2)
            {
                r.set(SkIntToScalar(x), 0, SkIntToScalar(x+1), SkIntToScalar(h));
                canvas.drawRect(r, p);
            }

        } else if (kThreeStripes_SourceAlpha == fSourceAlpha) {
            bm.eraseColor(0);

            SkCanvas canvas(bm);
            SkPaint p;
            p.setAntiAlias(false);
            p.setStyle(SkPaint::kFill_Style);

            // Draw vertical stripes on transparent background with a pattern
            // where the first pixel is fully transparent, the next is semi-transparent
            // and the third is fully opaque.
            SkRect r;
            for (int x = 0; x < w; x++)
            {
                if (x % 3 == 0) {
                    continue; // Keep transparent
                } else if (x % 3 == 1) {
                    p.setColor(SkColorSetARGB(127, 127, 127, 127)); // Semi-transparent
                } else if (x % 3 == 2) {
                    p.setColor(SK_ColorRED); // Opaque
                }
                r.set(SkIntToScalar(x), 0, SkIntToScalar(x+1), SkIntToScalar(h));
                canvas.drawRect(r, p);
            }
        }
    }

private:
    typedef BitmapBench INHERITED;
};

DEF_BENCH( return new BitmapBench(p, false, SkBitmap::kARGB_8888_Config); )
DEF_BENCH( return new BitmapBench(p, true, SkBitmap::kARGB_8888_Config); )
DEF_BENCH( return new BitmapBench(p, true, SkBitmap::kRGB_565_Config); )
DEF_BENCH( return new BitmapBench(p, false, SkBitmap::kIndex8_Config); )
DEF_BENCH( return new BitmapBench(p, true, SkBitmap::kIndex8_Config); )
DEF_BENCH( return new BitmapBench(p, true, SkBitmap::kARGB_8888_Config, true, true); )
DEF_BENCH( return new BitmapBench(p, true, SkBitmap::kARGB_8888_Config, true, false); )

// scale filter -> S32_opaque_D32_filter_DX_{SSE2,SSSE3} and Fact9 is also for S32_D16_filter_DX_SSE2
DEF_BENCH( return new FilterBitmapBench(p, false, SkBitmap::kARGB_8888_Config, false, false, kScale_Flag | kBilerp_Flag); )
DEF_BENCH( return new FilterBitmapBench(p, true, SkBitmap::kARGB_8888_Config, false, false, kScale_Flag | kBilerp_Flag); )
DEF_BENCH( return new FilterBitmapBench(p, true, SkBitmap::kARGB_8888_Config, true, true, kScale_Flag | kBilerp_Flag); )
DEF_BENCH( return new FilterBitmapBench(p, true, SkBitmap::kARGB_8888_Config, true, false, kScale_Flag | kBilerp_Flag); )

// scale rotate filter -> S32_opaque_D32_filter_DXDY_{SSE2,SSSE3}
DEF_BENCH( return new FilterBitmapBench(p, false, SkBitmap::kARGB_8888_Config, false, false, kScale_Flag | kRotate_Flag | kBilerp_Flag); )
DEF_BENCH( return new FilterBitmapBench(p, true, SkBitmap::kARGB_8888_Config, false, false, kScale_Flag | kRotate_Flag | kBilerp_Flag); )
DEF_BENCH( return new FilterBitmapBench(p, true, SkBitmap::kARGB_8888_Config, true, true, kScale_Flag | kRotate_Flag | kBilerp_Flag); )
DEF_BENCH( return new FilterBitmapBench(p, true, SkBitmap::kARGB_8888_Config, true, false, kScale_Flag | kRotate_Flag | kBilerp_Flag); )

DEF_BENCH( return new FilterBitmapBench(p, false, SkBitmap::kARGB_8888_Config, false, false, kScale_Flag | kBilerp_Flag | kBicubic_Flag); )
DEF_BENCH( return new FilterBitmapBench(p, false, SkBitmap::kARGB_8888_Config, false, false, kScale_Flag | kRotate_Flag | kBilerp_Flag | kBicubic_Flag); )

// source alpha tests -> S32A_Opaque_BlitRow32_{arm,neon}
DEF_BENCH( return new SourceAlphaBitmapBench(p, SourceAlphaBitmapBench::kOpaque_SourceAlpha, SkBitmap::kARGB_8888_Config); )
DEF_BENCH( return new SourceAlphaBitmapBench(p, SourceAlphaBitmapBench::kTransparent_SourceAlpha, SkBitmap::kARGB_8888_Config); )
DEF_BENCH( return new SourceAlphaBitmapBench(p, SourceAlphaBitmapBench::kTwoStripes_SourceAlpha, SkBitmap::kARGB_8888_Config); )
DEF_BENCH( return new SourceAlphaBitmapBench(p, SourceAlphaBitmapBench::kThreeStripes_SourceAlpha, SkBitmap::kARGB_8888_Config); )
