/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkBitmap.h"
#include "SkColorPriv.h"
#include "SkCommandLineFlags.h"
#include "SkData.h"
#include "SkGraphics.h"
#include "SkImageDecoder.h"
#include "SkImageEncoder.h"
#include "SkOSFile.h"
#include "SkStream.h"
#include "SkTArray.h"
#include "SkTemplates.h"

DEFINE_string2(readPath, r, "", "Folder(s) and files to decode images. Required.");
DEFINE_string2(writePath, w, "",  "Write rendered images into this directory.");
DEFINE_bool(reencode, true, "Reencode the images to test encoding.");

struct Format {
    SkImageEncoder::Type    fType;
    SkImageDecoder::Format  fFormat;
    const char*             fSuffix;
};

static const Format gFormats[] = {
    { SkImageEncoder::kBMP_Type, SkImageDecoder::kBMP_Format, ".bmp" },
    { SkImageEncoder::kGIF_Type, SkImageDecoder::kGIF_Format, ".gif" },
    { SkImageEncoder::kICO_Type, SkImageDecoder::kICO_Format, ".ico" },
    { SkImageEncoder::kJPEG_Type, SkImageDecoder::kJPEG_Format, ".jpg" },
    { SkImageEncoder::kPNG_Type, SkImageDecoder::kPNG_Format, ".png" },
    { SkImageEncoder::kWBMP_Type, SkImageDecoder::kWBMP_Format, ".wbmp" },
    { SkImageEncoder::kWEBP_Type, SkImageDecoder::kWEBP_Format, ".webp" }
};

static SkImageEncoder::Type format_to_type(SkImageDecoder::Format format) {
    for (size_t i = 0; i < SK_ARRAY_COUNT(gFormats); i++) {
        if (gFormats[i].fFormat == format) {
            return gFormats[i].fType;
        }
    }
    return SkImageEncoder::kUnknown_Type;
}

static const char* suffix_for_type(SkImageEncoder::Type type) {
    for (size_t i = 0; i < SK_ARRAY_COUNT(gFormats); i++) {
        if (gFormats[i].fType == type) {
            return gFormats[i].fSuffix;
        }
    }
    return "";
}

static SkImageDecoder::Format guess_format_from_suffix(const char suffix[]) {
    for (size_t i = 0; i < SK_ARRAY_COUNT(gFormats); i++) {
        if (strcmp(suffix, gFormats[i].fSuffix) == 0) {
            return gFormats[i].fFormat;
        }
    }
    return SkImageDecoder::kUnknown_Format;
}

static void make_outname(SkString* dst, const char outDir[], const char src[],
                         const char suffix[]) {
    dst->set(outDir);
    const char* start = strrchr(src, '/');
    if (start) {
        start += 1; // skip the actual last '/'
    } else {
        start = src;
    }
    dst->append(start);
    if (!dst->endsWith(suffix)) {
        const char* cstyleDst = dst->c_str();
        const char* dot = strrchr(cstyleDst, '.');
        if (dot != NULL) {
            int32_t index = SkToS32(dot - cstyleDst);
            dst->remove(index, dst->size() - index);
        }
        dst->append(suffix);
    }
}

// Store the names of the filenames to report later which ones failed, succeeded, and were
// invalid.
static SkTArray<SkString, false> gInvalidStreams;
static SkTArray<SkString, false> gMissingCodecs;
static SkTArray<SkString, false> gDecodeFailures;
static SkTArray<SkString, false> gEncodeFailures;
static SkTArray<SkString, false> gSuccessfulDecodes;

