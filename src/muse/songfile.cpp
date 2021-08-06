//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: songfile.cpp,v 1.25.2.12 2009/11/04 15:06:07 spamatica Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
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

#include <map>

#include <QUuid>
#include <QProgressDialog>
#include <QMessageBox>
#include <QCheckBox>
#include <QString>

#include "app.h"
#include "song.h"
#include "arranger.h"
#include "arrangerview.h"
#include "cobject.h"
#include "drumedit.h"
#include "pianoroll.h"
#include "scoreedit.h"
#include "globals.h"
#include "xml.h"
#include "drummap.h"
#include "drum_ordering.h"
#include "event.h"
#include "marker/marker.h"
#include "midiport.h"
#include "audio.h"
#include "mitplugin.h"
#include "wave.h"
#include "midictrl.h"
#include "audiodev.h"
#include "conf.h"
#include "keyevent.h"
#include "gconfig.h"
#include "config.h"

namespace MusEGlobal {
MusECore::CloneList cloneList;
}

namespace MusECore {


//---------------------------------------------------------
//   NKey::write
//---------------------------------------------------------

void NKey::write(int level, Xml& xml) const
      {
      xml.intTag(level, "key", val);
      }

//---------------------------------------------------------
//   NKey::read
//---------------------------------------------------------

void NKey::read(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::Text:
                        val = xml.s1().toInt();
                        break;
                  case Xml::TagEnd:
                        if (xml.s1() == "key")
                              return;
                  default:
                        break;
                  }
            }
      }


//---------------------------------------------------------
//   Scale::write
//---------------------------------------------------------

void Scale::write(int level, Xml& xml) const
      {
      xml.intTag(level, "scale", val);
      }

//---------------------------------------------------------
//   Scale::read
//---------------------------------------------------------

