/*
 * Copyright (C) 2021 Maneesh P M <manu.pm55@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * is provided AS IS, WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, and
 * NON-INFRINGEMENT.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

#include <zim/zim.h>
#include <zim/archive.h>
#include <zim/search.h>
#include <zim/writer/creator.h>
#include <zim/writer/item.h>
#include <zim/writer/contentProvider.h>

#include "tools.h"

#include "gtest/gtest.h"

namespace {

  class TestItem : public zim::writer::BasicItem {
      public:
      TestItem(const std::string& path, const std::string& mimetype, const std::string& title)
        : BasicItem(path, mimetype, title) {}

      virtual std::unique_ptr<zim::writer::ContentProvider> getContentProvider() const {
        return std::unique_ptr<zim::writer::ContentProvider>(new zim::writer::StringProvider(""));
      }
  };

  // Helper class to create a temporary zim from titles list
  // Remove the temporary file once test is done.
  class TempZimArchive : zim::unittests::TempFile {
    public:
      explicit TempZimArchive(const char* tempPath) : zim::unittests::TempFile {tempPath} {}

      zim::Archive createZimFromTitles(std::vector<std::string> titles) {
        zim::writer::Creator creator;
        creator.configIndexing(true, "eng");
        creator.startZimCreation(this->path());

        // add dummy items with given titles
        for (auto title : titles) {
          std::string path = "dummyPath" + title;
          auto item = std::make_shared<TestItem>(path, "plain/text", title);
          creator.addItem(item);
        }
        creator.addMetadata("Title", "This is a title");
        creator.finishZimCreation();
        return zim::Archive(this->path());
      }
  };

  std::vector<std::string> getSuggestions(const zim::Archive archive, std::string query, int range) {
    zim::Search search(archive);
    search.set_suggestion_mode(true);
    search.set_query(query);
    search.set_range(0, range);
    search.set_verbose(true);

    std::vector<std::string> result;
    for (auto entry = search.begin();entry!=search.end();entry++) {
      std::cout<<(*entry).getTitle()<<entry.get_score()<<std::endl;
      result.push_back((*entry).getTitle());
    }

    return result;
  }

  TEST(Suggestion, emptyQuery) {
    std::vector<std::string> titles = {
                                        "fooland",
                                        "berlin wall",
                                        "hotel berlin, berlin",
                                        "again berlin",
                                        "berlin",
                                        "not berlin"
                                       };

    TempZimArchive tza("testZim");
    const zim::Archive archive = tza.createZimFromTitles(titles);

    std::vector<std::string> resultSet = getSuggestions(archive, "", archive.getEntryCount());
    std::vector<std::string> expectedResult = {};

    ASSERT_EQ(resultSet, expectedResult);
  }

  TEST(Suggestion, noResult) {
    std::vector<std::string> titles = {
                                        "fooland"
                                        "berlin wall",
                                        "hotel berlin, berlin",
                                        "again berlin",
                                        "berlin",
                                        "not berlin"
                                       };

    TempZimArchive tza("testZim");
    const zim::Archive archive = tza.createZimFromTitles(titles);

    std::vector<std::string> resultSet = getSuggestions(archive, "none", archive.getEntryCount());
    std::vector<std::string> expectedResult = {};

    ASSERT_EQ(resultSet, expectedResult);
  }

  TEST(Suggestion, singleTermOrder) {
    std::vector<std::string> titles = {
                                        "fooland",
                                        "berlin wall",
                                        "hotel berlin, berlin",
                                        "again berlin",
                                        "berlin",
                                        "not berlin"
                                       };

    TempZimArchive tza("testZim");
    const zim::Archive archive = tza.createZimFromTitles(titles);

    std::vector<std::string> resultSet = getSuggestions(archive, "berlin", archive.getEntryCount());
    std::vector<std::string> expectedResult = {
                                        "berlin",
                                        "hotel berlin, berlin",
                                        "again berlin",
                                        "berlin wall",
                                        "not berlin"
                                       };

    ASSERT_EQ(expectedResult , resultSet);
  }

  TEST(Suggestion, resultsGreaterThanLimit) {
    std::vector<std::string> titles = {
                                        "foobar b",
                                        "foobar a",
                                        "foobar c",
                                        "foobar e",
                                        "foobar d"
                                       };

    TempZimArchive tza("testZim");
    const zim::Archive archive = tza.createZimFromTitles(titles);

    std::vector<std::string> resultSet = getSuggestions(archive, "foobar", 2);
    std::vector<std::string> expectedResult = {
                                    "foobar a",
                                    "foobar b"
                                    };

    ASSERT_EQ(expectedResult, resultSet);
  }

  TEST(Suggestion, partialQuery) {
    std::vector<std::string> titles = {
                                        "The chocolate factory",
                                        "The wolf of Shingashina",
                                        "The wolf of Wall Street",
                                        "Hour of the wolf",
                                        "Wolf",
                                        "Terma termb the wolf of wall street termc"
                                       };

    TempZimArchive tza("testZim");
    const zim::Archive archive = tza.createZimFromTitles(titles);

    // "wo"
    std::vector<std::string> resultSet = getSuggestions(archive, "Wo", archive.getEntryCount());
    std::vector<std::string> expectedResult = {
                      "Wolf",
                      "Hour of the wolf",
                      "The wolf of Shingashina",
                      "The wolf of Wall Street",
                      "Terma termb the wolf of wall street termc"
                    };

    ASSERT_EQ(expectedResult, resultSet);
  }

  TEST(Suggestion, phraseOrder) {
    std::vector<std::string> titles = {
                                        "Summer in Berlin",
                                        "In Summer",
                                        "Shivers in summer",
                                        "Summer in Paradise",
                                        "In mid Summer",
                                        "In the winter"
                                       };

    TempZimArchive tza("testZim");
    const zim::Archive archive = tza.createZimFromTitles(titles);

    std::vector<std::string> resultSet = getSuggestions(archive, "summer in", archive.getEntryCount());
    std::vector<std::string> expectedResult = {
                                        "Summer in Berlin",
                                        "Summer in Paradise",
                                        "In Summer",
                                        "In mid Summer",
                                        "Shivers in summer"
                                       };

    ASSERT_EQ(expectedResult, resultSet);
  }

    TEST(Suggestion, incrementalSearch) {
    std::vector<std::string> titles = {
                                        "The chocolate factory",
                                        "The wolf of Shingashina",
                                        "The wolf of Wall Street",
                                        "The wolf among sheeps",
                                        "The wolf of Wall Street Book" ,
                                        "Hour of the wolf",
                                        "Wolf",
                                        "Terma termb the wolf of wall street termc"
                                       };

    std::vector<std::string> resultSet, expectedResult;

    TempZimArchive tza("testZim");
    const zim::Archive archive = tza.createZimFromTitles(titles);

    // "wolf"
    resultSet = getSuggestions(archive, "Wolf", archive.getEntryCount());
    expectedResult = {
                      "Wolf",
                      "Hour of the wolf",
                      "The wolf among sheeps",
                      "The wolf of Shingashina",
                      "The wolf of Wall Street",
                      "The wolf of Wall Street Book",
                      "Terma termb the wolf of wall street termc"
                    };

    ASSERT_EQ(expectedResult, resultSet);

    // "the"
    resultSet = getSuggestions(archive, "the", archive.getEntryCount());
    expectedResult = {
                      "The chocolate factory",
                      "Hour of the wolf",
                      "The wolf among sheeps",
                      "The wolf of Shingashina",
                      "The wolf of Wall Street",
                      "The wolf of Wall Street Book",
                      "Terma termb the wolf of wall street termc"
                    };

    ASSERT_EQ(expectedResult, resultSet);

    // "the wolf"
    resultSet = getSuggestions(archive, "the wolf", archive.getEntryCount());
    expectedResult = {
                      "Hour of the wolf",
                      "The wolf among sheeps",
                      "The wolf of Shingashina",
                      "The wolf of Wall Street",
                      "The wolf of Wall Street Book",
                      "Terma termb the wolf of wall street termc"
                    };

    ASSERT_EQ(expectedResult, resultSet);

    // "the wolf of"
    resultSet = getSuggestions(archive, "the wolf of", archive.getEntryCount());
    expectedResult = {
                      "The wolf of Shingashina",
                      "The wolf of Wall Street",
                      "The wolf of Wall Street Book",
                      "Terma termb the wolf of wall street termc",
                      "Hour of the wolf"
                    };

    ASSERT_EQ(expectedResult, resultSet);

    // "the wolf of wall"
    resultSet = getSuggestions(archive, "the wolf of wall", archive.getEntryCount());
    expectedResult = {
                      "The wolf of Wall Street",
                      "The wolf of Wall Street Book",
                      "Terma termb the wolf of wall street termc"
                    };

    ASSERT_EQ(expectedResult, resultSet);
  }

  TEST(Suggestion, phraseOutOfWindow) {
    std::vector<std::string> titles = {
                                        "This query",
                                        "This is the dummy query phrase",
                                        "the aterm bterm dummy cterm query",
                                        "aterm the bterm dummy query cterm"
                                       };

    TempZimArchive tza("testZim");
    const zim::Archive archive = tza.createZimFromTitles(titles);

    std::vector<std::string> resultSet = getSuggestions(archive, "the dummy query", archive.getEntryCount());
    std::vector<std::string> expectedResult = {
                                        "This is the dummy query phrase",
                                        "aterm the bterm dummy query cterm",
                                        "the aterm bterm dummy cterm query"
                                       };

    ASSERT_EQ(expectedResult, resultSet);
  }
}
