/*
 * Copyright 2013 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#define __NO_STD_VECTOR // Uses cl::vectpr instead of std::vectpr
#define __NO_STD_STRING // Uses cl::STRING_CLASS instead of std::string
#include <CL/cl.hpp>

#include "SkCommandLineFlags.h"
#include "SkGraphics.h"
#include "SkStream.h"
#include "SkTDArray.h"

#include "SkCLImageDiffer.h"
#include "SkDiffContext.h"
#include "SkImageDiffer.h"
#include "SkPMetric.h"
#include "skpdiff_util.h"

#include "SkForceLinking.h"
__SK_FORCE_IMAGE_DECODER_LINKING;

// Command line argument definitions go here
DEFINE_bool2(list, l, false, "List out available differs");
DEFINE_string2(differs, d, "", "The names of the differs to use or all of them by default");
DEFINE_string2(folders, f, "", "Compare two folders with identical subfile names: <baseline folder> <test folder>");
DEFINE_string2(patterns, p, "", "Use two patterns to compare images: <baseline> <test>");
DEFINE_string2(output, o, "skpdiff_output.json", "Writes the output of these diffs to output: <output>");
DEFINE_bool(jsonp, true, "Output JSON with padding");

/// A callback for any OpenCL errors
CL_CALLBACK void error_notify(const char* errorInfo, const void* privateInfoSize, ::size_t cb, void* userData) {
    SkDebugf("OpenCL error notify: %s\n", errorInfo);
    exit(1);
}

/// Creates a device and context with OpenCL
static bool init_device_and_context(cl::Device* device, cl::Context* context) {
    // Query for a platform
    cl::vector<cl::Platform> platformList;
    cl::Platform::get(&platformList);
    SkDebugf("The number of platforms is %u\n", platformList.size());

    // Print some information about the platform for debugging
    cl::Platform& platform = platformList[0];
    cl::STRING_CLASS platformName;
    platform.getInfo(CL_PLATFORM_NAME, &platformName);
    SkDebugf("Platform index 0 is named %s\n", platformName.c_str());

    // Query for a device
    cl::vector<cl::Device> deviceList;
    platform.getDevices(CL_DEVICE_TYPE_GPU, &deviceList);
    SkDebugf("The number of GPU devices is %u\n", deviceList.size());

    // Print some information about the device for debugging
    *device = deviceList[0];
    cl::STRING_CLASS deviceName;
    device->getInfo(CL_DEVICE_NAME, &deviceName);
    SkDebugf("Device index 0 is named %s\n", deviceName.c_str());

    // Create a CL context and check for all errors
    cl_int contextErr = CL_SUCCESS;
    *context = cl::Context(deviceList, NULL, error_notify, NULL, &contextErr);
    if (contextErr != CL_SUCCESS) {
        SkDebugf("Context creation failed: %s\n", cl_error_to_string(contextErr));
        return false;
    }

    return true;
}



static bool init_cl_diff(SkImageDiffer* differ) {
    // Setup OpenCL
    cl::Device device;
    cl::Context context;
    if (!init_device_and_context(&device, &context)) {
        return false;
    }

    // Setup our differ of choice
    SkCLImageDiffer* clDiffer = (SkCLImageDiffer*)differ;
    return clDiffer->init(device(), context());
}

static bool init_dummy(SkImageDiffer* differ) {
    return true;
}


// TODO Find a better home for the diff registry. One possibility is to have the differs self
// register.

// List here every differ
SkDifferentPixelsImageDiffer gDiffPixel;
SkPMetric gPDiff;

// A null terminated array of pointer to every differ declared above
SkImageDiffer* gDiffers[] = { &gDiffPixel, &gPDiff, NULL };

// A parallel array of functions to initialize the above differs. The reason we don't initialize
// everything immediately is that certain differs may require special initialization, but we still
// want to construct all of them globally so they can be queried for things like their name and
// description.
bool (*gDiffInits[])(SkImageDiffer*) = { init_cl_diff, init_dummy, NULL };


int main(int argc, char** argv) {
    // Setup command line parsing
    SkCommandLineFlags::SetUsage("Compare images using various metrics.");
    SkCommandLineFlags::Parse(argc, argv);

    // Needed by various Skia components
    SkAutoGraphics ag;

    if (FLAGS_list) {
        SkDebugf("Available Metrics:\n");
    }

    // Figure which differs the user chose, and optionally print them if the user requests it
    SkTDArray<SkImageDiffer*> chosenDiffers;
    for (int differIndex = 0; NULL != gDiffers[differIndex]; differIndex++) {
        SkImageDiffer* differ = gDiffers[differIndex];
        if (FLAGS_list) {
            SkDebugf("    %s", differ->getName());
            SkDebugf("\n");
        }

        // Check if this differ was chosen by any of the flags. Initialize them if they were chosen.
        if (FLAGS_differs.isEmpty()) {
            // If no differs were chosen, they all get added
            chosenDiffers.push(differ);
            gDiffInits[differIndex](differ);
        } else {
            for (int flagIndex = 0; flagIndex < FLAGS_differs.count(); flagIndex++) {
                if (SkString(FLAGS_differs[flagIndex]).equals(differ->getName())) {
                    chosenDiffers.push(differ);
                    gDiffInits[differIndex](differ);
                    break;
                }
            }
        }
    }

    // Don't attempt to initialize the differ if we aren't going to use it
    if (FLAGS_folders.isEmpty() && FLAGS_patterns.isEmpty()) {
        return 0;
    }

    // Validate command line flags
    if (!FLAGS_folders.isEmpty()) {
        if (2 != FLAGS_folders.count()) {
            SkDebugf("Folders flag expects two arguments: <baseline folder> <test folder>\n");
            return 1;
        }
    }

    if (!FLAGS_patterns.isEmpty()) {
        if (2 != FLAGS_patterns.count()) {
            SkDebugf("Patterns flag expects two arguments: <baseline pattern> <test pattern>\n");
            return 1;
        }
    }

    SkDiffContext ctx;
    ctx.setDiffers(chosenDiffers);

    // Perform a folder diff if one is requested
    if (!FLAGS_folders.isEmpty()) {
        ctx.diffDirectories(FLAGS_folders[0], FLAGS_folders[1]);
    }

    // Perform a pattern diff if one is requested
    if (!FLAGS_patterns.isEmpty()) {
        ctx.diffPatterns(FLAGS_patterns[0], FLAGS_patterns[1]);
    }

    // Output to the file specified
    if (!FLAGS_output.isEmpty()) {
        SkFILEWStream outputStream(FLAGS_output[0]);
        ctx.outputRecords(outputStream, FLAGS_jsonp);
    }

    return 0;
}