void Scale::read(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::Text:
                        val = xml.s1().toInt();
                        break;
                  case Xml::TagEnd:
                        if (xml.s1() == "scale")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   Part::readFromXml
//---------------------------------------------------------

Part* Part::readFromXml(Xml& xml, Track* track, bool doClone, bool toTrack)
      {
      int id = -1;
      Part* npart = nullptr;
      QUuid uuid;
      bool uuidvalid = false;
      bool clone = true;
      bool wave = false;
      bool isclone = false;

      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return npart;
                  case Xml::TagStart:
                        if(!npart) // If the part has not been created yet...
                        {
                          if(id != -1) // If an id was found...
                          {
                            for(iClone i = MusEGlobal::cloneList.begin(); i != MusEGlobal::cloneList.end(); ++i)
                            {
                              if(i->is_deleted) // Is the clone item marked as deleted? Ignore it.
                                continue;
                              if(i->id == id) // Is a matching part found in the clone list?
                              {
                                // Create a clone. It must still be added later in a operationgroup
                                npart = track->newPart((Part*)i->cp, true);
                                break;
                              }
                            }
                          }
                          else if(uuidvalid) // If a uuid was found...
                          {
                            for(iClone i = MusEGlobal::cloneList.begin(); i != MusEGlobal::cloneList.end(); ++i)
                            {
                              if(uuid == i->_uuid) // Is a matching part found in the clone list?
                              {
                                Track* cpt = i->cp->track();
                                if(toTrack) // If we want to paste to the given track...
                                {
                                  // If the given track type is not the same as the part's
                                  //  original track type, we can't continue. Just return.
                                  if(!track || cpt->type() != track->type())
                                  {
                                    xml.skip("part");
                                    return nullptr;
                                  }
                                }
                                else // ...else we want to paste to the part's original track.
                                {
                                  // Make sure the track exists (has not been deleted).
                                  if((cpt->isMidiTrack() && MusEGlobal::song->midis()->find(cpt) != MusEGlobal::song->midis()->end()) ||
                                      (cpt->type() == Track::WAVE && MusEGlobal::song->waves()->find(cpt) != MusEGlobal::song->waves()->end()))
                                    track = cpt;
                                  else // Track was not found. Try pasting to the given track, as above...
                                  {
                                    if(!track || cpt->type() != track->type())
                                    {
                                      // No luck. Just return.
                                      xml.skip("part");
                                      return nullptr;
                                    }
                                  }
                                }

                                if(i->is_deleted) // Is the clone item marked as deleted? Don't create a clone, create a copy.
                                  break;

                                // If it's a regular paste (not paste clone), and the original part is
                                //  not a clone, defer so that a new copy is created in TagStart above.
                                if(!doClone && !isclone)
                                  break;

                                // Create a clone. It must still be added later in a operationgroup
                                npart = track->newPart((Part*)i->cp, true);
                                break;
                              }
                            }
                          }

                          if(!npart) // If the part still has not been created yet...
                          {

                            if(!track) // A clone was not created from any matching
                            {          // part. Create a non-clone part now.
                              xml.skip("part");
                              return nullptr;
                            }
                            // If we're pasting to selected track and the 'wave'
                            //  variable is valid, check for mismatch...
                            if(toTrack && uuidvalid)
                            {
                              // If both the part and track are not midi or wave...
                              if((wave && track->isMidiTrack()) ||
                                (!wave && track->type() == Track::WAVE))
                              {
                                xml.skip("part");
                                return nullptr;
                              }
                            }

                            if (track->isMidiTrack())
                              npart = new MidiPart((MidiTrack*)track);
                            else if (track->type() == Track::WAVE)
                              npart = new MusECore::WavePart((MusECore::WaveTrack*)track);
                            else
                            {
                              xml.skip("part");
                              return nullptr;
                            }

                            // Signify a new non-clone part was created.
                            // Even if the original part was itself a clone, clear this because the
                            //  attribute section did not create a clone from any matching part.
                            clone = false;

                            // If an id or uuid was found, add the part to the clone list
                            //  so that subsequent parts can look it up and clone from it...
                            if(id != -1)
                            {
                              ClonePart ncp(npart, id);
                              MusEGlobal::cloneList.push_back(ncp);
                            }
                            else
                            if(uuidvalid)
                            {
                              ClonePart ncp(npart);
                              // New ClonePart creates its own uuid, but we need to replace it.
                              ncp._uuid = uuid; // OK for non-windows?
                              MusEGlobal::cloneList.push_back(ncp);
                            }
                          }
                        }

                        if (tag == "name")
                              npart->setName(xml.parse1());
                        else if (tag == "viewState") {
                              npart->viewState().read(xml);
                              }
                        else if (tag == "poslen") {
                              ((PosLen*)npart)->read(xml, "poslen");
                              }
                        else if (tag == "pos") {
                              Pos pos;
                              pos.read(xml, "pos");  // obsolete
                              npart->setTick(pos.tick());
                              }
                        else if (tag == "len") {
                              Pos len;
                              len.read(xml, "len");  // obsolete
                              npart->setLenTick(len.tick());
                              }
                        else if (tag == "selected")
                              npart->setSelected(xml.parseInt());
                        else if (tag == "color")
                               npart->setColorIndex(xml.parseInt());
                        else if (tag == "mute")
                              npart->setMute(xml.parseInt());
                        else if (tag == "event")
                        {
                              // If a new non-clone part was created, accept the events...
                              if(!clone)
                              {
                                EventType type = Wave;
                                if(track->isMidiTrack())
                                  type = Note;
                                Event e(type);
                                e.read(xml);
                                // stored pos for event has absolute value. However internally
                                // pos is relative to start of part, we substract part pos.
                                // In case the event and part pos types differ, the event dominates.
                                e.setPosValue(e.posValue() - npart->posValue(e.pos().type()));
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
                                npart->addEvent(e);
#else
                                const int posval = e.posValue();
                                if(posval < 0)
                                {
                                  printf("readClone: warning: event at posval:%d not in part:%s, discarded\n",
                                    posval, npart->name().toLatin1().constData());
                                }
#endif
                              }
                              else // ...Otherwise a clone was created, so we don't need the events.
                                xml.skip(tag);
                        }
                        else
                              xml.unknown("readXmlPart");
                        break;
                  case Xml::Attribut:
                        if (tag == "type")
                        {
                          if(xml.s2() == "wave")
                            wave = true;
                        }
                        else if (tag == "cloneId")
                        {
                          id = xml.s2().toInt();
                        }
                        else if (tag == "uuid")
                        {
                          uuid = QUuid(xml.s2());
                          if(!uuid.isNull())
                          {
                            uuidvalid = true;
                          }
                        }
                        else if(tag == "isclone")
                          isclone = xml.s2().toInt();
                        break;
                  case Xml::TagEnd:
                        if (tag == "part")
                          return npart;
                  default:
                        break;
                  }
            }
  return npart;
}

//---------------------------------------------------------
//   Part::write
//   If isCopy is true, write the xml differently so that
//    we can have 'Paste Clone' feature.
//---------------------------------------------------------

