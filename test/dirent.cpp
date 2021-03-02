/*
 * Copyright (C) 2009 Tommi Maekitalo
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

#include <cstring>
#include <iostream>
#include <sstream>
#include <memory>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <fileapi.h>
#endif

#include "gtest/gtest.h"

#include "../src/buffer.h"
#include "../src/_dirent.h"
#include "../src/direntreader.h"
#include "../src/buffer_reader.h"
#include "../src/writer/_dirent.h"

#include "tools.h"

namespace
{

using zim::unittests::TempFile;
using zim::unittests::write_to_buffer;

zim::Dirent read_from_buffer(const zim::Buffer& buf)
{
  zim::DirentReader direntReader(std::make_shared<zim::BufferReader>(buf));
  return *direntReader.readDirent(zim::offset_t(0));
}

size_t writenDirentSize(const zim::writer::Dirent& dirent)
{
  TempFile tmpFile("test_dirent");
  const auto tmp_fd = tmpFile.fd();
  dirent.write(tmp_fd);
  auto size = lseek(tmp_fd, 0, SEEK_END);
  return size;
}

TEST(DirentTest, set_get_data_dirent)
{
  zim::Dirent dirent;
  dirent.setUrl('A', "Bar");
  dirent.setItem(17, zim::cluster_index_t(45), zim::blob_index_t(1234));
  dirent.setVersion(54346);

  ASSERT_TRUE(!dirent.isRedirect());
  ASSERT_EQ(dirent.getNamespace(), 'A');
  ASSERT_EQ(dirent.getUrl(), "Bar");
  ASSERT_EQ(dirent.getTitle(), "Bar");
  ASSERT_EQ(dirent.getParameter(), "");
  ASSERT_EQ(dirent.getClusterNumber().v, 45U);
  ASSERT_EQ(dirent.getBlobNumber().v, 1234U);
  ASSERT_EQ(dirent.getVersion(), 54346U);

  dirent.setTitle("Foo");
  ASSERT_EQ(dirent.getNamespace(), 'A');
  ASSERT_EQ(dirent.getUrl(), "Bar");
  ASSERT_EQ(dirent.getTitle(), "Foo");
  ASSERT_EQ(dirent.getParameter(), "");
}

TEST(DirentTest, read_write_article_dirent)
{
  zim::writer::Dirent dirent;
  dirent.setNamespace('A');
  dirent.setPath("Bar");
  dirent.setTitle("Foo");
  dirent.setItem(17, zim::cluster_index_t(45), zim::blob_index_t(1234));

  ASSERT_TRUE(!dirent.isRedirect());
  ASSERT_EQ(dirent.getNamespace(), 'A');
  ASSERT_EQ(dirent.getPath(), "Bar");
  ASSERT_EQ(dirent.getTitle(), "Foo");
  ASSERT_EQ(dirent.getClusterNumber().v, 45U);
  ASSERT_EQ(dirent.getBlobNumber().v, 1234U);
  ASSERT_EQ(dirent.getVersion(), 0U);

  auto buffer = write_to_buffer(dirent);
  zim::Dirent dirent2(read_from_buffer(buffer));

  ASSERT_TRUE(!dirent2.isRedirect());
  ASSERT_EQ(dirent2.getNamespace(), 'A');
  ASSERT_EQ(dirent2.getTitle(), "Foo");
  ASSERT_EQ(dirent2.getParameter(), "");
  ASSERT_EQ(dirent2.getClusterNumber().v, 45U);
  ASSERT_EQ(dirent2.getBlobNumber().v, 1234U);
  ASSERT_EQ(dirent2.getVersion(), 0U);
}

TEST(DirentTest, read_write_article_dirent_unicode)
{
  zim::writer::Dirent dirent;
  dirent.setNamespace('A');
  dirent.setPath("L\xc3\xbcliang");
  dirent.setItem(17, zim::cluster_index_t(45), zim::blob_index_t(1234));

  ASSERT_TRUE(!dirent.isRedirect());
  ASSERT_EQ(dirent.getNamespace(), 'A');
  ASSERT_EQ(dirent.getPath(), "L\xc3\xbcliang");
  ASSERT_EQ(dirent.getTitle(), "L\xc3\xbcliang");
  ASSERT_EQ(dirent.getClusterNumber().v, 45U);
  ASSERT_EQ(dirent.getBlobNumber().v, 1234U);

  auto buffer = write_to_buffer(dirent);
  zim::Dirent dirent2(read_from_buffer(buffer));

  ASSERT_TRUE(!dirent2.isRedirect());
  ASSERT_EQ(dirent2.getNamespace(), 'A');
  ASSERT_EQ(dirent2.getUrl(), "L\xc3\xbcliang");
  ASSERT_EQ(dirent2.getTitle(), "L\xc3\xbcliang");
  ASSERT_EQ(dirent2.getParameter(), "");
  ASSERT_EQ(dirent2.getClusterNumber().v, 45U);
  ASSERT_EQ(dirent2.getBlobNumber().v, 1234U);
}

TEST(DirentTest, read_write_redirect_dirent)
{
  zim::writer::Dirent targetDirent;
  targetDirent.setIdx(zim::entry_index_t(321));
  zim::writer::Dirent dirent;
  dirent.setNamespace('A');
  dirent.setPath("Bar");
  dirent.setRedirect(&targetDirent);

  ASSERT_TRUE(dirent.isRedirect());
  ASSERT_EQ(dirent.getNamespace(), 'A');
  ASSERT_EQ(dirent.getPath(), "Bar");
  ASSERT_EQ(dirent.getRedirectIndex().v, 321U);

  auto buffer = write_to_buffer(dirent);
  zim::Dirent dirent2(read_from_buffer(buffer));

  ASSERT_TRUE(dirent2.isRedirect());
  ASSERT_EQ(dirent2.getNamespace(), 'A');
  ASSERT_EQ(dirent2.getUrl(), "Bar");
  ASSERT_EQ(dirent2.getTitle(), "Bar");
  ASSERT_EQ(dirent2.getRedirectIndex().v, 321U);
}

TEST(DirentTest, dirent_size)
{
  zim::writer::Dirent dirent;
  std::string s;
  dirent.setItem(17, zim::cluster_index_t(45), zim::blob_index_t(1234));
  dirent.setNamespace('A');
  dirent.setPath("Bar");

  // case url set, title empty, extralen empty
  ASSERT_EQ(dirent.getDirentSize(), writenDirentSize(dirent));

  // case url set, title set, extralen empty
  dirent.setTitle("Foo");
  ASSERT_EQ(dirent.getDirentSize(), writenDirentSize(dirent));

  // case url set, title empty
  dirent.setTitle(std::string());
  ASSERT_EQ(dirent.getDirentSize(), writenDirentSize(dirent));
}

TEST(DirentTest, redirect_dirent_size)
{
  zim::writer::Dirent targetDirent;
  targetDirent.setIdx(zim::entry_index_t(321));
  zim::writer::Dirent dirent;
  dirent.setNamespace('A');
  dirent.setPath("Bar");
  dirent.setRedirect(&targetDirent);

  ASSERT_EQ(dirent.getDirentSize(), writenDirentSize(dirent));
}

}  // namespace
