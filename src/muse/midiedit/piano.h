//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: piano.h,v 1.2 2004/05/31 11:48:55 lunar_shuttle Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2012 Tim E. Real (terminator356 on users dot sourceforge dot net)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
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
//
//=========================================================

#ifndef __PIANO_H__
#define __PIANO_H__

#include <QRect>
#include <QRegion>

#include "view.h"


// Forward declarations:
class QEvent;
class QMouseEvent;
class QWheelEvent;
class QPainter;
class QPixmap;

#define KH  13

namespace MusEGui {

class MidiEditor;

//---------------------------------------------------------
//   Piano
//---------------------------------------------------------

class Piano : public View
      {
      Q_OBJECT

      MidiEditor* _midiEditor;
      int curPitch;
      int selectedPitch;
      int keyDown;
      bool shift;
      int button;
      int pianoWidth;

      
      int y2pitch(int) const;
      int pitch2y(int) const;
      void viewMouseMoveEvent(QMouseEvent* event) override;
      void leaveEvent(QEvent*e) override;
      void keyReleaseEvent(QKeyEvent *event) override;

      void viewMousePressEvent(QMouseEvent* event) override;
      void viewMouseReleaseEvent(QMouseEvent*) override;
      void wheelEvent(QWheelEvent* e) override;

   protected:
      void draw(QPainter&, const QRect&, const QRegion& = QRegion()) override;

   signals:
      void pitchChanged(int);
      void keyPressed(int, int, bool);
      void keyReleased(int, bool);
      void curSelectedPitchChanged(int);
      void redirectWheelEvent(QWheelEvent*);
      void wheelStep(bool);
      void shiftReleased();

   public slots:
      void setPitch(int);

   public:
      Piano(QWidget* parent, int ymag, int width, MidiEditor* editor = 0);
      int curSelectedPitch() const { return selectedPitch; }
      void setCurSelectedPitch(int pitch);
      };

} // namespace MusEGui
      
#endif