void Part::write(int level, Xml& xml, bool isCopy, bool forceWavePaths) const
      {
      int id              = -1;
      QUuid uuid;
      bool dumpEvents     = true;
      bool wave = _track->type() == Track::WAVE;

      // NOTE ::write() should never be called on a deleted part, so no checking here for cloneList items marked as deleted.
      //      Checking is awkward anyway and doesn't fit well here.

      if(isCopy)
      {
        for(iClone i = MusEGlobal::cloneList.begin(); i != MusEGlobal::cloneList.end(); ++i)
        {
          if(i->cp->isCloneOf(this))
          {
            uuid = i->_uuid;
            dumpEvents = false;
            break;
          }
        }
        if(uuid.isNull())
        {
          ClonePart cp(this);
          uuid = cp._uuid;
          MusEGlobal::cloneList.push_back(cp);
        }
      }
      else
      {
        if (this->hasClones())
        {
          for (iClone i = MusEGlobal::cloneList.begin(); i != MusEGlobal::cloneList.end(); ++i)
          {
            if (i->cp->isCloneOf(this))
            {
              id = i->id;
              dumpEvents = false;
              break;
            }
          }
          if (id == -1)
          {
            id = MusEGlobal::cloneList.size();
            ClonePart cp(this, id);
            MusEGlobal::cloneList.push_back(cp);
          }
        }
      }

      // Special markers if this is a copy operation and the
      //  part is a clone.
      if(isCopy)
      {
        if(wave)
          xml.nput(level, "<part type=\"wave\" uuid=\"%s\"", uuid.toByteArray().constData());
        else
          xml.nput(level, "<part uuid=\"%s\"", uuid.toByteArray().constData());

        if(hasClones())
          xml.nput(" isclone=\"1\"");
        xml.put(">");
        level++;
      }
      else
      if (id != -1)
      {
        xml.tag(level++, "part cloneId=\"%d\"", id);
      }
      else
        xml.tag(level++, "part");

      xml.strTag(level, "name", _name);

      // This won't bother writing if the state is invalid.
      viewState().write(level, xml);

      PosLen::write(level, xml, "poslen");
      xml.intTag(level, "selected", _selected);
      xml.intTag(level, "color", _colorIndex);
      if (_mute)
            xml.intTag(level, "mute", _mute);
      if (dumpEvents) {
            for (ciEvent e = events().begin(); e != events().end(); ++e)
                  e->second.write(level, xml, *this, forceWavePaths);
            }
      xml.etag(level, "part");
      }


//---------------------------------------------------------
//   writeFont
//---------------------------------------------------------

//void Song::writeFont(int level, Xml& xml, const char* name,
//   const QFont& font) const
//      {
//      xml.nput(level, "<%s family=\"%s\" size=\"%d\"",
//         name, Xml::xmlString(font.family()).toLatin1().constData(), font.pointSize());
//      if (font.weight() != QFont::Normal)
//            xml.nput(" weight=\"%d\"", font.weight());
//      if (font.italic())
//            xml.nput(" italic=\"1\"");
//      xml.nput(" />\n");
//      }

//---------------------------------------------------------
//   readFont
//---------------------------------------------------------

//QFont Song::readFont(Xml& xml, const char* name)
//      {
//      QFont f;
//      for (;;) {
//            Xml::Token token = xml.parse();
//            switch (token) {
//                  case Xml::Error:
//                  case Xml::End:
//                        return f;
//                  case Xml::TagStart:
//                        xml.unknown("readFont");
//                        break;
//                  case Xml::Attribut:
//                        if (xml.s1() == "family")
//                              f.setFamily(xml.s2());
//                        else if (xml.s1() == "size")
//                              f.setPointSize(xml.s2().toInt());
//                        else if (xml.s1() == "weight")
//                              f.setWeight(xml.s2().toInt());
//                        else if (xml.s1() == "italic")
//                              f.setItalic(xml.s2().toInt());
//                        break;
//                  case Xml::TagEnd:
//                        if (xml.s1() == name)
//                              return f;
//                  default:
//                        break;
//                  }
//            }
//      return f;
//      }

//---------------------------------------------------------
//   readMarker
//---------------------------------------------------------

void Song::readMarker(Xml& xml)
      {
      Marker m;
      m.read(xml);
      _markerList->add(m);
      }

//---------------------------------------------------------
//   checkSongSampleRate
//   Called by gui thread.
//---------------------------------------------------------

