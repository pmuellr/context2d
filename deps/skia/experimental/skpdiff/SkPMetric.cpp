#include <cmath>

#include "SkBitmap.h"
#include "skpdiff_util.h"
#include "SkPMetric.h"

struct RGB {
    float r, g, b;
};

struct LAB {
    float l, a, b;
};

template<class T>
struct Image2D {
    int width;
    int height;
    T* image;

    Image2D(int w, int h)
        : width(w),
          height(h) {
        SkASSERT(w > 0);
        SkASSERT(h > 0);
        image = SkNEW_ARRAY(T, w * h);
    }

    ~Image2D() {
        SkDELETE_ARRAY(image);
    }

    void readPixel(int x, int y, T* pixel) const {
        SkASSERT(x >= 0);
        SkASSERT(y >= 0);
        SkASSERT(x < width);
        SkASSERT(y < height);
        *pixel = image[y * width + x];
    }

    void writePixel(int x, int y, const T& pixel) {
        SkASSERT(x >= 0);
        SkASSERT(y >= 0);
        SkASSERT(x < width);
        SkASSERT(y < height);
        image[y * width + x] = pixel;
    }
};

typedef Image2D<float> ImageL;
typedef Image2D<RGB> ImageRGB;
typedef Image2D<LAB> ImageLAB;

template<class T>
struct ImageArray
{
    int slices;
    Image2D<T>** image;

    ImageArray(int w, int h, int s)
        : slices(s) {
        SkASSERT(s > 0);
        image = SkNEW_ARRAY(Image2D<T>*, s);
        for (int sliceIndex = 0; sliceIndex < slices; sliceIndex++) {
            image[sliceIndex] = SkNEW_ARGS(Image2D<T>, (w, h));
        }
    }

    ~ImageArray() {
        for (int sliceIndex = 0; sliceIndex < slices; sliceIndex++) {
            SkDELETE(image[sliceIndex]);
        }
        SkDELETE_ARRAY(image);
    }

    Image2D<T>* getLayer(int z) const {
        SkASSERT(z >= 0);
        SkASSERT(z < slices);
        return image[z];
    }
};

typedef ImageArray<float> ImageL3D;


#define MAT_ROW_MULT(rc,gc,bc) r*rc + g*gc + b*bc


void adobergb_to_cielab(float r, float g, float b, LAB* lab) {
    // Conversion of Adobe RGB to XYZ taken from from "Adobe RGB (1998) ColorImage Encoding"
    // URL:http://www.adobe.com/digitalimag/pdfs/AdobeRGB1998.pdf
    // Section: 4.3.5.3
    // See Also: http://en.wikipedia.org/wiki/Adobe_rgb
    float x = MAT_ROW_MULT(0.57667f, 0.18556f, 0.18823f);
    float y = MAT_ROW_MULT(0.29734f, 0.62736f, 0.07529f);
    float z = MAT_ROW_MULT(0.02703f, 0.07069f, 0.99134f);

    // The following is the white point in XYZ, so it's simply the row wise addition of the above
    // matrix.
    const float xw = 0.5767f + 0.185556f + 0.188212f;
    const float yw = 0.297361f + 0.627355f + 0.0752847f;
    const float zw = 0.0270328f + 0.0706879f + 0.991248f;

    // This is the XYZ color point relative to the white point
    float f[3] = { x / xw, y / yw, z / zw };

    // Conversion from XYZ to LAB taken from
    // http://en.wikipedia.org/wiki/CIELAB#Forward_transformation
    for (int i = 0; i < 3; i++) {
        if (f[i] >= 0.008856f) {
            f[i] = powf(f[i], 1.0f / 3.0f);
        } else {
            f[i] = 7.787f * f[i] + 4.0f / 29.0f;
        }
    }
    lab->l = 116.0f * f[1] - 16.0f;
    lab->a = 500.0f * (f[0] - f[1]);
    lab->b = 200.0f * (f[1] - f[2]);
}