static bool write_bitmap(const char outName[], SkBitmap* bm) {
    SkBitmap bitmap8888;
    if (SkBitmap::kARGB_8888_Config != bm->config()) {
        if (!bm->copyTo(&bitmap8888, SkBitmap::kARGB_8888_Config)) {
            return false;
        }
        bm = &bitmap8888;
    }
    // FIXME: This forces all pixels to be opaque, like the many implementations
    // of force_all_opaque. These should be unified if they cannot be eliminated.
    SkAutoLockPixels lock(*bm);
    for (int y = 0; y < bm->height(); y++) {
        for (int x = 0; x < bm->width(); x++) {
            *bm->getAddr32(x, y) |= (SK_A32_MASK << SK_A32_SHIFT);
        }
    }
    return SkImageEncoder::EncodeFile(outName, *bm, SkImageEncoder::kPNG_Type, 100);
}

static void decodeFileAndWrite(const char srcPath[], const SkString* writePath) {
    SkBitmap bitmap;
    SkFILEStream stream(srcPath);
    if (!stream.isValid()) {
        gInvalidStreams.push_back().set(srcPath);
        return;
    }

    SkImageDecoder* codec = SkImageDecoder::Factory(&stream);
    if (NULL == codec) {
        gMissingCodecs.push_back().set(srcPath);
        return;
    }

    SkAutoTDelete<SkImageDecoder> ad(codec);

    stream.rewind();
    if (!codec->decode(&stream, &bitmap, SkBitmap::kARGB_8888_Config,
                       SkImageDecoder::kDecodePixels_Mode)) {
        gDecodeFailures.push_back().set(srcPath);
        return;
    }

    gSuccessfulDecodes.push_back().printf("%s [%d %d]", srcPath, bitmap.width(), bitmap.height());

    if (FLAGS_reencode) {
        // Encode to the format the file was originally in, or PNG if the encoder for the same
        // format is unavailable.
        SkImageDecoder::Format format = codec->getFormat();
        if (SkImageDecoder::kUnknown_Format == format) {
            if (stream.rewind()) {
                format = SkImageDecoder::GetStreamFormat(&stream);
            }
            if (SkImageDecoder::kUnknown_Format == format) {
                const char* dot = strrchr(srcPath, '.');
                if (NULL != dot) {
                    format = guess_format_from_suffix(dot);
                }
                if (SkImageDecoder::kUnknown_Format == format) {
                    SkDebugf("Could not determine type for '%s'\n", srcPath);
                    format = SkImageDecoder::kPNG_Format;
                }

            }
        } else {
            SkASSERT(!stream.rewind() || SkImageDecoder::GetStreamFormat(&stream) == format);
        }
        SkImageEncoder::Type type = format_to_type(format);
        // format should never be kUnknown_Format, so type should never be kUnknown_Type.
        SkASSERT(type != SkImageEncoder::kUnknown_Type);

        SkImageEncoder* encoder = SkImageEncoder::Create(type);
        if (NULL == encoder) {
            type = SkImageEncoder::kPNG_Type;
            encoder = SkImageEncoder::Create(type);
            SkASSERT(encoder);
        }
        SkAutoTDelete<SkImageEncoder> ade(encoder);
        // Encode to a stream.
        SkDynamicMemoryWStream wStream;
        if (!encoder->encodeStream(&wStream, bitmap, 100)) {
            gEncodeFailures.push_back().printf("Failed to reencode %s to type '%s'", srcPath,
                                               suffix_for_type(type));
            return;
        }

        SkAutoTUnref<SkData> data(wStream.copyToData());
        if (writePath != NULL && type != SkImageEncoder::kPNG_Type) {
            // Write the encoded data to a file. Do not write to PNG, which will be written later,
            // regardless of the input format.
            SkString outPath;
            make_outname(&outPath, writePath->c_str(), srcPath, suffix_for_type(type));
            SkFILEWStream file(outPath.c_str());
            if(file.write(data->data(), data->size())) {
                gSuccessfulDecodes.push_back().appendf("\twrote %s", outPath.c_str());
            } else {
                gEncodeFailures.push_back().printf("Failed to write %s", outPath.c_str());
            }
        }
        // Ensure that the reencoded data can still be decoded.
        SkMemoryStream memStream(data);
        SkBitmap redecodedBitmap;
        SkImageDecoder::Format formatOnSecondDecode;
        if (SkImageDecoder::DecodeStream(&memStream, &redecodedBitmap, SkBitmap::kNo_Config,
                                          SkImageDecoder::kDecodePixels_Mode,
                                          &formatOnSecondDecode)) {
            SkASSERT(format_to_type(formatOnSecondDecode) == type);
        } else {
            gDecodeFailures.push_back().printf("Failed to redecode %s after reencoding to '%s'",
                                               srcPath, suffix_for_type(type));
        }
    }

    if (writePath != NULL) {
        SkString outPath;
        make_outname(&outPath, writePath->c_str(), srcPath, ".png");
        if (write_bitmap(outPath.c_str(), &bitmap)) {
            gSuccessfulDecodes.push_back().appendf("\twrote %s", outPath.c_str());
        } else {
            gEncodeFailures.push_back().set(outPath);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

// If strings is not empty, print title, followed by each string on its own line starting
// with a tab.
// @return bool True if strings had at least one entry.
static bool print_strings(const char* title, const SkTArray<SkString, false>& strings) {
    if (strings.count() > 0) {
        SkDebugf("%s:\n", title);
        for (int i = 0; i < strings.count(); i++) {
            SkDebugf("\t%s\n", strings[i].c_str());
        }
        SkDebugf("\n");
        return true;
    }
    return false;
}

int tool_main(int argc, char** argv);
int tool_main(int argc, char** argv) {
    SkCommandLineFlags::SetUsage("Decode files, and optionally write the results to files.");
    SkCommandLineFlags::Parse(argc, argv);

    if (FLAGS_readPath.count() < 1) {
        SkDebugf("Folder(s) or image(s) to decode are required.\n");
        return -1;
    }


    SkAutoGraphics ag;

    SkString outDir;
    SkString* outDirPtr;

    if (FLAGS_writePath.count() == 1) {
        outDir.set(FLAGS_writePath[0]);
        if (outDir.c_str()[outDir.size() - 1] != '/') {
            outDir.append("/");
        }
        outDirPtr = &outDir;
    } else {
        outDirPtr = NULL;
    }

    for (int i = 0; i < FLAGS_readPath.count(); i++) {
        if (strlen(FLAGS_readPath[i]) < 1) {
            break;
        }
        SkOSFile::Iter iter(FLAGS_readPath[i]);
        SkString filename;
        if (iter.next(&filename)) {
            SkString directory(FLAGS_readPath[i]);
            if (directory[directory.size() - 1] != '/') {
                directory.append("/");
            }
            do {
                SkString fullname(directory);
                fullname.append(filename);
                decodeFileAndWrite(fullname.c_str(), outDirPtr);
            } while (iter.next(&filename));
        } else {
            decodeFileAndWrite(FLAGS_readPath[i], outDirPtr);
        }
    }

    // Add some space, since codecs may print warnings without newline.
    SkDebugf("\n\n");

    bool failed = print_strings("Invalid files", gInvalidStreams);
    failed |= print_strings("Missing codec", gMissingCodecs);
    failed |= print_strings("Failed to decode", gDecodeFailures);
    failed |= print_strings("Failed to encode", gEncodeFailures);
    print_strings("Decoded", gSuccessfulDecodes);

    return failed ? -1 : 0;
}

void forceLinking();

void forceLinking() {
    // This function leaks, but that is okay because it is not intended
    // to be called. It is only here so that the linker will include the
    // decoders.
    SkDEBUGCODE(SkImageDecoder *creator = ) CreateJPEGImageDecoder();
    SkASSERT(creator);
    SkDEBUGCODE(creator = ) CreateWEBPImageDecoder();
    SkASSERT(creator);
#ifdef SK_BUILD_FOR_UNIX
    SkDEBUGCODE(creator = ) CreateGIFImageDecoder();
    SkASSERT(creator);
#endif
}

#if !defined SK_BUILD_FOR_IOS
int main(int argc, char * const argv[]) {
    return tool_main(argc, (char**) argv);
}
#endif