void Song::checkSongSampleRate()
{
  std::map<int, int> waveRates;
  
  for(ciWaveTrack iwt = waves()->begin(); iwt != waves()->end(); ++iwt)
  {
    WaveTrack* wt = *iwt;
    for(ciPart ipt = wt->parts()->begin(); ipt != wt->parts()->end(); ++ipt)
    {
      Part* pt = ipt->second;
      for(ciEvent ie = pt->events().begin(); ie != pt->events().end(); ++ie)
      {
        const Event e(ie->second);
        if(e.sndFile().isOpen())
        {
          const int sr = e.sndFile().samplerate();
          std::map<int, int>::iterator iwr = waveRates.find(sr);
          if(iwr == waveRates.end())
          {
            waveRates.insert(std::pair<int, int>(sr, 1));
          }
          else
          {
            ++iwr->second;
          }
        }
      }
    }
  }
  
  for(std::map<int, int>::const_iterator iwr = waveRates.cbegin(); iwr != waveRates.cend(); ++iwr)
  {
    
  }
}

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Song::read(Xml& xml, bool /*isTemplate*/)
      {
      MusEGlobal::cloneList.clear();
      for (;;) {
         if (MusEGlobal::muse->progress) {
            MusEGlobal::muse->progress->setValue(MusEGlobal::muse->progress->value()+1);
         }

            Xml::Token token;
            token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        goto song_read_end;
                  case Xml::TagStart:
                        if (tag == "master")
                        {
                              // Avoid emitting songChanged.
                              // Tick parameter is not used.
                              MusEGlobal::tempomap.setMasterFlag(0, xml.parseInt());
                        }
                        else if (tag == "info")
                              songInfoStr = xml.parse1();
                        else if (tag == "showinfo")
                              showSongInfo = xml.parseInt();
                        else if (tag == "loop")
                              setLoop(xml.parseInt());
                        else if (tag == "punchin")
                              setPunchin(xml.parseInt());
                        else if (tag == "punchout")
                              setPunchout(xml.parseInt());
                        else if (tag == "record")
                        // This doesn't work as there are no tracks yet at this point and this is checked
                        // in setRecord. So better make it clear and explicit.
                        // (Using the default autoRecEnable==true would seem wrong too at this point.)
                        // setRecord(xml.parseInt());
                              setRecord(false);
                        else if (tag == "solo")
                              soloFlag = xml.parseInt();
                        else if (tag == "type")          // Obsolete.
                              xml.parseInt();
                        else if (tag == "recmode")
                              _recMode  = xml.parseInt();
                        else if (tag == "cycle")
                              _cycleMode  = xml.parseInt();
                        else if (tag == "click")
                              setClick(xml.parseInt());
                        else if (tag == "quantize")
                              _quantize  = xml.parseInt();
                        else if (tag == "len")
                              _songLenTicks  = xml.parseInt();
                        else if (tag == "follow")
                              _follow  = FollowMode(xml.parseInt());
                        else if (tag == "midiDivision") {
                              // TODO: Compare with current global setting and convert the
                              //  song if required - similar to how the song vs. global
                              //  sample rate ratio is handled. Ignore for now.
                              xml.parseInt();
                            }
                        else if (tag == "sampleRate") {
                              // Ignore. Sample rate setting is handled by the
                              //  song discovery mechanism (in MusE::loadProjectFile1()).
                              xml.parseInt();
                            }
                        else if (tag == "tempolist") {
                              MusEGlobal::tempomap.read(xml);
                              }
                        else if (tag == "siglist")
                              MusEGlobal::sigmap.read(xml);
                        else if (tag == "keylist") {
                              MusEGlobal::keymap.read(xml);
                              }
                        else if (tag == "miditrack") {
                              MidiTrack* track = new MidiTrack();
                              track->read(xml);
                              insertTrack0(track, -1);
                              }
                        else if (tag == "drumtrack") { // Old drumtrack is obsolete.
                              MidiTrack* track = new MidiTrack();
                              track->setType(Track::DRUM);
                              track->read(xml);
                              track->convertToType(Track::DRUM); // Convert the notes and controllers.
                              insertTrack0(track, -1);
                              }
                        else if (tag == "newdrumtrack") {
                              MidiTrack* track = new MidiTrack();
                              track->setType(Track::DRUM);
                              track->read(xml);
                              insertTrack0(track, -1);
                              }
                        else if (tag == "wavetrack") {
                              MusECore::WaveTrack* track = new MusECore::WaveTrack();
                              track->read(xml);
                              insertTrack0(track,-1);
                              // Now that the track has been added to the lists in insertTrack2(),
                              //  OSC can find the track and its plugins, and start their native guis if required...
                              track->showPendingPluginNativeGuis();
                              }
                        else if (tag == "AudioInput") {
                              AudioInput* track = new AudioInput();
                              track->read(xml);
                              insertTrack0(track,-1);
                              track->showPendingPluginNativeGuis();
                              }
                        else if (tag == "AudioOutput") {
                              AudioOutput* track = new AudioOutput();
                              track->read(xml);
                              insertTrack0(track,-1);
                              track->showPendingPluginNativeGuis();
                              }
                        else if (tag == "AudioGroup") {
                              AudioGroup* track = new AudioGroup();
                              track->read(xml);
                              insertTrack0(track,-1);
                              track->showPendingPluginNativeGuis();
                              }
                        else if (tag == "AudioAux") {
                              AudioAux* track = new AudioAux();
                              track->read(xml);
                              insertTrack0(track,-1);
                              track->showPendingPluginNativeGuis();
                              }
                        else if (tag == "SynthI") {
                              SynthI* track = new SynthI();
                              track->read(xml);
                              // Done in SynthI::read()
                              // insertTrack(track,-1);
                              //track->showPendingPluginNativeGuis();
                              }
                        else if (tag == "Route") {
                              readRoute(xml);
                              }
                        else if (tag == "marker")
                              readMarker(xml);
                        else if (tag == "globalPitchShift")
                              _globalPitchShift = xml.parseInt();
                        // REMOVE Tim. automation. Removed.
                        // Deprecated. MusEGlobal::automation is now fixed TRUE
                        //  for now until we decide what to do with it.
                        else if (tag == "automation")
                        //      MusEGlobal::automation = xml.parseInt();
                              xml.parseInt();
                        else if (tag == "cpos") {
                              int pos = xml.parseInt();
                              Pos p(pos, true);
                              setPos(Song::CPOS, p, false, false, false);
                              }
                        else if (tag == "lpos") {
                              int pos = xml.parseInt();
                              Pos p(pos, true);
                              setPos(Song::LPOS, p, false, false, false);
                              }
                        else if (tag == "rpos") {
                              int pos = xml.parseInt();
                              Pos p(pos, true);
                              setPos(Song::RPOS, p, false, false, false);
                              }
                        else if (tag == "drummap")
                              readDrumMap(xml, false);
                        else if (tag == "drum_ordering")
                              MusEGlobal::global_drum_ordering.read(xml);
                        else
                              xml.unknown("Song");
                        break;
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
                        if (tag == "song") {
                              goto song_read_end;
                              }
                  default:
                        break;
                  }
            }
            