/// Converts a 8888 bitmap to LAB color space and puts it into the output
static void bitmap_to_cielab(const SkBitmap* bitmap, ImageLAB* outImageLAB) {
    SkASSERT(bitmap->config() == SkBitmap::kARGB_8888_Config);

    int width = bitmap->width();
    int height = bitmap->height();
    SkASSERT(outImageLAB->width == width);
    SkASSERT(outImageLAB->height == height);

    bitmap->lockPixels();
    RGB rgb;
    LAB lab;
    for (int y = 0; y < height; y++) {
        unsigned char* row = (unsigned char*)bitmap->getAddr(0, y);
        for (int x = 0; x < width; x++) {
            // Perform gamma correction which is assumed to be 2.2
            rgb.r = powf(row[x * 4 + 2] / 255.0f, 2.2f);
            rgb.g = powf(row[x * 4 + 1] / 255.0f, 2.2f);
            rgb.b = powf(row[x * 4 + 0] / 255.0f, 2.2f);
            adobergb_to_cielab(rgb.r, rgb.g, rgb.b, &lab);
            outImageLAB->writePixel(x, y, lab);
        }
    }
    bitmap->unlockPixels();
}

// From Barten SPIE 1989
static float contrast_sensitivity(float cyclesPerDegree, float luminance) {
    float a = 440.0f * powf(1.0f + 0.7f / luminance, -0.2f);
    float b = 0.3f * powf(1 + 100.0 / luminance, 0.15f);
    return a *
           cyclesPerDegree *
           expf(-b * cyclesPerDegree) *
           sqrtf(1.0f + 0.06f * expf(b * cyclesPerDegree));
}

// From Daly 1993
static float visual_mask(float contrast) {
    float x = powf(392.498f * contrast, 0.7f);
    x = powf(0.0153f * x, 4.0f);
    return powf(1.0f + x, 0.25f);
}

// From Ward Larson Siggraph 1997
static float threshold_vs_intensity(float adaptationLuminance) {
    float logLum = log10f(adaptationLuminance);
    float x;
    if (logLum < -3.94f) {
        x = -2.86f;
    } else if (logLum < -1.44f) {
        x = powf(0.405f * logLum + 1.6f, 2.18) - 2.86f;
    } else if (logLum < -0.0184f) {
        x = logLum - 0.395f;
    } else if (logLum < 1.9f) {
        x = powf(0.249f * logLum + 0.65f, 2.7f) - 0.72f;
    } else {
        x = logLum - 1.255f;
    }
    return powf(10.0f, x);
}

/// Simply takes the L channel from the input and puts it into the output
static void lab_to_l(const ImageLAB* imageLAB, ImageL* outImageL) {
    for (int y = 0; y < imageLAB->height; y++) {
        for (int x = 0; x < imageLAB->width; x++) {
            LAB lab;
            imageLAB->readPixel(x, y, &lab);
            outImageL->writePixel(x, y, lab.l);
        }
    }
}

/// Convolves an image with the given filter in one direction and saves it to the output image
static void convolve(const ImageL* imageL,
                     bool vertical, const float* matrix, int radius,
                     ImageL* outImageL) {
    SkASSERT(imageL->width == outImageL->width);
    SkASSERT(imageL->height == outImageL->height);
    for (int y = 0; y < imageL->height; y++) {
        for (int x = 0; x < imageL->width; x++) {
            float lSum = 0.0f;
            float l;
            for (int xx = -radius; xx <= radius; xx++) {
                int nx = x;
                int ny = y;

                // We mirror at edges so that edge pixels that the filter weighting still makes
                // sense.
                if (vertical) {
                    ny += xx;
                    if (ny < 0) {
                        ny = -ny;
                    }
                    if (ny >= imageL->height) {
                        ny = imageL->height + (imageL->height - ny - 1);
                    }
                } else {
                    nx += xx;
                    if (nx < 0) {
                        nx = -nx;
                    }
                    if (nx >= imageL->width) {
                        nx = imageL->width + (imageL->width - nx - 1);
                    }
                }

                imageL->readPixel(nx, ny, &l);
                float weight = matrix[xx + radius];
                lSum += l * weight;
            }
            outImageL->writePixel(x, y, lSum);
        }
    }
}

