/*
 * Copyright (C) 2020 Veloman Yunkan
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

#include "istreamreader.h"
#include "buffer_reader.h"

namespace zim
{

////////////////////////////////////////////////////////////////////////////////
// IDataStream
////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<const Reader>
IStreamReader::sub_reader(zsize_t size)
{
  auto buffer = Buffer::makeBuffer(size);
  readImpl(const_cast<char*>(buffer.data()), size);
  return std::unique_ptr<Reader>(new BufferReader(buffer));
}

} // namespace zim