song_read_end:
      dirty = false;

      // Since cloneList is also used for copy/paste operations,
      //  clear the copy clone list again.
      MusEGlobal::cloneList.clear();
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Song::write(int level, Xml& xml) const
      {
      xml.tag(level++, "song");
      xml.strTag(level, "info", songInfoStr);
      xml.intTag(level, "showinfo", showSongInfo);
// REMOVE Tim. automation. Removed.
// Deprecated. MusEGlobal::automation is now fixed TRUE
//   for now until we decide what to do with it.
//       xml.intTag(level, "automation", MusEGlobal::automation);
      xml.intTag(level, "cpos", MusEGlobal::song->cpos());
      xml.intTag(level, "rpos", MusEGlobal::song->rpos());
      xml.intTag(level, "lpos", MusEGlobal::song->lpos());
      xml.intTag(level, "master", MusEGlobal::tempomap.masterFlag());
      xml.intTag(level, "loop", loopFlag);
      xml.intTag(level, "punchin", punchinFlag);
      xml.intTag(level, "punchout", punchoutFlag);
      xml.intTag(level, "record", recordFlag);
      xml.intTag(level, "solo", soloFlag);
      xml.intTag(level, "recmode", _recMode);
      xml.intTag(level, "cycle", _cycleMode);
      xml.intTag(level, "click", _click);
      xml.intTag(level, "quantize", _quantize);
      xml.intTag(level, "len", _songLenTicks);
      xml.intTag(level, "follow", _follow);
      // Save the current global midi division as well as current sample rate
      //  so the song can be scaled properly if the values differ on reload.
      xml.intTag(level, "midiDivision", MusEGlobal::config.division);
      xml.intTag(level, "sampleRate", MusEGlobal::sampleRate);
      if (_globalPitchShift)
            xml.intTag(level, "globalPitchShift", _globalPitchShift);

      // Make a backup of the current clone list, to retain any 'copy' items,
      //  so that pasting works properly after.
      CloneList copyCloneList = MusEGlobal::cloneList;
      MusEGlobal::cloneList.clear();

      // write tracks
      for (ciTrack i = _tracks.begin(); i != _tracks.end(); ++i)
            (*i)->write(level, xml);

      // write routing
      for (ciTrack i = _tracks.begin(); i != _tracks.end(); ++i)
            (*i)->writeRouting(level, xml);

      // Write midi device routing.
      for (iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i)
            (*i)->writeRouting(level, xml);

      // Write midi port routing.
      for (int i = 0; i < MusECore::MIDI_PORTS; ++i)
            MusEGlobal::midiPorts[i].writeRouting(level, xml);

      MusEGlobal::tempomap.write(level, xml);
      MusEGlobal::sigmap.write(level, xml);
      MusEGlobal::keymap.write(level, xml);
      _markerList->write(level, xml);

      writeDrumMap(level, xml, false);
      MusEGlobal::global_drum_ordering.write(level, xml);
      xml.tag(level, "/song");

      // Restore backup of the clone list, to retain any 'copy' items,
      //  so that pasting works properly after.
      MusEGlobal::cloneList.clear();
      MusEGlobal::cloneList = copyCloneList;
      }