float pmetric(const ImageLAB* baselineLAB, const ImageLAB* testLAB, SkTDArray<SkIPoint>* poi) {
    int width = baselineLAB->width;
    int height = baselineLAB->height;
    int maxLevels = (int)log2(width < height ? width : height);

    const float fov = M_PI / 180.0f * 45.0f;
    float contrastSensitivityMax = contrast_sensitivity(3.248f, 100.0f);
    float pixelsPerDegree = width / (2.0f * tanf(fov * 0.5f) * 180.0f / M_PI);

    ImageL3D baselineL(width, height, maxLevels);
    ImageL3D testL(width, height, maxLevels);
    ImageL scratchImageL(width, height);
    float* cyclesPerDegree = SkNEW_ARRAY(float, maxLevels);
    float* thresholdFactorFrequency = SkNEW_ARRAY(float, maxLevels - 2);
    float* contrast = SkNEW_ARRAY(float, maxLevels - 2);

    lab_to_l(baselineLAB, baselineL.getLayer(0));
    lab_to_l(testLAB, testL.getLayer(0));

    // Compute cpd - Cycles per degree on the pyramid
    cyclesPerDegree[0] = 0.5f * pixelsPerDegree;
    for (int levelIndex = 1; levelIndex < maxLevels; levelIndex++) {
        cyclesPerDegree[levelIndex] = cyclesPerDegree[levelIndex - 1] * 0.5f;
    }

    const float filterMatrix[] = { 0.05f, 0.25f, 0.4f, 0.25f, 0.05f };
    // Compute G - The convolved lum for the baseline
    for (int levelIndex = 1; levelIndex < maxLevels; levelIndex++) {
        convolve(baselineL.getLayer(levelIndex - 1), false, filterMatrix, 2, &scratchImageL);
        convolve(&scratchImageL, true, filterMatrix, 2, baselineL.getLayer(levelIndex));
    }
    for (int levelIndex = 1; levelIndex < maxLevels; levelIndex++) {
        convolve(testL.getLayer(levelIndex - 1), false, filterMatrix, 2, &scratchImageL);
        convolve(&scratchImageL, true, filterMatrix, 2, testL.getLayer(levelIndex));
    }

    // Compute F_freq - The elevation f
    for (int levelIndex = 0; levelIndex < maxLevels - 2; levelIndex++) {
        float cpd = cyclesPerDegree[levelIndex];
        thresholdFactorFrequency[levelIndex] = contrastSensitivityMax /
                                               contrast_sensitivity(cpd, 100.0f);
    }

    int failures = 0;
    // Calculate F
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float lBaseline;
            float lTest;
            baselineL.getLayer(0)->readPixel(x, y, &lBaseline);
            testL.getLayer(0)->readPixel(x, y, &lTest);

            float avgLBaseline;
            float avgLTest;
            baselineL.getLayer(maxLevels - 1)->readPixel(x, y, &avgLBaseline);
            testL.getLayer(maxLevels - 1)->readPixel(x, y, &avgLTest);

            float lAdapt = 0.5f * (avgLBaseline + avgLTest);
            if (lAdapt < 1e-5) {
                lAdapt = 1e-5;
            }

            float contrastSum = 0.0f;
            for (int levelIndex = 0; levelIndex < maxLevels - 2; levelIndex++) {
                float baselineL0, baselineL1, baselineL2;
                float testL0, testL1, testL2;
                baselineL.getLayer(levelIndex + 0)->readPixel(x, y, &baselineL0);
                testL.    getLayer(levelIndex + 0)->readPixel(x, y, &testL0);
                baselineL.getLayer(levelIndex + 1)->readPixel(x, y, &baselineL1);
                testL.    getLayer(levelIndex + 1)->readPixel(x, y, &testL1);
                baselineL.getLayer(levelIndex + 2)->readPixel(x, y, &baselineL2);
                testL.    getLayer(levelIndex + 2)->readPixel(x, y, &testL2);

                float baselineContrast1 = fabsf(baselineL0 - baselineL1);
                float testContrast1     = fabsf(testL0 - testL1);
                float numerator = (baselineContrast1 > testContrast1) ?
                                   baselineContrast1 : testContrast1;

                float baselineContrast2 = fabsf(baselineL2);
                float testContrast2     = fabsf(testL2);
                float denominator = (baselineContrast2 > testContrast2) ?
                                    baselineContrast2 : testContrast2;

                // Avoid divides by close to zero
                if (denominator < 1e-5) {
                    denominator = 1e-5;
                }

                contrast[levelIndex] = numerator / denominator;
                contrastSum += contrast[levelIndex];
            }

            if (contrastSum < 1e-5) {
                contrastSum = 1e-5;
            }

            float F = 0.0f;
            for (int levelIndex = 0; levelIndex < maxLevels - 2; levelIndex++) {
                float mask = visual_mask(contrast[levelIndex] *
                             contrast_sensitivity(cyclesPerDegree[levelIndex], lAdapt));

                F += contrast[levelIndex] +
                     thresholdFactorFrequency[levelIndex] * mask / contrastSum;
            }

            if (F < 1.0f) {
                F = 1.0f;
            }

            if (F > 10.0f) {
                F = 10.0f;
            }


            bool isFailure = false;
            if (fabsf(lBaseline - lTest) > F * threshold_vs_intensity(lAdapt)) {
                isFailure = true;
            } else {
                LAB baselineColor;
                LAB testColor;
                baselineLAB->readPixel(x, y, &baselineColor);
                testLAB->readPixel(x, y, &testColor);
                float contrastA = baselineColor.a - testColor.a;
                float contrastB = baselineColor.b - testColor.b;
                float colorScale = 1.0f;
                if (lAdapt < 10.0f) {
                    colorScale = lAdapt / 10.0f;
                }
                colorScale *= colorScale;

                if ((contrastA * contrastA + contrastB * contrastB) * colorScale > F)
                {
                    isFailure = true;
                }
            }

            if (isFailure) {
                failures++;
                poi->push()->set(x, y);
            }
        }
    }

    SkDELETE_ARRAY(cyclesPerDegree);
    SkDELETE_ARRAY(contrast);
    SkDELETE_ARRAY(thresholdFactorFrequency);
    return (double)failures;
}

