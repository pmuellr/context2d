/*
 * Copyright 2013 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkBitmap.h"
#include "SkImageDecoder.h"
#include "SkOSFile.h"
#include "SkStream.h"

#include "SkDiffContext.h"
#include "SkImageDiffer.h"
#include "skpdiff_util.h"

SkDiffContext::SkDiffContext() {
    fRecords = NULL;
    fDiffers = NULL;
    fDifferCount = 0;
}

SkDiffContext::~SkDiffContext() {
    // Delete the record linked list
    DiffRecord* currentRecord = fRecords;
    while (NULL != currentRecord) {
        DiffRecord* nextRecord = currentRecord->fNext;
        SkDELETE(currentRecord);
        currentRecord = nextRecord;
    }

    if (NULL != fDiffers) {
        SkDELETE_ARRAY(fDiffers);
    }
}

void SkDiffContext::setDiffers(const SkTDArray<SkImageDiffer*>& differs) {
    // Delete whatever the last array of differs was
    if (NULL != fDiffers) {
        SkDELETE_ARRAY(fDiffers);
        fDiffers = NULL;
        fDifferCount = 0;
    }

    // Copy over the new differs
    fDifferCount = differs.count();
    fDiffers = SkNEW_ARRAY(SkImageDiffer*, fDifferCount);
    differs.copy(fDiffers);
}

void SkDiffContext::addDiff(const char* baselinePath, const char* testPath) {
    // Load the images at the paths
    SkBitmap baselineBitmap;
    SkBitmap testBitmap;
    if (!SkImageDecoder::DecodeFile(baselinePath, &baselineBitmap)) {
        SkDebugf("Failed to load bitmap \"%s\"\n", baselinePath);
        return;
    }
    if (!SkImageDecoder::DecodeFile(testPath, &testBitmap)) {
        SkDebugf("Failed to load bitmap \"%s\"\n", testPath);
        return;
    }

    // Setup a record for this diff
    DiffRecord* newRecord = SkNEW(DiffRecord);
    newRecord->fBaselinePath = baselinePath;
    newRecord->fTestPath = testPath;
    newRecord->fNext = fRecords;
    fRecords = newRecord;

    // Perform each diff
    for (int differIndex = 0; differIndex < fDifferCount; differIndex++) {
        SkImageDiffer* differ = fDiffers[differIndex];
        int diffID = differ->queueDiff(&baselineBitmap, &testBitmap);
        if (diffID >= 0) {

            // Copy the results into data for this record
            DiffData& diffData = newRecord->fDiffs.push_back();

            diffData.fDiffName = differ->getName();
            diffData.fResult = differ->getResult(diffID);

            int poiCount = differ->getPointsOfInterestCount(diffID);
            SkIPoint* poi = differ->getPointsOfInterest(diffID);
            diffData.fPointsOfInterest.append(poiCount, poi);

            // Because we are doing everything synchronously for now, we are done with the diff
            // after reading it.
            differ->deleteDiff(diffID);
        }
    }
}


void SkDiffContext::diffDirectories(const char baselinePath[], const char testPath[]) {
    // Get the files in the baseline, we will then look for those inside the test path
    SkTArray<SkString> baselineEntries;
    if (!get_directory(baselinePath, &baselineEntries)) {
        SkDebugf("Unable to open path \"%s\"\n", baselinePath);
        return;
    }

    for (int baselineIndex = 0; baselineIndex < baselineEntries.count(); baselineIndex++) {
        const char* baseFilename = baselineEntries[baselineIndex].c_str();

        // Find the real location of each file to compare
        SkString baselineFile = SkOSPath::SkPathJoin(baselinePath, baseFilename);
        SkString testFile = SkOSPath::SkPathJoin(testPath, baseFilename);

        // Check that the test file exists and is a file
        if (sk_exists(testFile.c_str()) && !sk_isdir(testFile.c_str())) {
            // Queue up the comparison with the differ
            this->addDiff(baselineFile.c_str(), testFile.c_str());
        } else {
            SkDebugf("Baseline file \"%s\" has no corresponding test file\n", baselineFile.c_str());
        }
    }
}


void SkDiffContext::diffPatterns(const char baselinePattern[], const char testPattern[]) {
    // Get the files in the baseline and test patterns. Because they are in sorted order, it's easy
    // to find corresponding images by matching entry indices.

    SkTArray<SkString> baselineEntries;
    if (!glob_files(baselinePattern, &baselineEntries)) {
        SkDebugf("Unable to get pattern \"%s\"\n", baselinePattern);
        return;
    }

    SkTArray<SkString> testEntries;
    if (!glob_files(testPattern, &testEntries)) {
        SkDebugf("Unable to get pattern \"%s\"\n", testPattern);
        return;
    }

    if (baselineEntries.count() != testEntries.count()) {
        SkDebugf("Baseline and test patterns do not yield corresponding number of files\n");
        return;
    }

    for (int entryIndex = 0; entryIndex < baselineEntries.count(); entryIndex++) {
        const char* baselineFilename = baselineEntries[entryIndex].c_str();
        const char* testFilename     = testEntries    [entryIndex].c_str();

        this->addDiff(baselineFilename, testFilename);
    }
}

void SkDiffContext::outputRecords(SkWStream& stream, bool useJSONP) {
    DiffRecord* currentRecord = fRecords;
    if (useJSONP) {
        stream.writeText("var SkPDiffRecords = {\n");
    }
    else
    {
        stream.writeText("{\n");
    }
    stream.writeText("    \"records\": [\n");
    while (NULL != currentRecord) {
        stream.writeText("        {\n");

            stream.writeText("            \"baselinePath\": \"");
            stream.writeText(currentRecord->fBaselinePath.c_str());
            stream.writeText("\",\n");

            stream.writeText("            \"testPath\": \"");
            stream.writeText(currentRecord->fTestPath.c_str());
            stream.writeText("\",\n");

            stream.writeText("            \"diffs\": [\n");
            for (int diffIndex = 0; diffIndex < currentRecord->fDiffs.count(); diffIndex++) {
                DiffData& data = currentRecord->fDiffs[diffIndex];
                stream.writeText("                {\n");

                    stream.writeText("                    \"differName\": \"");
                    stream.writeText(data.fDiffName);
                    stream.writeText("\",\n");

                    stream.writeText("                    \"result\": ");
                    stream.writeScalarAsText(data.fResult);
                    stream.writeText(",\n");

                    stream.writeText("                    \"pointsOfInterest\": [\n");
                    for (int poiIndex = 0; poiIndex < data.fPointsOfInterest.count(); poiIndex++) {
                        SkIPoint poi = data.fPointsOfInterest[poiIndex];
                        stream.writeText("                        [");
                        stream.writeDecAsText(poi.x());
                        stream.writeText(",");
                        stream.writeDecAsText(poi.y());
                        stream.writeText("]");

                        // JSON does not allow trailing commas
                        if (poiIndex + 1 < data.fPointsOfInterest.count())
                        {
                            stream.writeText(",");
                        }
                        stream.writeText("\n");
                    }
                    stream.writeText("                    ]\n");
                stream.writeText("                }");

                // JSON does not allow trailing commas
                if (diffIndex + 1 < currentRecord->fDiffs.count())
                {
                    stream.writeText(",");
                }
                stream.writeText("                \n");
            }
            stream.writeText("            ]\n");

        stream.writeText("        }");

        // JSON does not allow trailing commas
        if (NULL != currentRecord->fNext)
        {
            stream.writeText(",");
        }
        stream.writeText("\n");
        currentRecord = currentRecord->fNext;
    }
    stream.writeText("    ]\n");
    if (useJSONP) {
        stream.writeText("};\n");
    }
    else
    {
        stream.writeText("}\n");
    }
}