//--------------------------------
//   resolveInstrumentReferences
//--------------------------------

static void resolveInstrumentReferences()
{
  for(int i = 0; i < MIDI_PORTS; ++i)
  {
    MidiPort* mp = &MusEGlobal::midiPorts[i];
    const QString& name = mp->tmpInstrRef();
    const int idx = mp->tmpTrackRef();
    if(idx >= 0)
    {
      Track* track = MusEGlobal::song->tracks()->index(idx);
      if(track && track->isSynthTrack())
      {
        SynthI* si = static_cast<SynthI*>(track);
        mp->changeInstrument(si);
      }
    }
    else if(!name.isEmpty())
    {
      mp->changeInstrument(registerMidiInstrument(name));
    }

    // Done with temporary file references. Clear them.
    mp->clearTmpFileRefs();
  }
}

//--------------------------------
//   resolveStripReferences
//--------------------------------

static void resolveStripReferences(MusEGlobal::MixerConfig* mconfig)
{
  MusECore::TrackList* tl = MusEGlobal::song->tracks();
  MusECore::Track* track;
  MusEGlobal::StripConfigList_t& scl = mconfig->stripConfigList;
  if(!scl.empty())
  {
    for(MusEGlobal::iStripConfigList isc = scl.begin(); isc != scl.end(); )
    {
      MusEGlobal::StripConfig& sc = *isc;

      // Is it a dud? Delete it.
      if(sc.isNull() && sc._tmpFileIdx < 0)
      {
        isc = scl.erase(isc);
        continue;
      }

      // Does it have a temporary track index?
      if(sc._tmpFileIdx >= 0)
      {
        // Find an existing track at that index.
        track = tl->index(sc._tmpFileIdx);
        // No corresponding track found? Delete the strip config.
        if(!track)
        {
          isc = scl.erase(isc);
          continue;
        }
        // Link the strip config to the track, via serial number.
        sc._serial = track->serial();
        // Done with temporary index. Reset it.
        sc._tmpFileIdx = -1;
      }

      // As for other configs with a serial and no _tmpFileIdx,
      //  leave them alone. They may already be valid. If not,
      //  they may need to persist for the undo system to restore from.
      // Ultimately when saving, the strip config write() function removes duds anyway.

      ++isc;
    }
  }
}

//--------------------------------
//   resolveSongfileReferences
//--------------------------------

void Song::resolveSongfileReferences()
{
  //-----------------------------------------------
  // Resolve instrument references:
  //-----------------------------------------------
  resolveInstrumentReferences();

  //-----------------------------------------------
  // Resolve mixer strip configuration references:
  //-----------------------------------------------
  resolveStripReferences(&MusEGlobal::config.mixer1);
  resolveStripReferences(&MusEGlobal::config.mixer2);
}

// REMOVE Tim. samplerate. Added. TODO
#if 0
//---------------------------------------------------------
//   convertProjectSampleRate
//---------------------------------------------------------

void Song::convertProjectSampleRate(int newRate)
{
  double sratio = double(newRate) / double(MusEGlobal::sampleRate);
  
  for(iTrack i = _tracks.begin(); i != _tracks.end(); ++i)
  {
    Track* track = *i;
    //if(track->isMidiTrack)
    //  continue;
    
    PartList* parts = track->parts();
    
    for(iPart ip = parts->begin(); i != parts->end(); ++i)
    {
      Part* part = *ip;
      
      EventList& el = part->nonconst_events();
      
      for(iEvent ie = el.begin(); ie != el.end(); ++ie)
      {
        Event& e = *ie;
      }
      
      if(part->type() == Part::FRAMES)
      {
        part->setFrame(double(part->frame()) * sratio);
        part->setLenFrame(double(part->lenFrame()) * sratio);
      }
    }
    
    if(track->type == Track::WAVE)
    {
      
    }

    
  }
}
#endif 
      
} // namespace MusECore

