//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: functions.h,v 1.20.2.19 2011/05/05 20:10 flo93 Exp $
//  (C) Copyright 2011 Florian Jung (flo93@sourceforge.net)
//=========================================================

#ifndef __FUNCTIONS_H__
#define __FUNCTIONS_H__

#include "widgets/function_dialogs/velocity.h"
#include "widgets/function_dialogs/quantize.h"
#include "widgets/function_dialogs/crescendo.h"
#include "widgets/function_dialogs/gatetime.h"
#include "widgets/function_dialogs/remove.h"
#include "widgets/function_dialogs/transpose.h"
#include "widgets/function_dialogs/setlen.h"
#include "widgets/function_dialogs/move.h"
#include "widgets/function_dialogs/deloverlaps.h"
#include "widgets/function_dialogs/legato.h"

#include <set>
#include "part.h"

class QString;
class QMimeData;

extern GateTime* gatetime_dialog;
extern Velocity* velocity_dialog;
extern Quantize* quantize_dialog;
extern Remove* erase_dialog;
extern DelOverlaps* del_overlaps_dialog;
extern Setlen* set_notelen_dialog;
extern Move* move_notes_dialog;
extern Transpose* transpose_dialog;
extern Crescendo* crescendo_dialog;
extern Legato* legato_dialog;

void init_function_dialogs(QWidget* parent);


std::set<Part*> partlist_to_set(PartList* pl);
std::map<Event*, Part*> get_events(const std::set<Part*>& parts, int range);

//these functions simply do their job, non-interactively
void modify_velocity(const std::set<Part*>& parts, int range, int rate, int offset=0);
void modify_off_velocity(const std::set<Part*>& parts, int range, int rate, int offset=0);
void modify_notelen(const std::set<Part*>& parts, int range, int rate, int offset=0);
void quantize_notes(const std::set<Part*>& parts, int range, int raster, bool len=false, int strength=100, int swing=0, int threshold=0);
void erase_notes(const std::set<Part*>& parts, int range, int velo_threshold=0, bool velo_thres_used=false, int len_threshold=0, bool len_thres_used=false);
void delete_overlaps(const std::set<Part*>& parts, int range);
void set_notelen(const std::set<Part*>& parts, int range, int len);
void move_notes(const std::set<Part*>& parts, int range, signed int ticks);
void transpose_notes(const std::set<Part*>& parts, int range, signed int halftonesteps);
void crescendo(const std::set<Part*>& parts, int range, int start_val, int end_val, bool absolute);
void legato(const std::set<Part*>& parts, int range, int min_len=1, bool dont_shorten=false);


//the below functions automatically open the dialog
//they return true if you click "ok" and false if "abort"
bool modify_velocity(const std::set<Part*>& parts);
bool modify_notelen(const std::set<Part*>& parts);
bool quantize_notes(const std::set<Part*>& parts);
bool set_notelen(const std::set<Part*>& parts);
bool move_notes(const std::set<Part*>& parts);
bool transpose_notes(const std::set<Part*>& parts);
bool crescendo(const std::set<Part*>& parts);
bool erase_notes(const std::set<Part*>& parts);
bool delete_overlaps(const std::set<Part*>& parts);
bool legato(const std::set<Part*>& parts);


//functions for copy'n'paste
void copy_notes(const std::set<Part*>& parts, int range);
void paste_notes(Part* dest_part);
QMimeData* selected_events_to_mime(const std::set<Part*>& parts, int range);
void paste_at(Part* dest_part, const QString& pt, int pos);

//functions for selections
void select_all(const std::set<Part*>& parts);
void select_none(const std::set<Part*>& parts);
void select_invert(const std::set<Part*>& parts);
void select_in_loop(const std::set<Part*>& parts);
void select_not_in_loop(const std::set<Part*>& parts);

//functions for reading and writing default values
class Xml;
void read_function_dialog_config(Xml& xml);
void write_function_dialog_config(int level, Xml& xml);

#endif