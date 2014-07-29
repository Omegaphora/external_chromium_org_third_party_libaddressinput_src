// Copyright (C) 2013 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "fake_downloader.h"

#include <libaddressinput/callback.h>
#include <libaddressinput/downloader.h>
#include <libaddressinput/util/basictypes.h>
#include <libaddressinput/util/scoped_ptr.h>

#include <cstddef>
#include <string>

#include <gtest/gtest.h>

#include "region_data_constants.h"

namespace {

using i18n::addressinput::BuildCallback;
using i18n::addressinput::Downloader;
using i18n::addressinput::FakeDownloader;
using i18n::addressinput::RegionDataConstants;
using i18n::addressinput::scoped_ptr;

// Tests for FakeDownloader object.
class FakeDownloaderTest : public testing::TestWithParam<std::string> {
 protected:
  FakeDownloaderTest()
      : downloader_(),
        success_(false),
        url_(),
        data_(),
        downloaded_(BuildCallback(this, &FakeDownloaderTest::OnDownloaded)) {}

  virtual ~FakeDownloaderTest() {}

  FakeDownloader downloader_;
  bool success_;
  std::string url_;
  std::string data_;
  const scoped_ptr<const Downloader::Callback> downloaded_;

 private:
  void OnDownloaded(bool success, const std::string& url, std::string* data) {
    ASSERT_FALSE(success && data == NULL);
    success_ = success;
    url_ = url;
    if (data != NULL) {
      data_ = *data;
      delete data;
    }
  }

  DISALLOW_COPY_AND_ASSIGN(FakeDownloaderTest);
};

// Returns testing::AssertionSuccess if |data| is valid downloaded data for
// |key|.
testing::AssertionResult DataIsValid(const std::string& data,
                                     const std::string& key) {
  if (data.empty()) {
    return testing::AssertionFailure() << "empty data";
  }

  std::string expected_data_begin = "{\"id\":\"" + key + "\"";
  if (data.compare(0, expected_data_begin.length(), expected_data_begin) != 0) {
    return testing::AssertionFailure() << data << " does not begin with "
                                       << expected_data_begin;
  }

  // Verify that the data ends on "}.
  static const char kDataEnd[] = "\"}";
  static const size_t kDataEndLength = sizeof kDataEnd - 1;
  if (data.compare(data.length() - kDataEndLength,
                   kDataEndLength,
                   kDataEnd,
                   kDataEndLength) != 0) {
    return testing::AssertionFailure() << data << " does not end with "
                                       << kDataEnd;
  }

  return testing::AssertionSuccess();
}

// Verifies that FakeDownloader downloads valid data for a region code.
TEST_P(FakeDownloaderTest, FakeDownloaderHasValidDataForRegion) {
  std::string key = "data/" + GetParam();
  std::string url = std::string(FakeDownloader::kFakeDataUrl) + key;
  downloader_.Download(url, *downloaded_);

  EXPECT_TRUE(success_);
  EXPECT_EQ(url, url_);
  EXPECT_TRUE(DataIsValid(data_, key));
};

// Returns testing::AssertionSuccess if |data| is valid aggregated downloaded
// data for |key|.
testing::AssertionResult AggregateDataIsValid(const std::string& data,
                                              const std::string& key) {
  if (data.empty()) {
    return testing::AssertionFailure() << "empty data";
  }

  std::string expected_data_begin = "{\"" + key;
  if (data.compare(0, expected_data_begin.length(), expected_data_begin) != 0) {
    return testing::AssertionFailure() << data << " does not begin with "
                                       << expected_data_begin;
  }

  // Verify that the data ends on "}}.
  static const char kDataEnd[] = "\"}}";
  static const size_t kDataEndLength = sizeof kDataEnd - 1;
  if (data.compare(data.length() - kDataEndLength,
                   kDataEndLength,
                   kDataEnd,
                   kDataEndLength) != 0) {
    return testing::AssertionFailure() << data << " does not end with "
                                       << kDataEnd;
  }

  return testing::AssertionSuccess();
}

// Verifies that FakeDownloader downloads valid aggregated data for a region
// code.
TEST_P(FakeDownloaderTest, FakeDownloaderHasValidAggregatedDataForRegion) {
  std::string key = "data/" + GetParam();
  std::string url = std::string(FakeDownloader::kFakeAggregateDataUrl) + key;
  downloader_.Download(url, *downloaded_);

  EXPECT_TRUE(success_);
  EXPECT_EQ(url, url_);
  EXPECT_TRUE(AggregateDataIsValid(data_, key));
};

// Test all region codes.
INSTANTIATE_TEST_CASE_P(
    AllRegions, FakeDownloaderTest,
    testing::ValuesIn(RegionDataConstants::GetRegionCodes()));

// Verifies that the key "data" also contains valid data.
TEST_F(FakeDownloaderTest, DownloadExistingData) {
  static const std::string kKey = "data";
  static const std::string kUrl =
      std::string(FakeDownloader::kFakeDataUrl) + kKey;
  downloader_.Download(kUrl, *downloaded_);

  EXPECT_TRUE(success_);
  EXPECT_EQ(kUrl, url_);
  EXPECT_TRUE(DataIsValid(data_, kKey));
}

// Verifies that downloading a missing key will return "{}".
TEST_F(FakeDownloaderTest, DownloadMissingKeyReturnsEmptyDictionary) {
  static const std::string kJunkUrl =
      std::string(FakeDownloader::kFakeDataUrl) + "junk";
  downloader_.Download(kJunkUrl, *downloaded_);

  EXPECT_TRUE(success_);
  EXPECT_EQ(kJunkUrl, url_);
  EXPECT_EQ("{}", data_);
}

// Verifies that aggregate downloading of a missing key will also return "{}".
TEST_F(FakeDownloaderTest, AggregateDownloadMissingKeyReturnsEmptyDictionary) {
  static const std::string kJunkUrl =
      std::string(FakeDownloader::kFakeAggregateDataUrl) + "junk";
  downloader_.Download(kJunkUrl, *downloaded_);

  EXPECT_TRUE(success_);
  EXPECT_EQ(kJunkUrl, url_);
  EXPECT_EQ("{}", data_);
}

// Verifies that downloading an empty key will return "{}".
TEST_F(FakeDownloaderTest, DownloadEmptyKeyReturnsEmptyDictionary) {
  static const std::string kPrefixOnlyUrl = FakeDownloader::kFakeDataUrl;
  downloader_.Download(kPrefixOnlyUrl, *downloaded_);

  EXPECT_TRUE(success_);
  EXPECT_EQ(kPrefixOnlyUrl, url_);
  EXPECT_EQ("{}", data_);
}

// Verifies that downloading a real URL fails.
TEST_F(FakeDownloaderTest, DownloadRealUrlFals) {
  static const std::string kRealUrl = "http://www.google.com/";
  downloader_.Download(kRealUrl, *downloaded_);

  EXPECT_FALSE(success_);
  EXPECT_EQ(kRealUrl, url_);
  EXPECT_TRUE(data_.empty());
}

}  // namespace