namespace MusEGui {

//---------------------------------------------------------
//   readPart
//---------------------------------------------------------

MusECore::Part* MusE::readPart(MusECore::Xml& xml)
      {
      MusECore::Part* part = nullptr;
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return part;
                  case MusECore::Xml::Text:
                        {
                        int trackIdx, partIdx;
                        sscanf(tag.toLatin1().constData(), "%d:%d", &trackIdx, &partIdx);
                        MusECore::Track* track = nullptr;
                        //check if track index is in bounds before getting it (danvd)
                        if(trackIdx < (int)MusEGlobal::song->tracks()->size())
                        {
                            track = MusEGlobal::song->tracks()->index(trackIdx);
                        }
                        if (track)
                              part = track->parts()->find(partIdx);
                        }
                        break;
                  case MusECore::Xml::TagStart:
                        xml.unknown("readPart");
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "part")
                              return part;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   readToplevels
//---------------------------------------------------------

void MusE::readToplevels(MusECore::Xml& xml)
{
    MusECore::PartList* pl = new MusECore::PartList;
    for (;;) {
        MusECore::Xml::Token token = xml.parse();
        const QString& tag = xml.s1();
        switch (token) {
        case MusECore::Xml::Error:
        case MusECore::Xml::End:
            delete pl;
            return;
        case MusECore::Xml::TagStart:
            if (tag == "part") {
                MusECore::Part* part = readPart(xml);
                if (part)
                    pl->add(part);
            }
            else if (tag == "pianoroll") {
                // p3.3.34
                // Do not open if there are no parts.
                // Had bogus '-1' part index for list edit in med file,
                //  causing list edit to segfault on song load.
                // Somehow that -1 was put there on write, because the
                //  current part didn't exist anymore, so no index number
                //  could be found for it on write. Watching... may be fixed.
                // But for now be safe for all the top levels...
                if(!pl->empty())
                {
                    startPianoroll(pl);
                    toplevels.back()->readStatus(xml);
                    pl = new MusECore::PartList;
                }
            }
            else if (tag == "scoreedit") {
                MusEGui::ScoreEdit* score = new MusEGui::ScoreEdit(this, 0, _arranger->cursorValue());
                toplevels.push_back(score);
                connect(score, SIGNAL(isDeleting(MusEGui::TopWin*)), SLOT(toplevelDeleting(MusEGui::TopWin*)));
                connect(score, SIGNAL(name_changed()), arrangerView, SLOT(scoreNamingChanged()));
                score->show();
                score->readStatus(xml);
            }
            else if (tag == "drumedit") {
                if(!pl->empty())
                {
                    startDrumEditor(pl);
                    toplevels.back()->readStatus(xml);
                    pl = new MusECore::PartList;
                }
            }
            else if (tag == "master") {
                startMasterEditor();
                toplevels.back()->readStatus(xml);
            }
            else if (tag == "arrangerview") {
                TopWin* tw = toplevels.findType(TopWin::ARRANGER);
                tw->readStatus(xml);
                tw->showMaximized();
            }
            else if (tag == "waveedit") {
                if(!pl->empty())
                {
                    startWaveEditor(pl);
                    toplevels.back()->readStatus(xml);
                    pl = new MusECore::PartList;
                }
            }
            else
                xml.unknown("MusE");
            break;
        case MusECore::Xml::Attribut:
            break;
        case MusECore::Xml::TagEnd:
            if (tag == "toplevels") {
                delete pl;
                return;
            }
        default:
            break;
        }
    }
}

//---------------------------------------------------------
//   read
//    read song
//---------------------------------------------------------

void MusE::read(MusECore::Xml& xml, bool doReadMidiPorts, bool isTemplate)
      {
      bool skipmode = true;

      writeTopwinState=true;

      for (;;) {
            if (progress)
                progress->setValue(progress->value()+1);
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (skipmode && tag == "muse")
                              skipmode = false;
                        else if (skipmode)
                              break;
                        else if (tag == "configuration")
                              readConfiguration(xml, doReadMidiPorts, false /* do NOT read global settings, see below */);
                        /* Explanation for why "do NOT read global settings":
                         * if you would use true here, then muse would overwrite certain global config stuff
                         * by the settings stored in the song. but you don't want this. imagine that you
                         * send a friend a .med file. your friend opens it and baaam, his configuration is
                         * garbled. why? well, because these readConfigurations here would have overwritten
                         * parts (but not all) of his global config (like MDI/SDI, toolbar states etc.)
                         * with the data stored in the song. (it IS stored there. dunny why, i find it pretty
                         * senseless.)
                         *
                         * If you've a problem which seems to be solved by replacing "false" with "true", i've
                         * a better solution for you: go into conf.cpp, in void readConfiguration(Xml& xml, bool readOnlySequencer, bool doReadGlobalConfig)
                         * (around line 525), look for a comment like this:
                         * "Global and/or per-song config stuff ends here" (alternatively just search for
                         * "----"). Your problem is probably that some non-global setting should be loaded but
                         * is not. Fix it by either placing the else if (foo)... clause responsible for that
                         * setting to be loaded into the first part, that is, before "else if (!doReadGlobalConfig)"
                         * or (if the settings actually IS global and not per-song), ensure that the routine
                         * which writes the global (and not the song-)configuration really writes that setting.
                         * (it might happen that it formerly worked because it was written to the song instead
                         *  of the global config by mistake, and now isn't loaded anymore. write it to the
                         *  correct location.)
                         *
                         *                                                                                -- flo93
                         */
                        else if (tag == "song")
                        {
                              MusEGlobal::song->read(xml, isTemplate);

                              // Now that the song file has been fully loaded, resolve any references in the file.
                              MusEGlobal::song->resolveSongfileReferences();

                              // Now that all track and instrument references have been resolved,
                              //  it is safe to add all the midi controller cache values.
                              MusEGlobal::song->changeMidiCtrlCacheEvents(true, true, true, true, true);

                              MusEGlobal::audio->msgUpdateSoloStates();
                              // Inform the rest of the app that the song (may) have changed, using these flags.
                              // After this function is called, the caller can do a general Song::update() MINUS these flags,
                              //  like in MusE::loadProjectFile1() - the only place calling so far, as of this writing.
                              // Some existing windows need this, like arranger, some don't which are dynamically created after this.
                              MusEGlobal::song->update(SC_TRACK_INSERTED);
                        }
                        else if (tag == "toplevels")
                              readToplevels(xml);
                        else if (tag == "no_toplevels")
                        {
                              if (!isTemplate)
                                writeTopwinState=false;

                              xml.skip("no_toplevels");
                        }

                        else
                              xml.unknown("muse");
                        break;
                  case MusECore::Xml::Attribut:
                        if (tag == "version") {
                              int major = xml.s2().section('.', 0, 0).toInt();
                              int minor = xml.s2().section('.', 1, 1).toInt();
                              xml.setVersion(major, minor);
                              }
                        break;
                  case MusECore::Xml::TagEnd:
                        if(!xml.isVersionEqualToLatest())
                        {
                          fprintf(stderr, "\n***WARNING***\nLoaded file version is %d.%d\nCurrent version is %d.%d\n"
                                  "Conversions may be applied if file is saved!\n\n",
                                  xml.majorVersion(), xml.minorVersion(),
                                  xml.latestMajorVersion(), xml.latestMinorVersion());
                          // Cannot construct QWidgets until QApplication created!
                          // Check MusEGlobal::muse which is created shortly after the application...
                          if(MusEGlobal::muse && MusEGlobal::config.warnOnFileVersions)
                          {
                            QString txt = tr("File version is %1.%2\nCurrent version is %3.%4\n"
                                             "Conversions may be applied if file is saved!")
                                            .arg(xml.majorVersion()).arg(xml.minorVersion())
                                            .arg(xml.latestMajorVersion()).arg(xml.latestMinorVersion());
                            QMessageBox* mb = new QMessageBox(QMessageBox::Warning,
                                                              tr("Opening file"),
                                                              txt,
                                                              QMessageBox::Ok, MusEGlobal::muse);
                            QCheckBox* cb = new QCheckBox(tr("Do not warn again"));
                            cb->setChecked(!MusEGlobal::config.warnOnFileVersions);
                            mb->setCheckBox(cb);
                            mb->exec();
                            if(!mb->checkBox()->isChecked() != MusEGlobal::config.warnOnFileVersions)
                            {
                              MusEGlobal::config.warnOnFileVersions = !mb->checkBox()->isChecked();
                              // Save settings. Use simple version - do NOT set style or stylesheet, this has nothing to do with that.
                              //MusEGlobal::muse->changeConfig(true);  // Save settings? No, wait till close.
                            }
                            delete mb;
                          }
                        }
                        if (!skipmode && tag == "muse")
                              return;
                  default:
                        break;
                  }
            }
      }


//---------------------------------------------------------
//   write
//    write song
//---------------------------------------------------------

void MusE::write(MusECore::Xml& xml, bool writeTopwins) const
{
    xml.header();

    int level = 0;
    xml.nput(level++, "<muse version=\"%d.%d\">\n", xml.latestMajorVersion(), xml.latestMinorVersion());

    writeConfiguration(level, xml);

    writeStatusMidiInputTransformPlugins(level, xml);

    MusEGlobal::song->write(level, xml);

    if (writeTopwins && !toplevels.empty()) {
        xml.tag(level++, "toplevels");
        for (const auto& i : toplevels) {
            if (i->isVisible())
                i->writeStatus(level, xml);
        }
        xml.tag(level--, "/toplevels");
    }
    else if (!writeTopwins)
    {
        xml.tag(level, "no_toplevels");
        xml.etag(level, "no_toplevels");
    }

    xml.tag(level, "/muse");
}

} // namespace MusEGui