const char* SkPMetric::getName() {
    return "perceptual";
}

int SkPMetric::queueDiff(SkBitmap* baseline, SkBitmap* test) {
    int diffID = fQueuedDiffs.count();
    double startTime = get_seconds();
    QueuedDiff& diff = fQueuedDiffs.push_back();
    diff.result = 0.0;

    // Ensure the images are comparable
    if (baseline->width() != test->width() || baseline->height() != test->height() ||
                    baseline->width() <= 0 || baseline->height() <= 0) {
        diff.finished = true;
        return diffID;
    }

    ImageLAB baselineLAB(baseline->width(), baseline->height());
    ImageLAB testLAB(baseline->width(), baseline->height());

    bitmap_to_cielab(baseline, &baselineLAB);
    bitmap_to_cielab(test, &testLAB);

    diff.result = pmetric(&baselineLAB, &testLAB, &diff.poi);

    SkDebugf("Time: %f\n", (get_seconds() - startTime));

    return diffID;
}


void SkPMetric::deleteDiff(int id) {
   fQueuedDiffs[id].poi.reset();
}

bool SkPMetric::isFinished(int id) {
    return fQueuedDiffs[id].finished;
}

double SkPMetric::getResult(int id) {
    return fQueuedDiffs[id].result;
}

int SkPMetric::getPointsOfInterestCount(int id) {
    return fQueuedDiffs[id].poi.count();
}

SkIPoint* SkPMetric::getPointsOfInterest(int id) {
    return fQueuedDiffs[id].poi.begin();
}
