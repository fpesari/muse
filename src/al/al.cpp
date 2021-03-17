//=============================================================================
//  AL
//  Audio Utility Library
//  $Id: al.cpp,v 1.1.2.2 2009/12/06 01:39:33 terminator356 Exp $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//=============================================================================

#include "al.h"

namespace AL {
      int sampleRate = 44100;
      int mtcType = 0;
      int division = 384;
      bool debugMsg = false;
      float denormalBias = 1e-18;
      }
