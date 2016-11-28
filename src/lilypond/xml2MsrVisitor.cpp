/*
  MusicXML Library
  Copyright (C) Grame 2006-2013

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Grame Research Laboratory, 11, cours de Verdun Gensoul 69002 Lyon - France
  research@grame.fr
*/

#ifdef VC6
# pragma warning (disable : 4786)
#endif

#include <sstream>
#include <stdlib.h>     /* abort, NULL */
#include <climits>      /* INT_MIN */
#include <algorithm>    /* for_each */

#include "xml_tree_browser.h"

#include "conversions.h"

#include "xml2MsrVisitor.h"


using namespace std;

namespace MusicXML2
{

//________________________________________________________________________
/* JMI
void xml2MsrVisitor::msrMusicXMLWarning (
  string message)
{
  musicXMLWarning (
    gCurrentMusicXMLLocation,
    message);
}

void xml2MsrVisitor::msrMusicXMLError (
  string message)
{
  musicXMLError (
    gCurrentMusicXMLLocation,
    message);
}

void xml2MsrVisitor::msrInternalError (
  string message)
{
  internalError (
    gCurrentMusicXMLLocation,
    message);
}
*/

//________________________________________________________________________
xml2MsrVisitor::xml2MsrVisitor (
  S_msrOptions& msrOpts)
  /* JMI
    : gCurrentMusicXMLLocation (
      0, // before first line on MusicXML contents
      0, // in case of an anacrusis
      0) // at the beginning of a measure
      */
{
  fMsrOptions = msrOpts;

  /* JMI
    : gCurrentMusicXMLLoclation (
      0, // before first line on MusicXML contents
      0, // in case of an anacrusis
      0) // at the beginning of a measure
      */
  gCurrentMusicXMLLocation.fInputLineNumber = 0;
  gCurrentMusicXMLLocation.fMeasureNumber = 0; // in case of an anacrusis
  gCurrentMusicXMLLocation.fPositionInMeasure = 1;

  fMillimeters       = -1;
  fTenths            = -1;
  fOnGoingPageLayout = false;

  fCurrentMusicXMLDivisions = 0;

  // create the MSR score
  fMsrScore =
    msrScore::create (fMsrOptions, 0);

  fCurrentTimeStaffNumber = 1; // it may be absent
  
  fCurrentLyricsNumber = -1; // JMI
  fCurrentLyricschunkType = msrLyricschunk::k_NoChunk;

  fOnGoingChord = false;
  
  fOnGoingTuplet = false;

  fCurrentBackupDuration = -1;

  fOnGoingNote = false;

  fOnGoingChord = false;
  
  fOnGoingSlur = false;

  fOnGoingDirection = false;

  fOnGoingRepeat = false;
  
  fOnGoingBackup  = false;
  fOnGoingForward = false;
}

xml2MsrVisitor::~xml2MsrVisitor ()
{}

//________________________________________________________________________
S_msrScore
xml2MsrVisitor::buildMsrScoreFromXMLElementTree (
  const Sxmlelement& xmlTree)
{
  S_msrScore result;
  
  if (xmlTree) {
    // create a tree browser on this visitor
    tree_browser<xmlelement> browser (this);
    
    // browse the xmlelement tree
    browser.browse (*xmlTree);
    
    // grab the resulting Score
    result = fMsrScore;
  }

  return result;
}

//________________________________________________________________________
S_msrPartgroup xml2MsrVisitor::createImplicitMSRPartgroup (
  int inputLineNumber)
{
  /*
  A first part group is created with all the needed contents
  if none is specified in the MusicXML data.
  */

  // create an implicit part group
  fCurrentPartgroupNumber = 1;
  
  if (fMsrOptions->fTrace)
    cerr << idtr <<
      "Creating an implicit part group with number " <<
      fCurrentPartgroupNumber << endl;

  S_msrPartgroup
    partgroup =
      msrPartgroup::create (
        fMsrOptions,
        inputLineNumber,
        fCurrentPartgroupNumber,
        "Implicit",
        "Impl.",
        msrPartgroup::kBracketPartgroupSymbol,
        0,
        true);

  /*
    this implicit part group will be added to the MSR score
    in method visitEnd (S_part_list& elt)
  */
  
  // add implicit part group to the map of this visitor
  if (fMsrOptions->fTrace)
    cerr << idtr <<
      "Adding implicit part group " << fCurrentPartgroupNumber <<
      " to visitor's data" << endl;
  fPartgroupsMap [fCurrentPartgroupNumber] = partgroup;
  fPartgroupsList.push_front (partgroup);

  fImplicitPartgroup = partgroup;
  
  return partgroup;
} // xml2MsrVisitor::createImplicitMSRPartgroup ()

//______________________________________________________________________________
void xml2MsrVisitor::visitStart ( S_work_number& elt )
{
  fMsrScore->getIdentification () ->
    setWorkNumber (
      elt->getInputLineNumber (),
      elt->getValue ());
}

void xml2MsrVisitor::visitStart ( S_work_title& elt )
{
  fMsrScore->getIdentification () ->
    setWorkTitle (
      elt->getInputLineNumber (),
      elt->getValue ());
}
  
void xml2MsrVisitor::visitStart ( S_movement_number& elt )
{
  fMsrScore->getIdentification () ->
    setMovementNumber (
      elt->getInputLineNumber (),
      elt->getValue ());
}

void xml2MsrVisitor::visitStart ( S_movement_title& elt )
{
  fMsrScore->getIdentification () ->
    setMovementTitle (
      elt->getInputLineNumber (),
      elt->getValue ());
}

void xml2MsrVisitor::visitStart ( S_creator& elt )
{
  fMsrScore->getIdentification () ->
    addCreator (
      elt->getInputLineNumber (),
      elt->getAttributeValue ("type"),
      elt->getValue ());
}

void xml2MsrVisitor::visitStart ( S_rights& elt )
{
  fMsrScore->getIdentification () ->
    setRights (
      elt->getInputLineNumber (),
      elt->getValue ());
}

void xml2MsrVisitor::visitStart ( S_software& elt )
{
  fMsrScore->getIdentification () ->
    addSoftware (
      elt->getInputLineNumber (),
      elt->getValue ());
}

void xml2MsrVisitor::visitStart ( S_encoding_date& elt )
{
  fMsrScore->getIdentification () ->
    setEncodingDate (
      elt->getInputLineNumber (),
      elt->getValue ());
}

//______________________________________________________________________________
void xml2MsrVisitor::visitStart ( S_millimeters& elt )
{ 
  fMillimeters = (int)(*elt);
  
  fMsrScore->getPageGeometry ()->
    setMillimeters (fMillimeters);
}

void xml2MsrVisitor::visitStart ( S_tenths& elt )
{
  fTenths = (int)(*elt);

  fMsrScore->getPageGeometry ()->
    setTenths (fTenths);
}

void xml2MsrVisitor::visitEnd ( S_scaling& elt)
{
  if (fMsrOptions->fTrace)
    cerr <<
      "There are " << fTenths <<
      " tenths for " <<  fMillimeters <<
      " millimeters, hence the global staff size is " <<
      fMsrScore->getPageGeometry ()->globalStaffSize () <<
      endl;
}

//______________________________________________________________________________
void xml2MsrVisitor::visitStart ( S_system_distance& elt )
{
  int systemDistance = (int)(*elt);
  
//  cout << "--> systemDistance = " << systemDistance << endl;
  fMsrScore->getPageGeometry ()->
    setBetweenSystemSpace (
      1.0 * systemDistance * fMillimeters / fTenths / 10);  
}

void xml2MsrVisitor::visitStart ( S_top_system_distance& elt )
{
  int topSystemDistance = (int)(*elt);
  
//  cout << "--> fTopSystemDistance = " << topSystemDistance << endl;
    fMsrScore->getPageGeometry ()->
    setPageTopSpace (
      1.0 * topSystemDistance * fMillimeters / fTenths / 10);  
}

//______________________________________________________________________________
void xml2MsrVisitor::visitStart ( S_page_layout& elt )
{
  fOnGoingPageLayout = true;
}
void xml2MsrVisitor::visitEnd ( S_page_layout& elt )
{
  fOnGoingPageLayout = false;
}

void xml2MsrVisitor::visitStart ( S_page_height& elt )
{
  if (fOnGoingPageLayout) {
    int pageHeight = (int)(*elt);
    
    //cout << "--> pageHeight = " << pageHeight << endl;
    fMsrScore->getPageGeometry ()->
      setPaperHeight (
        1.0 * pageHeight * fMillimeters / fTenths / 10);  
  }
}

void xml2MsrVisitor::visitStart ( S_page_width& elt )
{
  if (fOnGoingPageLayout) {
    int pageWidth = (int)(*elt);
    
    //cout << "--> pageWidth = " << pageWidth << endl;
    fMsrScore->getPageGeometry ()->
      setPaperWidth (
        1.0 * pageWidth * fMillimeters / fTenths / 10);  
  }
}

void xml2MsrVisitor::visitStart ( S_left_margin& elt )
{
  if (fOnGoingPageLayout) {
    int leftMargin = (int)(*elt);
    
    //cout << "--> leftMargin = " << leftMargin << endl;
    fMsrScore->getPageGeometry ()->
      setLeftMargin (
        1.0 * leftMargin * fMillimeters / fTenths / 10);  
  }
}

void xml2MsrVisitor::visitStart ( S_right_margin& elt )
{
  if (fOnGoingPageLayout) {
    int rightMargin = (int)(*elt);
    
    //cout << "--> rightMargin = " << rightMargin << endl;
    fMsrScore->getPageGeometry ()->
      setRightMargin (
        1.0 * rightMargin * fMillimeters / fTenths / 10);  
  }
}

void xml2MsrVisitor::visitStart ( S_top_margin& elt )
{
  if (fOnGoingPageLayout) {
    int topMargin = (int)(*elt);
    
    //cout << "--> topMargin = " << topMargin << endl;
    fMsrScore->getPageGeometry ()->
      setTopMargin (
        1.0 * topMargin * fMillimeters / fTenths / 10);  
  }
}

void xml2MsrVisitor::visitStart ( S_bottom_margin& elt )
{
  if (fOnGoingPageLayout) {
    int bottomMargin = (int)(*elt);
    
    //cout << "--> bottomMargin = " << bottomMargin << endl;
    fMsrScore->getPageGeometry ()->
      setBottomMargin (
        1.0 * bottomMargin * fMillimeters / fTenths / 10);  
  }
}

//________________________________________________________________________
/* JMI
void xml2MsrVisitor::visitStart ( S_instrument_name& el
{
  fCurrentInstrumentName = elt->getValue();
}
*/

//________________________________________________________________________
void xml2MsrVisitor::visitStart (S_part_list& elt)
{
  if (fMsrOptions->fTrace)
    cerr << idtr <<
      "Analysing part list" << endl;

  idtr++;
}
/*
  <part-list>
    <part-group number="1" type="start">
      <group-symbol default-x="-7">bracket</group-symbol>
      <group-barline>yes</group-barline>
    </part-group>
    <score-part id="P1">
      <part-name>Piccolo</part-name>
      <part-abbreviation>Picc.</part-abbreviation>
      <score-instrument id="P1-I18">
        <instrument-name>Picc.</instrument-name>
      </score-instrument>
      <midi-instrument id="P1-I18">
        <midi-channel>1</midi-channel>
        <midi-program>73</midi-program>
      </midi-instrument>
    </score-part>
    <part-group number="2" type="start">
      <group-name>1
2</group-name>
      <group-barline>yes</group-barline>
    </part-group>
    <score-part id="P2">
*/

void xml2MsrVisitor::visitEnd (S_part_list& elt)
{
  idtr--;

  if (fImplicitPartgroup) {
    // force an implicit part group "stop" on it
    // fCurrentPartgroupNumber holds the value 1
    handlePartgroupStop (elt->getInputLineNumber ());
    
    fImplicitPartgroup = 0; // NULL contents
  }
    
//  fMsrOptions->fDebug = false; // TEMP JMI
}

//________________________________________________________________________
/*
  There is no hierarchy implied in part-group elements.
  All that matters is the sequence of part-group elements relative to score-part elements.
  The sequencing of two consecutive part-group elements does not matter.
  It is the default-x attribute that indicates the left-to-right ordering of the group symbols.

  <part-group number="1" type="start">
  <group-name>Trombones</group-name>
  <group-abbreviation>Trb.</group-abbreviation>
  <group-symbol default-x="-12">brace</group-symbol>
  <group-barline>yes</group-barline>
  </part-group>
*/

S_msrPartgroup xml2MsrVisitor::fetchPartgroupInThisVisitor (
  int partgroupNumber)
{
  S_msrPartgroup result;
  
  if (fPartgroupsMap.count (partgroupNumber)) {
    result = fPartgroupsMap [partgroupNumber];
  }

  return result;
}

void xml2MsrVisitor::visitStart (S_part_group& elt)
{  
  fCurrentPartgroupNumber =
    elt->getAttributeIntValue ("number", 0);
    
  fCurrentPartgroupType =
    elt->getAttributeValue ("type");

  fCurrentPartgroupName = "";
  fCurrentPartgroupAbbreviation = "";
  fCurrentPartgroupSymbol = "";
  fCurrentPartgroupSymbolDefaultX = INT_MIN;
  fCurrentPartgroupBarline = "yes";
}

void xml2MsrVisitor::visitStart (S_group_name& elt)
{
  fCurrentPartgroupName = elt->getValue();
}

void xml2MsrVisitor::visitStart (S_group_abbreviation& elt)
{
  fCurrentPartgroupAbbreviation = elt->getValue ();
}

void xml2MsrVisitor::visitStart (S_group_symbol& elt)
{
  fCurrentPartgroupSymbol = elt->getValue ();

  fCurrentPartgroupSymbolDefaultX =
    elt->getAttributeIntValue ("default-x", 0);
}

void xml2MsrVisitor::visitStart ( S_group_barline& elt)
{
  fCurrentPartgroupBarline = elt->getValue ();
}

void xml2MsrVisitor::showPartgroupsData (string context)
{    
//  if (true || fMsrOptions->fDebug) {
  if (fMsrOptions->fDebugDebug) {
    cerr << idtr <<
      "==> " << context << ": fPartgroupsMap contains:" << endl;
    if (fPartgroupsMap.size()) {
      map<int, S_msrPartgroup>::const_iterator
        iBegin = fPartgroupsMap.begin(),
        iEnd   = fPartgroupsMap.end(),
        i      = iBegin;
        
      idtr++;
      for ( ; ; ) {
        cerr << idtr <<
          "\"" << (*i).first << "\" ----> " << (*i).second;
        if (++i == iEnd) break;
        cerr << endl;
      } // for
      idtr--;
    }
    cerr << idtr << "<== fPartgroupsMap" << endl;
  }

 // if (true || fMsrOptions->fDebug) {
  if (fMsrOptions->fDebugDebug) {
    cerr << idtr <<
      "==> " << context << ": fPartgroupsList contains:" << endl;
    if (fPartgroupsList.size()) {
      list<S_msrPartgroup>::const_iterator
        iBegin = fPartgroupsList.begin(),
        iEnd   = fPartgroupsList.end(),
        i      = iBegin;
        
      idtr++;
      for ( ; ; ) {
        cerr << idtr << (*i);
        if (++i == iEnd) break;
        cerr << endl;
      } // for
      idtr--;
    }
    cerr << idtr << "<== fPartgroupsList" << endl;
  }
}

void xml2MsrVisitor::handlePartgroupStart (
  int
      inputLineNumber,
  msrPartgroup::msrPartgroupSymbolKind
      partgroupSymbol,
  bool
      partgroupBarline)
{
  showPartgroupsData ("BEFORE START");

  // fetch part group to be started
  S_msrPartgroup
    partgroupToBeStarted =
      fetchPartgroupInThisVisitor (fCurrentPartgroupNumber);

  if (! partgroupToBeStarted) {
    // no, create it
    partgroupToBeStarted =
      msrPartgroup::create (
        fMsrOptions,
        inputLineNumber,
        fCurrentPartgroupNumber,
        fCurrentPartgroupName,
        fCurrentPartgroupAbbreviation,
        partgroupSymbol,
        fCurrentPartgroupSymbolDefaultX,
        partgroupBarline);

    // add it to the part group map of this visitor
    if (fMsrOptions->fTrace)
      cerr << idtr <<
        "Adding part group " << fCurrentPartgroupNumber <<
        " to visitor's part group map" << endl;
    fPartgroupsMap [fCurrentPartgroupNumber] =
      partgroupToBeStarted;
  }
  
  // add it to the part group list of this visitor
  if (fMsrOptions->fTrace)
    cerr << idtr <<
      "Adding part group " << fCurrentPartgroupNumber <<
      " to visitor's part groups list" << endl;

  if (! fPartgroupsList.size())
    // insert first part group ahead of the list
    fPartgroupsList.push_front (partgroupToBeStarted);
  else {
    // place in the part groups list so as to
    // have them ordered by increasing order
    // (all of them they are negative)
    list<S_msrPartgroup>::iterator
      iBegin = fPartgroupsList.begin(),
      iEnd   = fPartgroupsList.end(),
      i      = iBegin;

    while (true) {
      if (i == iEnd) {
        fPartgroupsList.push_back (partgroupToBeStarted);
        break;
      }

      // CAUTION: insert() inserts before the position
      // indicated by its iterator argument
      if (
          fCurrentPartgroupSymbolDefaultX
            <
          (*i)->getPartgroupSymbolDefaultX ()) {
        fPartgroupsList.insert (i, partgroupToBeStarted);
        break;
      }
      
      i++;
    } // while
  }
  
  showPartgroupsData ("AFTER START");
}
  
void xml2MsrVisitor::handlePartgroupStop (int inputLineNumber)
{
  showPartgroupsData ("BEFORE STOP");

  // is the part group to be stopped known?
  S_msrPartgroup
    partgroupToBeStopped =
      fetchPartgroupInThisVisitor (fCurrentPartgroupNumber);

  if (! partgroupToBeStopped) {
    // no, but we should have fount it
    stringstream s;

    s <<
      "part group " << fCurrentPartgroupNumber <<
      " not found in this visitor's part groups map" << endl;
    msrInternalError (
      fMsrOptions->fInputSourceName,
      inputLineNumber,
      s.str());
  }

  // remove the part group to be stopped from the part group list
  if (fMsrOptions->fTrace)
    cerr << idtr <<
      "Removing part group " <<
      partgroupToBeStopped->getPartgroupNumber () <<
      " from visitor's part groups list" << endl;

  list<S_msrPartgroup>::iterator
    iBegin = fPartgroupsList.begin(),
    iEnd   = fPartgroupsList.end(),
    i      = iBegin;

  while (true) {
    if (i == iEnd) {
      stringstream s;
      s <<
        "part group " <<
        fCurrentPartgroupNumber <<
        " not found in part groups list";
        
      msrInternalError (
      fMsrOptions->fInputSourceName,
        inputLineNumber,
        s.str());
      break;
    }

    if ((*i) == partgroupToBeStopped) {
      fPartgroupsList.erase (i);
      break;
    }
    
    i++;
  } // while

  showPartgroupsData ("AFTER REMOVAL FROM LIST");
  
  // take care of the part group to be stopped
  // in the part groups list
  if (! fPartgroupsList.size()) {
    
    // we're just removed the only part group in the list:
    // append it to the MSR score
    if (fMsrOptions->fTrace)
      cerr << idtr <<
        "Appending part group " <<
        partgroupToBeStopped->getPartgroupNumber () <<
        " to MSR score" << endl;
        
    fMsrScore->
      addPartgroupToScore (partgroupToBeStopped);
      
  } else {

    // the front element in the part group list is
    // the new current part group
    S_msrPartgroup
      newCurrentPartgroup = fPartgroupsList.front ();

    if (
        partgroupToBeStopped->getPartgroupNumber ()
          ==
        newCurrentPartgroup->getPartgroupNumber () ) {
      cerr << idtr <<
        "--> partgroupToBeStopped = " << partgroupToBeStopped <<
        ", newCurrentPartgroup = " << newCurrentPartgroup << endl;

      stringstream s;
      s <<
        "cannot append part group " <<
        partgroupToBeStopped->getPartgroupNumber () <<
        " as sub part group of itself";
      msrInternalError (
        fMsrOptions->fInputSourceName,
        inputLineNumber,
        s.str());
    }
    
    // insert current group into future current group
    if (fMsrOptions->fTrace)
      cerr << idtr <<
        "Preending (sub-)part group " <<
        partgroupToBeStopped->getPartgroupNumber () <<
        " at the beginning of part group " <<
        newCurrentPartgroup->getPartgroupNumber () << endl;

    newCurrentPartgroup->
      prependSubPartgroupToPartgroup (
        partgroupToBeStopped);
  }

  // remove part group from the map
  // CAUTION: erase() destroys the element it removes!
  if (fMsrOptions->fTrace)
    cerr << idtr <<
      "Removing part group " << fCurrentPartgroupNumber <<
      " from visitor's part group map" << endl;
  try {
    fPartgroupsMap.erase (fCurrentPartgroupNumber);
  }
  catch (int e) {
    cerr << "An exception number " << e << " occurred" << endl;
  }

  showPartgroupsData ("AFTER STOP");
} // handlePartgroupStop ()

void xml2MsrVisitor::visitEnd (S_part_group& elt)
{
  if (fMsrOptions->fTrace)
    cerr << idtr <<
      "Handling part group " << fCurrentPartgroupNumber <<
      ", type: \"" << fCurrentPartgroupType << "\""  << endl;

  idtr++;
  
  msrPartgroup::msrPartgroupTypeKind partgroupTypeKind;

  // check part group type
  if (fCurrentPartgroupType == "start")
    partgroupTypeKind = msrPartgroup::kStartPartgroupType;
    
  else
  if (fCurrentPartgroupType == "stop")
    partgroupTypeKind = msrPartgroup::kStopPartgroupType;
    
  else {
    if (fCurrentPartgroupType.size())
      // part group type may be absent
      msrMusicXMLError (
        fMsrOptions->fInputSourceName,
        elt->getInputLineNumber (),
        "unknown part group type \"" + fCurrentPartgroupType + "\"");
    partgroupTypeKind = msrPartgroup::k_NoPartgroupType;
  }

  msrPartgroup::msrPartgroupSymbolKind partgroupSymbolKind;
  
  // check part group symbol
  // Values include none,
  //  brace, line, bracket, and square; the default is none
 
  if (fCurrentPartgroupSymbol == "brace")
    partgroupSymbolKind = msrPartgroup::kBracePartgroupSymbol;
    
  else
  if (fCurrentPartgroupSymbol == "bracket")
    partgroupSymbolKind = msrPartgroup::kBracketPartgroupSymbol;
    
  else
  if (fCurrentPartgroupSymbol == "line")
    partgroupSymbolKind = msrPartgroup::kLinePartgroupSymbol;
    
  else
  if (fCurrentPartgroupSymbol == "square")
    partgroupSymbolKind = msrPartgroup::kSquarePartgroupSymbol;
    
  else
  if (fCurrentPartgroupSymbol == "none")
    partgroupSymbolKind = msrPartgroup::k_NoPartgroupSymbol;
    
  else {
   if (fCurrentPartgroupSymbol.size())
      // part group type may be absent
      msrMusicXMLError (
        fMsrOptions->fInputSourceName,
        elt->getInputLineNumber (),
        "unknown part group symbol \"" + fCurrentPartgroupSymbol + "\"");
    partgroupSymbolKind = msrPartgroup::k_NoPartgroupSymbol;
  }

  bool partgroupBarline;
  
  // check part group barline
  if (fCurrentPartgroupBarline == "yes")
    partgroupBarline = true;
    
  else
  if (fCurrentPartgroupBarline == "no")
    partgroupBarline = false;
    
  else {
    msrMusicXMLError (
      fMsrOptions->fInputSourceName,
      elt->getInputLineNumber (),
      "unknown part group barline \"" + fCurrentPartgroupBarline + "\"");
    partgroupBarline = false;
  }

  switch (partgroupTypeKind) {
    
    case msrPartgroup::kStartPartgroupType:
      handlePartgroupStart (
        elt->getInputLineNumber (),
        partgroupSymbolKind, partgroupBarline);
      break;
      
    case msrPartgroup::kStopPartgroupType:
      handlePartgroupStop (
        elt->getInputLineNumber ());
      break;
      
    case msrPartgroup::k_NoPartgroupType:
      {}
      break;
  } // switch

  idtr--;
} // visitEnd (S_part_group& elt)

//________________________________________________________________________
void xml2MsrVisitor::visitStart (S_score_part& elt)
{
  fCurrentPartMusicXMLName = elt->getAttributeValue ("id");

  if (fMsrOptions->fDebug)
    cerr << idtr <<
      "Found part name \"" << fCurrentPartMusicXMLName << "\"" << endl;

  fCurrentPartMusicXMLName = "";
  fCurrentPartAbbreviation = "";
  fCurrentPartInstrumentName = "";
}

void xml2MsrVisitor::visitStart (S_part_name& elt)
{
  fCurrentPartName = elt->getValue ();
}

void xml2MsrVisitor::visitStart (S_part_abbreviation& elt)
{
  fCurrentPartAbbreviation = elt->getValue ();
}

void xml2MsrVisitor::visitStart (S_instrument_name& elt)
{
  fCurrentPartInstrumentName = elt->getValue(); // jMI
}

void xml2MsrVisitor::visitEnd (S_score_part& elt)
{
  string scorePartID = elt->getAttributeValue ("id");

  if (fMsrOptions->fTrace)
    cerr << idtr <<
      "Handling part \"" << scorePartID << "\"" << endl;

  idtr++;

  S_msrPartgroup currentPartgroup;

  // is there a current part group?
  if (! fPartgroupsList.size()) {
    // no, create an implicit one
    currentPartgroup =
      createImplicitMSRPartgroup (elt->getInputLineNumber ());
  }

  // fetch current part group
  try {
    currentPartgroup = fPartgroupsList.front ();
  }
  catch (int e) {
    cerr << "An exception number " << e << " occurred" << endl;
  }

  // is this part already present in the current part group?
  fCurrentPart =
    currentPartgroup->
      fetchPartFromPartgroup (scorePartID);

  if (! fCurrentPart) {
    // no, add it to the current part group
    fCurrentPart =
      currentPartgroup->
        addPartToPartgroup (
          elt->getInputLineNumber (), scorePartID);
  }

  // populate current part
  fCurrentPart->
    setPartName (fCurrentPartName);
  fCurrentPart->
    setPartAbbreviation (fCurrentPartAbbreviation);
  fCurrentPart->
    setPartInstrumentName (fCurrentPartInstrumentName);

  // register it in this visitor's parts map
  fPartsMap [scorePartID] = fCurrentPart;

  if (fImplicitPartgroup) {
    // force an implicit part group "stop" on it
    // fCurrentPartgroupNumber hold the value 1
    handlePartgroupStop (
      elt->getInputLineNumber ());

    // forget the implicit group
    fImplicitPartgroup = 0;
  }
    
  showPartgroupsData (
    "AFTER handling part \""+scorePartID+"\"");

  idtr--;
}

//________________________________________________________________________
void xml2MsrVisitor::visitStart (S_part& elt)
{
  string partID = elt->getAttributeValue ("id");

  // is this part already known?
  if (fPartsMap.count (partID))
    fCurrentPart =
      fPartsMap [partID];
  else
    msrInternalError (
      fMsrOptions->fInputSourceName,
      elt->getInputLineNumber (),
      "part "+partID+" is not registered in this visitor's part map");

  if (fMsrOptions->fTrace)
    cerr << idtr <<
      "Analyzing part " << fCurrentPart->getPartCombinedName() << endl;

  idtr++;

  fCurrentStaffNumber = 1; // default if there are no <staff> element

  // is this staff already present?
  fCurrentStaff =
    fCurrentPart->
      fetchStaffFromPart (fCurrentStaffNumber);

  if (! fCurrentStaff) 
    // no, add it to the current part
    fCurrentStaff =
      fCurrentPart->
        addStaffToPart (
          elt->getInputLineNumber (), fCurrentStaffNumber);

  // there can be an anacrusis
  gCurrentMusicXMLLocation.fMeasureNumber = 0;

  fOnGoingRepeat = false;
}

void xml2MsrVisitor::visitEnd (S_part& elt)
{
  idtr--;
}

//______________________________________________________________________________
void xml2MsrVisitor::visitStart ( S_divisions& elt ) 
{
  fCurrentMusicXMLDivisions = (int)(*elt);
  
  if (fCurrentMusicXMLDivisions <= 0)
    msrMusicXMLError (
      fMsrOptions->fInputSourceName,
      elt->getInputLineNumber (),
      "divisions per quarter note should be positive");
  
  if (fMsrOptions->fTrace) {
    cerr << idtr;
    if (fCurrentMusicXMLDivisions== 1)
      cerr << "There is 1 division";
    else
      cerr <<
        "There are " << fCurrentMusicXMLDivisions<<
        " divisions";
    cerr <<
      " per quater note in part " <<
      fCurrentPart->getPartCombinedName() << endl;
  }

  fCurrentPart->setPartDivisions (
    gCurrentMusicXMLLocation.fPositionInMeasure);
}

//______________________________________________________________________________
void xml2MsrVisitor::visitStart ( S_clef& elt )
{ 
  // The optional number attribute refers to staff numbers.
  // If absent (0), apply to all part staves.
  fCurrentClefStaffNumber =
    elt->getAttributeIntValue("number", 0); 

  fCurrentClefLine = 0;;
  fCurrentClefOctaveChange = 0;
  fCurrentClefSign = "";
}

void xml2MsrVisitor::visitStart ( S_clef_octave_change& elt )
  { fCurrentClefOctaveChange = (int)(*elt); }
  
void xml2MsrVisitor::visitStart ( S_line& elt )
  { fCurrentClefLine = (int)(*elt); }
  
void xml2MsrVisitor::visitStart ( S_sign& elt )
  { fCurrentClefSign = elt->getValue(); }

void xml2MsrVisitor::visitEnd ( S_clef& elt ) 
{    
  S_msrClef
    clef =
      msrClef::create (
        fMsrOptions,
        elt->getInputLineNumber (),
        fCurrentClefSign, fCurrentClefLine, fCurrentClefOctaveChange);

  if (fCurrentClefStaffNumber == 0)
    fCurrentPart->setAllPartStavesClef (clef);
  else {
    S_msrStaff
      staff =
        fCurrentPart->
          fetchStaffFromPart (fCurrentClefStaffNumber);
    staff->setStaffClef (clef);
  }
}

//______________________________________________________________________________
void xml2MsrVisitor::visitStart ( S_key& elt )
{
  
  // The optional number attribute refers to staff numbers.
  // If absent (0), apply to all part staves.
  fCurrentKeyStaffNumber =
    elt->getAttributeIntValue ("number", 0);

  fCurrentFifths = 0;
  fCurrentCancel = 0;
  fCurrentMode   = "";
}
  
void xml2MsrVisitor::visitStart ( S_fifths& elt )
  { fCurrentFifths = (int)(*elt); }
  
void xml2MsrVisitor::visitStart ( S_mode& elt )
  { fCurrentMode = elt->getValue(); }

void xml2MsrVisitor::visitStart ( S_cancel& elt )
  { fCurrentCancel = (int)(*elt); }

void xml2MsrVisitor::visitEnd ( S_key& elt ) 
{  
  // create msrKey
  S_msrKey
    key =
      msrKey::create (
        fMsrOptions,
        elt->getInputLineNumber (),
        fCurrentFifths, fCurrentMode, fCurrentCancel);

  if (fCurrentKeyStaffNumber == 0)
    fCurrentPart->setAllPartStavesKey (key);
  else {
    // JMI ???
    S_msrStaff
      staff =
        fCurrentPart->
          fetchStaffFromPart (fCurrentKeyStaffNumber);
    staff->setStaffKey (key);
 // JMI   fCurrentPart->
 //     setAllPartStavesKey (key);
  }
}

//______________________________________________________________________________
void xml2MsrVisitor::visitStart ( S_time& elt )
{  
  /*
    The optional number attribute refers to staff numbers,
    from top to bottom on the system. If absent, the key
    signature applies to all staves in the part.
  */
  fCurrentTimeStaffNumber =
    elt->getAttributeIntValue ("number", 0);
    
  fCurrentTimeSymbol =
    elt->getAttributeValue ("symbol");
  // time symbol="cut" or "common" JMI
  
  fCurrentTimeSenzaMisura = false;

  fCurrentTimeBeats = 0;
  fCurrentTimeBeatType = 0;
  
  fCurrentTimeSymbol = "";
}

void xml2MsrVisitor::visitStart ( S_beats& elt )
{ fCurrentTimeBeats = (int)(*elt); }
  
void xml2MsrVisitor::visitStart ( S_beat_type& elt )
  { fCurrentTimeBeatType = (int)(*elt); }
 
void xml2MsrVisitor::visitStart ( S_senza_misura& elt )
  { fCurrentTimeSenzaMisura = true; }

void xml2MsrVisitor::visitEnd ( S_time& elt ) 
{  
  S_msrTime
    time =
      msrTime::create (
        fMsrOptions,
        elt->getInputLineNumber (),
        fCurrentTimeBeats,
        fCurrentTimeBeatType);

  if (fCurrentTimeStaffNumber == 0)
    fCurrentPart->setAllPartStavesTime (time);
  else {
    S_msrStaff
      staff =
        fCurrentPart->
          fetchStaffFromPart (fCurrentTimeStaffNumber);
    staff->setStaffTime (time);
  }
}

//________________________________________________________________________
void xml2MsrVisitor::visitStart (S_direction& elt)
{
/*
      <direction placement="above">
        <direction-type>
          <words default-y="48" font-size="10.5" font-weight="bold" relative-x="-40" xml:lang="de">Sehr langsam</words>
        </direction-type>
        <staff>1</staff>
        <sound tempo="26"/>
      </direction>
*/
}

void xml2MsrVisitor::visitStart (S_direction_type& elt)
{
  fCurrentDirectionPlacement =
    elt->getAttributeValue ("placement"); //JMI

  fOnGoingDirection = true;
}

void xml2MsrVisitor::visitStart (S_words& elt)
{
  fCurrentDirectionWords = elt->getValue ();
}

void xml2MsrVisitor::visitEnd (S_direction& elt)
{
  fOnGoingDirection = false;
}

//________________________________________________________________________
void xml2MsrVisitor::visitStart (S_staves& elt)
{
  int stavesNumber = int(*elt);

  if (fMsrOptions->fTrace) {
    switch (stavesNumber) {
      case 0:
        cerr << idtr <<
          "There isn't any explicit staff (hence 1 by default)"; // JMI
        break;
      case 1:
        cerr << idtr <<
          "There is 1 staff";
        break;
      default:
        cerr << idtr <<
          "There are " << stavesNumber << " staves";
    } // switch
    cerr <<
      " in part " << fCurrentPart->getPartCombinedName() << endl;
  }

  if (stavesNumber > 1) {
    // add n-1 staves to current part
    int n = 2;
    
    while (n <= stavesNumber) {
      fCurrentPart->
        addStaffToPart (
          elt->getInputLineNumber (), n);
      n++;
    } // while
  }
}

void xml2MsrVisitor::visitStart (S_staff& elt)
{
  /*
        <note>
        <pitch>
          <step>A</step>
          <octave>3</octave>
        </pitch>
        <duration>2</duration>
        <voice>3</voice>
        <type>eighth</type>
        <stem>down</stem>
        <staff>2</staff>
        <beam number="1">end</beam>
      </note>
*/
  int  staffNumber = int(*elt);

//  if (true || fMsrOptions->fDebug)
  if (fMsrOptions->fDebug)
    cerr <<
      idtr <<
      "--> S_staff, staffNumber         = " << staffNumber << endl <<
      idtr <<
      "--> S_staff, fCurrentStaffNumber = " << fCurrentStaffNumber << endl <<
      idtr <<
      "--> S_staff, current staff name  = " << fCurrentStaff->getStaffName() << endl;

  if (fOnGoingForward) {

    fCurrentForwardStaffNumber = staffNumber;

  }
  else if (fOnGoingNote) {

    // regular staff indication in note/rest
    fCurrentStaffNumber = staffNumber;

  }
  else if (fOnGoingDirection) {

    // JMI
    
  }
  else {
    
    stringstream s;
    s << "staff " << staffNumber << " is out of context";
// JMI    msrMusicXMLError (s.str());
    msrMusicXMLWarning (
      fMsrOptions->fInputSourceName,
      elt->getInputLineNumber (),
      s.str());    
  }
}
    
//________________________________________________________________________
void xml2MsrVisitor::visitStart (S_voice& elt )
{
  /*
        <note>
        <pitch>
          <step>A</step>
          <octave>3</octave>
        </pitch>
        <duration>2</duration>
        <voice>3</voice>
        <type>eighth</type>
        <stem>down</stem>
        <staff>2</staff>
        <beam number="1">end</beam>
      </note>
*/
  int voiceNumber = int(*elt);
  
  if (false && fMsrOptions->fDebug)
//  if (fMsrOptions->fDebug)
    cerr <<
      idtr <<
      "--> S_voice, voiceNumber         = " << voiceNumber << endl <<
      idtr <<
      "--> S_voice, fCurrentStaffNumber = " << fCurrentStaffNumber << endl <<
      idtr <<
      "--> S_voice, current staff name  = " << fCurrentStaff->getStaffName() << endl;

  if (fOnGoingForward) {

    fCurrentForwardVoiceNumber = voiceNumber;

  }
  else
  if (fOnGoingNote) {

    // regular voice indication in note/rest
    fCurrentVoiceNumber = voiceNumber;
    fCurrentVoice =
      fCurrentStaff->
        fetchVoiceFromStaff (fCurrentVoiceNumber);
    
  }
  else {
    
    stringstream s;
    s << "voice " << voiceNumber << " is out of context";
    msrMusicXMLError (
      fMsrOptions->fInputSourceName,
      elt->getInputLineNumber (),
      s.str());
    
  }
}

//________________________________________________________________________
void xml2MsrVisitor::visitStart (S_backup& elt )
{
/*
 The backup and forward elements are required to coordinate multiple voices in one part, including music on multiple staves. The backup type is generally used to move between voices and staves. Thus the backup element does not include voice or staff elements. Duration values should always be positive, and should not cross measure boundaries or mid-measure changes in the divisions value.
 
      <backup>
        <duration>8</duration>
      </backup>
*/
  
  gCurrentMusicXMLLocation.fInputLineNumber =
    elt->getInputLineNumber ();
  
  fOnGoingBackup = true;
}

void xml2MsrVisitor::visitEnd (S_backup& elt )
{
  if (fMsrOptions->fTrace)
    cerr << idtr <<
      "Handling 'backup <<< " << fCurrentBackupDuration <<
      " divisions'" << endl;

  int
    saveCurrentPositionInMeasure =
      gCurrentMusicXMLLocation.fPositionInMeasure;
  
  gCurrentMusicXMLLocation.fPositionInMeasure =-
    fCurrentBackupDuration;

  if (gCurrentMusicXMLLocation.fPositionInMeasure < 0) {
    stringstream s;
    s <<
      "backup divisions " << fCurrentBackupDuration <<
      " from position " << saveCurrentPositionInMeasure <<
      " crosses measure left boundary";
// JMI    msrMusicXMLError (s.str());
    msrMusicXMLWarning (
      fMsrOptions->fInputSourceName,
      elt->getInputLineNumber (),
      s.str());
  }
  
  fOnGoingBackup = false;
}

//______________________________________________________________________________
void xml2MsrVisitor::visitStart ( S_forward& elt )
{
  /*
         <forward>
        <duration>96</duration>
        <voice>1</voice>
        <staff>1</staff>
      </forward>
*/
  fOnGoingForward = true;
}

void xml2MsrVisitor::visitEnd ( S_forward& elt )
{
  // change staff
  fCurrentStaffNumber = fCurrentForwardStaffNumber;

  // is the new staff already present?
  fCurrentStaff =
    fCurrentPart->
      fetchStaffFromPart (fCurrentStaffNumber);

  if (! fCurrentStaff) 
    // no, add it to the current part
    fCurrentStaff =
      fCurrentPart->
        addStaffToPart (
          elt->getInputLineNumber (), fCurrentStaffNumber);

  // is the new current voice known?
  fCurrentVoiceNumber = fCurrentForwardVoiceNumber;
  fCurrentVoice =
    fCurrentStaff->
      fetchVoiceFromStaff (fCurrentVoiceNumber);

  if (! fCurrentVoice) 
    // no, add it to the current staff
    fCurrentVoice =
      fCurrentStaff->
        addVoiceToStaff (
          elt->getInputLineNumber (), fCurrentVoiceNumber);
/* JMI
  if (! fCurrentVoice) {
    stringstream s;
    s <<
      "voice " << fCurrentVoiceNumber <<
      " in forward not found (staff = " << fCurrentStaffNumber <<
      ", duration = " << fCurrentForwardDuration << ")";
    msrMusicXMLError (s.str());
  }
*/

  if (fMsrOptions->fTrace)
    cerr << idtr <<
      "Handling 'forward >>> " << fCurrentForwardDuration <<
      "', thus switching to " <<
      "voice " << fCurrentVoice->getVoiceName () <<
      " in staff " << fCurrentStaff->getStaffName () << endl;

  fOnGoingForward = false;
}

//________________________________________________________________________
void xml2MsrVisitor::visitStart ( S_metronome& elt )
{
  string parentheses = elt->getAttributeValue("parentheses");
  
  fBeatsData.clear();
  fPerMinute = 0;
  fCurrentBeat.fBeatUnit = "";
  fCurrentBeat.fDots = 0;

  if (parentheses.size()) {
    // cerr << "--> S_metronome, parentheses = " << parentheses << endl;
    
    if (parentheses == "yes") 
      fParentheses = true;
    else
    if (parentheses == "no")
      fParentheses = true;
    else {
      stringstream s;
      s << "parentheses value " << parentheses << " should be 'yes' or 'no'";
      msrMusicXMLError (
        fMsrOptions->fInputSourceName,
        elt->getInputLineNumber (),
        s.str());
    }
  }
}
  
void xml2MsrVisitor::visitEnd ( S_metronome& elt )
{ 
 // if (fSkipDirection) return;

  // fParentheses ??? JMI
  if (fCurrentBeat.fBeatUnit.size()) { // JMI
    fBeatsData.push_back(fCurrentBeat);
    fCurrentBeat.fBeatUnit = "";
    fCurrentBeat.fDots = 0;
  }
  
  if (fBeatsData.size() != 1) {
    msrMusicXMLWarning (
      fMsrOptions->fInputSourceName,
      elt->getInputLineNumber (),
      "multiple beats found, but only per-minute tempos is supported");
    return;
  }
  
  if (! fPerMinute) {
    msrMusicXMLWarning (
      fMsrOptions->fInputSourceName,
      elt->getInputLineNumber (),
      "per-minute not found, only per-minute tempos is supported");
    return;    // support per minute tempo only (for now)
  }

  musicXMLBeatData b = fBeatsData[0];
  rational         r = 
    NoteType::type2rational(
      NoteType::xml (b.fBeatUnit)), rdot(3,2);
  
  while (b.fDots-- > 0) { // JMI
    r *= rdot;
  }
  r.rationalise ();

  S_msrTempo
    tempo =
      msrTempo::create (
        fMsrOptions,
        elt->getInputLineNumber (),
        r.getDenominator(), fPerMinute);
    
  // is fCurrentStaffNumber already present in fCurrentPart?
  fCurrentStaff =
    fCurrentPart->
      fetchStaffFromPart (fCurrentStaffNumber);

  if (! fCurrentStaff) 
    // no, add it to the current part
    fCurrentStaff =
      fCurrentPart->
        addStaffToPart (
          elt->getInputLineNumber (), fCurrentStaffNumber);
    
  // fetch the voice in the current staff
  fCurrentVoice =
    fCurrentStaff->
      fetchVoiceFromStaff (fCurrentVoiceNumber);

  // does the voice exist?
  if (! fCurrentVoice) 
    // no, add it to the current staff
    fCurrentVoice =
      fCurrentStaff->
        addVoiceToStaff (
          elt->getInputLineNumber (), fCurrentVoiceNumber);

  fCurrentVoice->
    appendTempoToVoice (tempo);
  
  // JMI if (fCurrentOffset) addDelayed(cmd, fCurrentOffset);
}

void xml2MsrVisitor::visitStart ( S_beat_unit& elt )
{ 
  if (fCurrentBeat.fBeatUnit.size()) {
    fBeatsData.push_back (fCurrentBeat); 
    fCurrentBeat.fBeatUnit = "";
    fCurrentBeat.fDots = 0;
  }
  fCurrentBeat.fBeatUnit = elt->getValue();
}

void xml2MsrVisitor::visitStart ( S_beat_unit_dot& elt )
  { fCurrentBeat.fDots++; }
  
void xml2MsrVisitor::visitStart ( S_per_minute& elt )
  { fPerMinute = (int)(*elt); }


//________________________________________________________________________
void xml2MsrVisitor::visitStart (S_tied& elt )
{
//           <tied orientation="over" type="start"/>

fCurrentTiedType =
  elt->getAttributeValue ("type");

fCurrentTiedOrientation =
  elt->getAttributeValue ("orientation");

 if (fCurrentTiedType == "start") { // JMI
    
//    fCurrentTiedKind = msrTied::kStartTied;
    
  }
  else
  if (fCurrentTiedType == "continue") {
    
//    fCurrentTiedKind = msrTied::kContinueTied;
    fMusicXMLNoteData.fMusicXMLNoteIsTied = true;
    
  }
  else
  if (fCurrentTiedType == "stop") {
    
//    fCurrentTiedKind = msrTied::kStopTied;
    fMusicXMLNoteData.fMusicXMLNoteIsTied = true;
    
  } else {

    // inner tied notes may miss the "continue" type:
    // let' complain on slur notes outside of slurs 
//    if (! fOnGoingSlur) JMI
      if (fCurrentTiedType.size()) {
        stringstream s;
        s << "tied type" << fCurrentSlurType << "unknown";
        msrMusicXMLError (
          fMsrOptions->fInputSourceName,
          elt->getInputLineNumber (),
          s.str());
      }
      
    }
}

void xml2MsrVisitor::visitStart (S_slur& elt )
{
//          <slur number="1" placement="above" type="start"/>
  fCurrentSlurNumber =
    elt->getAttributeIntValue ("number", 0);

  fCurrentSlurType =
    elt->getAttributeValue ("type");

  fCurrentSlurPlacement =
    elt->getAttributeValue ("placement");

  if (fCurrentSlurType == "start") {
    
    fCurrentSlurKind = msrSlur::kStartSlur;
    fOnGoingSlur = true;
    
  }
  else if (fCurrentSlurType == "continue") {
    
    fCurrentSlurKind = msrSlur::kContinueSlur;
    
  }
  else if (fCurrentSlurType == "stop") {
    
    fCurrentSlurKind = msrSlur::kStopSlur;
    fOnGoingSlur = false;
    
  }
  else {

    // inner slur notes may miss the "continue" type:
    // let' complain on slur notes outside of slurs 
    if (! fOnGoingSlur)
      if (fCurrentSlurType.size()) {
        stringstream s;
        s << "slur type" << fCurrentSlurType << "unknown";
        msrMusicXMLError (
          fMsrOptions->fInputSourceName,
          elt->getInputLineNumber (),
          s.str());
      }
      
    }
}

//________________________________________________________________________
void xml2MsrVisitor::visitStart (S_lyric& elt )
{ 
  fCurrentLyricsNumber =
    elt->getAttributeIntValue ("number", 0);

  // is voice fCurrentVoiceNumber present in current staff?
  fCurrentVoice =
    fCurrentStaff->
      fetchVoiceFromStaff (fCurrentVoiceNumber);

  if (! fCurrentVoice)
    // no, add it to the staff
    fCurrentVoice =
      fCurrentStaff->
        addVoiceToStaff (
          elt->getInputLineNumber (), fCurrentVoiceNumber);
        
  // is lyrics fCurrentLyricsNumber present in current voice?
  fCurrentLyrics =
    fCurrentVoice->
      fetchLyricsFromVoice (fCurrentLyricsNumber);

  if (! fCurrentLyrics)
    // no, add it to the voice
    fCurrentLyrics =
      fCurrentVoice->
        addLyricsToVoice (
          elt->getInputLineNumber (), fCurrentLyricsNumber);
        
  fCurrentLyricsHasText = false;
  fCurrentText = "";
  fCurrentElision = false;

  fCurrentNoteHasLyrics = true;
}

void xml2MsrVisitor::visitStart ( S_syllabic& elt )
{
  string syllabic = elt->getValue();
  
  if      (syllabic == "single")
    fCurrentLyricschunkType = msrLyricschunk::kSingleChunk;
  else if (syllabic == "begin")
    fCurrentLyricschunkType = msrLyricschunk::kBeginChunk;
  else if (syllabic == "middle")
    fCurrentLyricschunkType = msrLyricschunk::kMiddleChunk;
  else if (syllabic == "end")
    fCurrentLyricschunkType = msrLyricschunk::kEndChunk;
  else {
    stringstream s;
    s << "--> syllabic \"" << syllabic << "\" is unknown";
    msrMusicXMLError (
      fMsrOptions->fInputSourceName,
      elt->getInputLineNumber (),
      s.str());

    fCurrentLyricschunkType = msrLyricschunk::k_NoChunk;
  }

/* JMI
  if (fCurrentSyllabic == "begin") {
    fOnGoingLyrics = true;
  }
  else if (fCurrentSyllabic == "end") {
    fOnGoingLyrics = true;
  }
  */
}

void xml2MsrVisitor::visitEnd ( S_text& elt ) 
{
  string text = elt->getValue();

  // text may be composed of only spaces, so:
  string dest;
  for_each (
    text.begin(), text.end(), stringSpaceRemover (dest));

  if (fCurrentElision)
    fCurrentText += " "+dest;
  else
    fCurrentText = dest;

  fCurrentLyricsHasText = true;
}

/*
  cerr <<
    "--> lyricNumber = " << lyricNumber <<
    ", fCurrentSyllabic = " << fCurrentSyllabic <<
    ", fCurrentText = |" << fCurrentText << "|" << endl;
*/

/*
      <note default-x="143">
        <pitch>
          <step>E</step>
          <alter>-1</alter>
          <octave>4</octave>
        </pitch>
        <duration>6</duration>
        <voice>1</voice>
        <type>eighth</type>
        <stem default-y="-5">up</stem>
        <beam number="1">begin</beam>
        <lyric default-y="-80" justify="left" number="1">
          <syllabic>single</syllabic>
          <text font-family="FreeSerif" font-size="11">1.</text>
          <elision> </elision>
          <syllabic>begin</syllabic>
          <text font-family="FreeSerif" font-size="11">A</text>
        </lyric>
        <lyric default-y="-97" justify="left" number="2">
          <syllabic>single</syllabic>
          <text font-family="FreeSerif" font-size="11">2.</text>
          <elision> </elision>
          <syllabic>single</syllabic>
          <text font-family="FreeSerif" font-size="11">'T</text>
          <elision> </elision>
          <syllabic>single</syllabic>
          <text font-family="FreeSerif" font-size="11">was</text>
        </lyric>
        <lyric default-y="-113" justify="left" number="3">
          <syllabic>single</syllabic>
          <text font-family="FreeSerif" font-size="11">3.</text>
          <elision> </elision>
          <syllabic>single</syllabic>
          <text font-family="FreeSerif" font-size="11">Throug</text>
          <extend type="start"/>
        </lyric>
*/

void xml2MsrVisitor::visitEnd ( S_elision& elt ) 
{
  fCurrentElision = true;
}

void xml2MsrVisitor::visitEnd ( S_lyric& elt )
{
 // JMI  handleLyricsText (elt->getInputLineNumber ());

  // avoiding handling of the same by visitEnd ( S_note )
  fCurrentLyricschunkType = msrLyricschunk::k_NoChunk;

}
/*
  if (
    fCurrentSlurKind == msrSlur::kContinueSlur
      ||
    fCurrentSlurKind == msrSlur::kStopSlur) {
      
    fCurrentLyrics->
      addSlurChunkToLyrics (
        lyricMsrDuration);
        
  } else if (fMusicXMLNoteData.fMusicXMLNoteIsTied) {
    
    fCurrentLyrics->
      addTiedChunkToLyrics (
        lyricMsrDuration);
        
  } else {
    
    // there can be notes without any slur indication

    if (fMusicXMLNoteData.fMusicXMLStepIsARest) {
      
      fCurrentLyrics->
        addSkipChunkToLyrics (
          lyricMsrDuration);
        
    } else if (
    // JMI fOnGoingSlur && JMI
    ! fCurrentNoteHasLyrics) {

      fCurrentLyrics->
        addSlurChunkToLyrics ( 
  //      addSkipChunkToLyrics (// JMI ???
          lyricMsrDuration);

    } else {
      
      fCurrentLyrics->
        addTextChunkToLyrics (
          fCurrentSyllabic,
          fCurrentText,
          fCurrentElision,
          lyricMsrDuration);
          
    }
  }
  */

/*
      <note default-x="61">
        <pitch>
          <step>D</step>
          <octave>4</octave>
        </pitch>
        <duration>8</duration>
        <voice>1</voice>
        <type>quarter</type>
        <stem default-y="-10">up</stem>
        <lyric default-y="-80" number="1">
          <syllabic>single</syllabic>
          <text>オノ</text>
        </lyric>
      </note>
*/

/* JMI lyric without text
        <lyric name="verse" number="3">
          <extend type="stop"/>
        </lyric>
*/

//________________________________________________________________________
void xml2MsrVisitor::visitStart (S_measure& elt)
{
  gCurrentMusicXMLLocation.fMeasureNumber =
    elt->getAttributeIntValue ("number", 0);

  // is this measure number in the debug set?
  set<int>::iterator
    it =
      fMsrOptions->fDebugMeasureNumbersSet.find (
        gCurrentMusicXMLLocation.fMeasureNumber);
        
  if (it != fMsrOptions->fDebugMeasureNumbersSet.end ()) {
    // yes, activate debug for it
    fMsrOptions->fSaveDebug = fMsrOptions->fDebug;
    fMsrOptions->fSaveDebugDebug = fMsrOptions->fDebugDebug;
  }

  gCurrentMusicXMLLocation.fPositionInMeasure = 0;
    
  if (fMsrOptions->fDebug)
    cerr << idtr << 
      "=== MEASURE " <<
      gCurrentMusicXMLLocation.fMeasureNumber << " === " <<
      "PART " << fCurrentPart->getPartCombinedName () <<" ===" << endl;

  S_msrBarCheck
    barCheck =
      msrBarCheck::create (
        fMsrOptions,
        elt->getInputLineNumber (),
        gCurrentMusicXMLLocation.fMeasureNumber);
            
  // append it to the voice
  if (fCurrentVoice)
    // it may not have been created yet JMI
    fCurrentVoice->
      appendBarCheckToVoice (barCheck);
}

void xml2MsrVisitor::visitEnd (S_measure& elt)
{
  // restore debug options in case they were set in visitStart()
  fMsrOptions->fDebug = fMsrOptions->fSaveDebug;
  fMsrOptions->fDebugDebug = fMsrOptions->fSaveDebugDebug;
}

//______________________________________________________________________________
void xml2MsrVisitor::visitStart ( S_print& elt ) 
{
  const string& newSystem = elt->getAttributeValue ("new-system");
  
  if (newSystem.size()) {
    
    if (newSystem == "yes") {
      // create a barnumbercheck command
      S_msrBarnumberCheck
        barnumbercheck_ =
          msrBarnumberCheck::create (
            fMsrOptions,
            elt->getInputLineNumber (),
            gCurrentMusicXMLLocation.fMeasureNumber);
            
      // append it to the voice
// JMI      S_msrElement bnc = barnumbercheck_;
      fCurrentVoice->
        appendBarnumberCheckToVoice (barnumbercheck_);
  
      // create a break command
      S_msrBreak
        break_ =
          msrBreak::create(
            fMsrOptions,
            elt->getInputLineNumber (),
            gCurrentMusicXMLLocation.fMeasureNumber);
  
      // append it to the voice
      S_msrElement brk = break_;
      fCurrentVoice->
        appendBreakToVoice (break_);
    
      // add a break chunk to the voice master lyrics
      fCurrentVoice->
        getVoiceMasterLyrics ()->
          addBreakChunkToLyrics (
            elt->getInputLineNumber (),
            gCurrentMusicXMLLocation.fMeasureNumber);
    }
    
    else if (newSystem == "no") {
      // ignore it
    }
    
    else {
      stringstream s;
  
      s << "unknown 'new-system' value '" << newSystem <<
      "' in '<print />', should be 'yes', 'no' or empty";
      
      msrMusicXMLError (
        fMsrOptions->fInputSourceName,
        elt->getInputLineNumber (),
        s.str());
    }
  }
}

/*
  http://www.musicxml.com/for-developers/musicxml-dtd/barline-elements/
 
  <barline location="left">
    <bar-style>heavy-light</bar-style>
    <repeat direction="forward"/>
  </barline>
  
  <barline location="right">
    <bar-style>light-heavy</bar-style>
    <repeat direction="backward"/>
  </barline>

http://usermanuals.musicxml.com/MusicXML/Content/EL-MusicXML-repeat.htm

      <barline location="left">
        <bar-style>heavy-light</bar-style>
        <repeat direction="forward" winged="none"/>
      </barline>
      
      <barline location="right">
        <bar-style>light-heavy</bar-style>
        <ending number="1, 2" type="stop"/>
        <repeat direction="backward" winged="none"/>
      </barline>

  Repeat start:
      <barline location="left">
        <bar-style>heavy-light</bar-style>
        <repeat direction="forward"/>
      </barline>

  In the middle of a measure: (MozartTrio.xml)
    <measure number="X1" implicit="yes">
      <barline location="left">
        <bar-style>heavy-light</bar-style>
        <repeat direction="forward"/>
      </barline>
      <note>
        <rest/>
        <duration>6</duration>
        <voice>1</voice>
        <type>quarter</type>
      </note>
    </measure>

  Repeat end:
    implicit at end or part if nothing specified

  In the middle of a measure: (MozartTrio.xml)
    <measure number="12">
      <note>
        <pitch>
          <step>C</step>
          <octave>5</octave>
        </pitch>
        <duration>6</duration>
        <voice>1</voice>
        <type>quarter</type>
        <stem>down</stem>
      </note>
      <note>
        <rest/>
        <duration>6</duration>
        <voice>1</voice>
        <type>quarter</type>
      </note>
      <barline location="right">
        <bar-style>light-heavy</bar-style>
        <repeat direction="backward"/>
      </barline>
    </measure>
  
  Double bar:
      <barline location="right">
        <bar-style>light-light</bar-style>
      </barline>

  End of part:
      <barline location="right">
        <bar-style>light-light</bar-style>
      </barline>

(Saltarello.xml):
      <barline location="left">
        <ending type="start" number="1"/>
      </barline>

      <barline location="right">
        <bar-style>light-heavy</bar-style>
        <ending type="stop" number="1"/>
        <repeat direction="backward"/>
      </barline>

    Endings refers to multiple (e.g. first and second) endings.
    Typically, the start type is associated with the left
    barline of the first measure in an ending. The stop and
    discontinue types are associated with the right barline of
    the last measure in an ending. Stop is used when the ending
    mark concludes with a downward jog, as is typical for first
    endings. Discontinue is used when there is no downward jog,
    as is typical for second endings that do not conclude a
    piece. The length of the jog can be specified using the
    end-length attribute. The text-x and text-y attributes
    are offsets that specify where the baseline of the start
    of the ending text appears, relative to the start of the
    ending line.

    The number attribute reflects the numeric values of what
    is under the ending line. Single endings such as "1" or
    comma-separated multiple endings such as "1, 2" may be
    used. The ending element text is used when the text
    displayed in the ending is different than what appears in
    the number attribute. The print-object element is used to
    indicate when an ending is present but not printed, as is
    often the case for many parts in a full score.
    
*/

//______________________________________________________________________________
void xml2MsrVisitor::visitStart ( S_barline& elt ) 
{
/*
      <barline location="right">
        <bar-style>light-heavy</bar-style>
        <ending type="stop" number="1"/>
        <repeat direction="backward"/>
      </barline>
*/
  fCurrentLocation        = "";
  fCurrentStyle           = "";
  fCurrentEndingtype      = "";
  fCurrentEndingNumber    = "";
  fCurrentRepeatDirection = "";
  fCurrentRepeatWinged    = "";

  fCurrentBarlineLocation        = msrBarline::k_NoLocation;
  fCurrentBarlineStyle           = msrBarline::k_NoStyle;
  fCurrentBarlineEndingType      = msrBarline::k_NoEndingType;
  fCurrentBarlineEndingNumber    = ""; // may be "1, 2"
  fCurrentBarlineRepeatDirection = msrBarline::k_NoRepeatDirection;
  fCurrentBarlineRepeatWinged    = msrBarline::k_NoRepeatWinged;

  fCurrentLocation = elt->getAttributeValue ("location");

  fCurrentBarlineLocation =
    msrBarline::kRight; // by default
    
  if       (fCurrentLocation == "left") {
    fCurrentBarlineLocation =
      msrBarline::kLeft;
  }
  else  if (fCurrentLocation == "middle") {
    fCurrentBarlineLocation =
      msrBarline::kMiddle;
  }
  else if  (fCurrentLocation == "right") {
    fCurrentBarlineLocation =
      msrBarline::kRight;
  }
  else {
    stringstream s;
    s << "barline location " << fCurrentLocation << " is unknown";
    msrMusicXMLError (
      fMsrOptions->fInputSourceName,
      elt->getInputLineNumber (),
      s.str());
  }
}

void xml2MsrVisitor::visitStart ( S_bar_style& elt ) 
{
  fCurrentStyle = elt->getValue();

  fCurrentBarlineStyle =
    msrBarline::k_NoStyle;

  if      (fCurrentStyle == "regular") {
    fCurrentBarlineStyle =
      msrBarline::kRegular;
  }
  else if (fCurrentStyle == "dotted") {
    fCurrentBarlineStyle =
      msrBarline::kDotted;
  }
  else if (fCurrentStyle == "dashed") {
    fCurrentBarlineStyle =
      msrBarline::kDashed;
  }
  else if (fCurrentStyle == "heavy") {
    fCurrentBarlineStyle =
      msrBarline::kHeavy;
  }
  else if (fCurrentStyle == "light-light") {
    fCurrentBarlineStyle =
      msrBarline::kLightLight;
  }
  else if (fCurrentStyle == "light-heavy") {
    fCurrentBarlineStyle =
      msrBarline::kLightHeavy;
  }
  else if (fCurrentStyle == "heavy-light") {
    fCurrentBarlineStyle =
      msrBarline::kHeavyLight;
  }
  else if (fCurrentStyle == "heavy-heavy") {
    fCurrentBarlineStyle =
      msrBarline::kHeavyHeavy;
  }
  else if (fCurrentStyle == "tick") {
    fCurrentBarlineStyle =
      msrBarline::kTick;
  }
  else if (fCurrentStyle == "short") {
    fCurrentBarlineStyle =
      msrBarline::kShort;
  }
  else {
    msrMusicXMLError (
      fMsrOptions->fInputSourceName,
      elt->getInputLineNumber (),
      "barline style " + fCurrentStyle + " is unknown");
  }
}

void xml2MsrVisitor::visitStart ( S_ending& elt ) 
{  
  fCurrentEndingtype   = elt->getAttributeValue ("type");  
  fCurrentEndingNumber = elt->getAttributeValue ("number");  
  
  if       (fCurrentEndingtype == "start") {
    fCurrentBarlineEndingType =
      msrBarline::kStart;
  }
  else  if (fCurrentEndingtype == "stop") {
    fCurrentBarlineEndingType =
      msrBarline::kStop;
  }
  else  if (fCurrentEndingtype == "discontinue") {
    fCurrentBarlineEndingType =
      msrBarline::kDiscontinue;
  }
  else {
    stringstream s;
    s << "ending type " << fCurrentEndingtype << " is unknown";
    msrMusicXMLError (
      fMsrOptions->fInputSourceName,
      elt->getInputLineNumber (),
      s.str());
  }

  fCurrentBarlineEndingNumber =
    elt->getAttributeValue ("number"); // may be "1, 2"
}

void xml2MsrVisitor::visitStart ( S_repeat& elt ) 
{
  fCurrentRepeatDirection =
    elt->getAttributeValue ("direction");

  fCurrentRepeatWinged =
    elt->getAttributeValue ("winged");

  fCurrentBarlineRepeatDirection =
    msrBarline::k_NoRepeatDirection;
    
  if       (fCurrentRepeatDirection == "forward") {
    fCurrentBarlineRepeatDirection =
      msrBarline::kForward;
  }
  else  if (fCurrentRepeatDirection == "backward") {
    fCurrentBarlineRepeatDirection =
      msrBarline::kBackward;
  }
  else {
    stringstream s;
    s << "repeat direction " << fCurrentRepeatDirection << " is unknown";
    msrMusicXMLError (
      fMsrOptions->fInputSourceName,
      elt->getInputLineNumber (),
      s.str());
  }

  fCurrentBarlineRepeatWinged =
    msrBarline::k_NoRepeatWinged;

  if (fCurrentRepeatWinged.size()) {
    if       (fCurrentRepeatWinged == "straight") {
      fCurrentBarlineRepeatWinged =
        msrBarline::kStraight;
    }
    else  if (fCurrentRepeatWinged == "curved") {
      fCurrentBarlineRepeatWinged =
        msrBarline::kCurved;
    }
    else  if (fCurrentRepeatWinged == "doubleStraight") {
      fCurrentBarlineRepeatWinged =
        msrBarline::kDoubleStraight;
    }
    else  if (fCurrentRepeatWinged == "doubleCurved") {
      fCurrentBarlineRepeatWinged =
        msrBarline::kDoubleCurved;
    }
    else {
      stringstream s;
      s << "repeat winged " << fCurrentRepeatWinged << " is unknown";
      msrMusicXMLError (
        fMsrOptions->fInputSourceName,
        elt->getInputLineNumber (),
        s.str());
    }
  }
}

void xml2MsrVisitor::visitEnd ( S_barline& elt ) 
{
  /*
  There may be a barline in a part before any music
  */
  
  // is fCurrentStaffNumber already present in fCurrentPart?
  fCurrentStaff =
    fCurrentPart->
      fetchStaffFromPart (fCurrentStaffNumber);

  if (! fCurrentStaff) 
    // no, add it to the current part
    fCurrentStaff =
      fCurrentPart->
        addStaffToPart (
          elt->getInputLineNumber (), fCurrentStaffNumber);
    
  // fetch the voice in the current staff
  fCurrentVoice =
    fCurrentStaff->
      fetchVoiceFromStaff (fCurrentVoiceNumber);

  // does the voice exist?
  if (! fCurrentVoice) 
    // no, add it to the current staff
    fCurrentVoice =
      fCurrentStaff->
        addVoiceToStaff (
          elt->getInputLineNumber (), fCurrentVoiceNumber);

  // create the barline
  S_msrBarline
    barline =
      msrBarline::create (
        fMsrOptions,
        elt->getInputLineNumber (),
        fCurrentBarlineLocation,
        fCurrentBarlineStyle,
        fCurrentBarlineEndingType,
        fCurrentBarlineEndingNumber,
        fCurrentBarlineRepeatDirection,
        fCurrentBarlineRepeatWinged);

  // don't display the barline yet in case of debug,
  // wait until its category is defined
  // append the barline to the current voice chunk

  // handle the barline according to:
  // http://www.musicxml.com/tutorial/the-midi-compatible-part/repeats/

  bool barlineIsAlright = false;

  if (
    fCurrentBarlineLocation == msrBarline::kLeft
      &&
    fCurrentBarlineRepeatDirection == msrBarline::kForward) {
    // repeat start
    // ------------------------------------------------------
    
        /*
        A forward repeat mark is represented by a left barline at the beginning of the measure (following the attributes element, if there is one):
        
          <barline location="left">
            <bar-style>heavy-light</bar-style>
            <repeat direction="forward"/>
          </barline>
        */

//      if (fMsrOptions->fDebug)
      cerr <<
        idtr << "--> input line " << elt->getInputLineNumber () <<
        endl <<
        idtr <<
        "--> barline, left and forward: repeat start" <<
        endl;

    // set the barline category
    barline->
      setBarlineCategory (msrBarline::kRepeatStart);
  
    // get the current voice chunk
    S_msrVoicechunk
      currentVoicechunk =
        fCurrentVoice->
          getVoicechunk ();

    if (! fCurrentRepeat) {
      // create the repeat
      if (fMsrOptions->fTrace)
        cerr << idtr <<
          "Creating a repeat in voice " <<
          fCurrentVoice->getVoiceName () << endl;
  
      fCurrentRepeat =
        msrRepeat::create (
          fMsrOptions, elt->getInputLineNumber (),
          currentVoicechunk,
          fCurrentVoice);
    }
    
    // create a new voice chunk for the voice
    if (fMsrOptions->fDebug)
      cerr << idtr <<
        "--> setting new voice chunk for voice " <<
        fCurrentVoice->getVoiceName () << endl;
        
    fCurrentVoice->
      setNewVoicechunkForVoice (
        elt->getInputLineNumber ());

    // append the bar line to the new current voice chunk
    fCurrentVoice->
      appendBarlineToVoice (barline);

    // push the barline onto the stack
    fPendingBarlines.push (barline);

    barlineIsAlright = true;
  }
  
  else if (
    fCurrentBarlineLocation == msrBarline::kRight
      &&
    fCurrentBarlineEndingType == msrBarline::kStop
 // JMI     &&
 //   fCurrentBarlineRepeatDirection == msrBarline::kBackward
 ) {
    // hooked ending end
    // ------------------------------------------------------
    
    /*
    The stop value is used when the end of the ending is marked with a downward hook, as is typical for a first ending. It is usually used together with a backward repeat at the end of a measure:
    
      <barline location="right">
        <bar-style>light-heavy</bar-style>
        <ending type="stop" number="1"/>
        <repeat direction="backward"/>
      </barline>
    */
//       if (fMsrOptions->fDebug)
      cerr <<
        idtr << "--> input line " << elt->getInputLineNumber () <<
        endl <<
        idtr <<
        "--> barline right, stop and backward: hooked ending end" <<
        endl;

    // set the barline category
    barline->
      setBarlineCategory (msrBarline::kHookedEndingEnd);
  
    // append the bar line to the current voice chunk
    fCurrentVoice->
      appendBarlineToVoice (barline);

    // get the current voice chunk
    S_msrVoicechunk
      currentVoicechunk =
        fCurrentVoice->
          getVoicechunk ();

    // create new voice chunk from current voice
    if (fMsrOptions->fDebug)
      cerr << idtr <<
        "--> setting new voice chunk for voice " <<
        fCurrentVoice->getVoiceName () << endl;
        
    fCurrentVoice->
      setNewVoicechunkForVoice (
        elt->getInputLineNumber ());

    if (! fCurrentRepeat) {
      // create the repeat
      if (fMsrOptions->fTrace)
        cerr << idtr <<
          "Creating a repeat for voice " <<
          fCurrentVoice->getVoiceName () << endl;

      fCurrentRepeat =
        msrRepeat::create (
          fMsrOptions, elt->getInputLineNumber (),
          currentVoicechunk,
          fCurrentVoice);
    }
      
    // create a repeat ending from the current voice chunk
    if (fMsrOptions->fDebug)
      cerr << idtr <<
        "--> creating a new hooked repeat ending for voice " <<
        fCurrentVoice->getVoiceName () << endl;
        
    S_msrRepeatending
      repeatEnding =
        msrRepeatending::create (
          fMsrOptions, elt->getInputLineNumber (),
          fCurrentBarlineEndingNumber,
          msrRepeatending::kHookedEnding,
          currentVoicechunk,
          fCurrentRepeat);

    // append it to the current repeat
    if (fMsrOptions->fDebug)
      cerr << idtr <<
        "--> appending repeat ending to current repeat in voice " <<
        fCurrentVoice->getVoiceName () << endl;
        
    fCurrentRepeat->
      addRepeatending (repeatEnding);
    
    if (fPendingBarlines.empty ()) {
//       if (fMsrOptions->fDebug)
      cerr <<
        idtr <<
        "--> there's an implicit repeat start at the beginning of the part" <<
        endl;

      // create the implicit barline
      S_msrBarline
        implicitBarline =
          msrBarline::create (
            fMsrOptions,
            elt->getInputLineNumber (),
            msrBarline::kLeft,
            msrBarline::kHeavyLight,
            msrBarline::kStart,
            fCurrentBarlineEndingNumber,
            msrBarline::kForward,
            fCurrentBarlineRepeatWinged);

      // set the implicit barline category
      implicitBarline->
        setBarlineCategory (
          msrBarline::kRepeatStart);
    
      // prepend the implicit barline to the current voice chunk
      fCurrentVoice->
        prependBarlineToVoice (implicitBarline);
              
      // get the current voice chunk
      S_msrVoicechunk
        currentVoicechunk =
          fCurrentVoice->
            getVoicechunk ();

      if (! fCurrentRepeat) {
        // create the repeat
        if (fMsrOptions->fTrace)
          cerr << idtr <<
            "Creating a repeat in voice " <<
            fCurrentVoice->getVoiceName () << endl;
  
        fCurrentRepeat =
          msrRepeat::create (
            fMsrOptions, elt->getInputLineNumber (),
            currentVoicechunk,
            fCurrentVoice);
      }
      
      // create a new voice chunk for the voice
      if (fMsrOptions->fDebug)
        cerr << idtr <<
          "--> setting new voice chunk for voice " <<
          fCurrentVoice->getVoiceName () << endl;
          
      fCurrentVoice->
        setNewVoicechunkForVoice (
          elt->getInputLineNumber ());

      // add the repeat to the new voice chunk
      if (fMsrOptions->fDebug)
        cerr << idtr <<
          "--> appending the repeat to voice " <<
          fCurrentVoice->getVoiceName () << endl;

      fCurrentVoice->
        appendRepeatToVoice (fCurrentRepeat);

      barlineIsAlright = true;
    }
    
    else {
      // pop the pending barline off the stack
      fPendingBarlines.pop ();
    }
    
    barlineIsAlright = true;
  }

  else if (
    fCurrentBarlineLocation == msrBarline::kRight
      &&
    fCurrentBarlineRepeatDirection == msrBarline::kBackward) {
    // repeat end
    // ------------------------------------------------------
    
    /*
    Similarly, a backward repeat mark is represented by a right barline at the end of the measure:
    
      <barline location="right">
        <bar-style>light-heavy</bar-style>
        <repeat direction="backward"/>
      </barline>
    */
         
//       if (fMsrOptions->fDebug)
      cerr <<
        idtr << "--> input line " << elt->getInputLineNumber () <<
        endl <<
        idtr <<
        "--> barline, right and backward: repeat end" <<
        endl;

    // set the barline category
    barline->
      setBarlineCategory (msrBarline::kRepeatEnd);

    // append the bar line to the current voice chunk
    fCurrentVoice->
      appendBarlineToVoice (barline);

    if (fPendingBarlines.empty ()) {
//       if (fMsrOptions->fDebug)
      cerr <<
        idtr <<
        "--> there's an implicit repeat start at the beginning of the part" <<
        endl;

      // create the implicit barline
      S_msrBarline
        implicitBarline =
          msrBarline::create (
            fMsrOptions,
            elt->getInputLineNumber (),
            msrBarline::kLeft,
            msrBarline::kHeavyLight,
            msrBarline::kStart,
            fCurrentBarlineEndingNumber,
            msrBarline::kForward,
            fCurrentBarlineRepeatWinged);

      // set the implicit barline category
      implicitBarline->
        setBarlineCategory (
          msrBarline::kRepeatStart);
    
      // prepend the implicit barline to the current voice chunk
      fCurrentVoice->
        prependBarlineToVoice (implicitBarline);
              
      // get the current voice chunk
      S_msrVoicechunk
        currentVoicechunk =
          fCurrentVoice->
            getVoicechunk ();

      if (! fCurrentRepeat) {
        // create the repeat
        if (fMsrOptions->fTrace)
          cerr << idtr <<
            "Creating a repeat in voice " <<
            fCurrentVoice->getVoiceName () << endl;
  
        fCurrentRepeat =
          msrRepeat::create (
            fMsrOptions, elt->getInputLineNumber (),
            currentVoicechunk,
            fCurrentVoice);
      }
      
      // create a new voice chunk for the voice
      if (fMsrOptions->fDebug)
        cerr << idtr <<
          "--> setting new voice chunk for voice " <<
          fCurrentVoice->getVoiceName () << endl;
          
      fCurrentVoice->
        setNewVoicechunkForVoice (
          elt->getInputLineNumber ());

      // add the repeat to the new voice chunk
      if (fMsrOptions->fDebug)
        cerr << idtr <<
          "--> appending the repeat to voice " <<
          fCurrentVoice->getVoiceName () << endl;

      fCurrentVoice->
        appendRepeatToVoice (fCurrentRepeat);

      barlineIsAlright = true;
    }
    
    else {
      // pop the pending barline off the stack
      fPendingBarlines.pop ();
    }
  }

  else if (
    fCurrentBarlineLocation == msrBarline::kLeft
      &&
    fCurrentBarlineEndingType == msrBarline::kStart) {
    // ending start
    // ------------------------------------------------------

//    if (fMsrOptions->fDebug)
      cerr <<
        idtr << "--> input line " <<
          elt->getInputLineNumber () <<
        endl <<
        idtr << "--> measure    " <<
          gCurrentMusicXMLLocation.fMeasureNumber <<
        endl <<
        idtr <<
        "--> barline, left and start: ending start" <<
        endl;

    // set the barline category
    barline->
      setBarlineCategory (msrBarline::kEndingStart);
    
    // append the bar line to the current voice chunk
    fCurrentVoice->
      appendBarlineToVoice (barline);


    // get the current voice chunk
    S_msrVoicechunk
      currentVoicechunk =
        fCurrentVoice->
          getVoicechunk ();

    // push the barline onto the stack
    fPendingBarlines.push (barline);

    barlineIsAlright = true;
  }

  else if (
    fCurrentBarlineLocation == msrBarline::kRight
      &&
    fCurrentBarlineEndingType == msrBarline::kDiscontinue) {
    // hookless ending end
    // ------------------------------------------------------
    
    /*
    The discontinue value is typically used for the last ending in a set,
    where there is no downward hook to mark the end of an ending:
    
      <barline location="right">
        <ending type="discontinue" number="2"/>
      </barline>
    */
//       if (fMsrOptions->fDebug)
      cerr <<
        idtr << "--> input line " << elt->getInputLineNumber () <<
        endl <<
        idtr <<
        "--> barline, right and discontinue: hookless ending end" <<
        endl;

    // set the barline category
    barline->
      setBarlineCategory (msrBarline::kHooklessEndingEnd);
    
    // append the bar line to the current voice chunk
    fCurrentVoice->
      appendBarlineToVoice (barline);

    // get the current voice chunk
    S_msrVoicechunk
      currentVoicechunk =
        fCurrentVoice->
          getVoicechunk ();

    // create a repeat ending from the current voice chunk
    if (fMsrOptions->fDebug)
      cerr << idtr <<
        "--> creating a new hookless repeat ending for voice " <<
        fCurrentVoice->getVoiceName () << endl;
        
    S_msrRepeatending
      repeatEnding =
        msrRepeatending::create (
          fMsrOptions, elt->getInputLineNumber (),
          fCurrentBarlineEndingNumber,
          msrRepeatending::kHooklessEnding,
          currentVoicechunk,
          fCurrentRepeat);

    // add the repeat ending it to the current repeat
    if (fMsrOptions->fDebug)
      cerr << idtr <<
        "--> appending repeat ending to current repeat in voice " <<
        fCurrentVoice->getVoiceName () << endl;
        
    fCurrentRepeat->
      addRepeatending (repeatEnding);

    // create new voice chunk from current voice
    if (fMsrOptions->fDebug)
      cerr << idtr <<
        "--> setting new voice chunk for voice " <<
        fCurrentVoice->getVoiceName () << endl;
        
    fCurrentVoice->
      setNewVoicechunkForVoice (
        elt->getInputLineNumber ());

    // add the repeat to the voice
    if (fMsrOptions->fDebug)
      cerr << idtr <<
        "--> appending the repeat to voice " <<
        fCurrentVoice->getVoiceName () << endl;
    fCurrentVoice->
      appendRepeatToVoice (fCurrentRepeat);

    if (fPendingBarlines.empty ()) {
//       if (fMsrOptions->fDebug)
      cerr <<
        idtr <<
        "--> there's an implicit repeat start at the beginning of the part" <<
        endl;

      // create the implicit barline
      S_msrBarline
        implicitBarline =
          msrBarline::create (
            fMsrOptions,
            elt->getInputLineNumber (),
            msrBarline::kLeft,
            msrBarline::kHeavyLight,
            msrBarline::kStart,
            fCurrentBarlineEndingNumber,
            msrBarline::kForward,
            fCurrentBarlineRepeatWinged);

      // set the implicit barline category
      implicitBarline->
        setBarlineCategory (
          msrBarline::kRepeatStart);
    
      // prepend the implicit barline to the current voice chunk
      fCurrentVoice->
        prependBarlineToVoice (implicitBarline);
              
      // get the current voice chunk
      S_msrVoicechunk
        currentVoicechunk =
          fCurrentVoice->
            getVoicechunk ();

      if (! fCurrentRepeat) {
        // create the repeat
        if (fMsrOptions->fTrace)
          cerr << idtr <<
            "Creating a repeat in voice " <<
            fCurrentVoice->getVoiceName () << endl;
  
        fCurrentRepeat =
          msrRepeat::create (
            fMsrOptions, elt->getInputLineNumber (),
            currentVoicechunk,
            fCurrentVoice);
      }
      
      // create a new voice chunk for the voice
      if (fMsrOptions->fDebug)
        cerr << idtr <<
          "--> setting new voice chunk for voice " <<
          fCurrentVoice->getVoiceName () << endl;
          
      fCurrentVoice->
        setNewVoicechunkForVoice (
          elt->getInputLineNumber ());

      // add the repeat to the new voice chunk
      if (fMsrOptions->fDebug)
        cerr << idtr <<
          "--> appending the repeat to voice " <<
          fCurrentVoice->getVoiceName () << endl;

      fCurrentVoice->
        appendRepeatToVoice (fCurrentRepeat);

      barlineIsAlright = true;
    }
    
    else {
      // pop the pending barline off the stack
      fPendingBarlines.pop ();
    }
    
    barlineIsAlright = true;
  }

  else {

    switch (fCurrentBarlineStyle) {
      
      case msrBarline::kRegular:
      //---------------------------------------
        // don't handle regular barlines specifically,
        // they'll handled later by the software
        // that handles the text output
  
        // set the barline category
        barline->
          setBarlineCategory (msrBarline::kStandaloneBar);
        
        // append the bar line to the current voice chunk
        fCurrentVoice->
          appendBarlineToVoice (barline);
      
        barlineIsAlright = true;
        break;
        
      case msrBarline::kDotted:
      //---------------------------------------
        // set the barline category
        barline->
          setBarlineCategory (msrBarline::kStandaloneBar);
        
        // append the bar line to the current voice chunk
        fCurrentVoice->
          appendBarlineToVoice (barline);
              
        barlineIsAlright = true;
        break;
        
      case msrBarline::kDashed:
      //---------------------------------------    
        // set the barline category
        barline->
          setBarlineCategory (msrBarline::kStandaloneBar);
        
        // append the bar line to the current voice chunk
        fCurrentVoice->
          appendBarlineToVoice (barline);
      
        barlineIsAlright = true;
        break;
        
      case msrBarline::kHeavy:
      //---------------------------------------    
        // set the barline category
        barline->
          setBarlineCategory (msrBarline::kStandaloneBar);
        
        // append the bar line to the current voice chunk
        fCurrentVoice->
          appendBarlineToVoice (barline);
      
        barlineIsAlright = true;
        break;
        
      case msrBarline::kLightLight:
      //---------------------------------------
        // set the barline category
        barline->
          setBarlineCategory (msrBarline::kStandaloneBar);
  
        // append the bar line to the current voice chunk
        fCurrentVoice->
          appendBarlineToVoice (barline);
    
        barlineIsAlright = true;
        break;
        
      case msrBarline::kLightHeavy:
      //---------------------------------------
  
/*
      if (
        fCurrentBarlineLocation == msrBarline::msrBarline::kRight) {
   //       if (fMsrOptions->fDebug)
            cerr <<
              idtr << "--> input line " << elt->getInputLineNumber () <<
              endl <<
              idtr <<
              "--> barline, right:" << endl;
            }
*/

        // set the barline category
        barline->
          setBarlineCategory (msrBarline::kStandaloneBar);
        
        // append the bar line to the current voice chunk
        fCurrentVoice->
          appendBarlineToVoice (barline);
          
        barlineIsAlright = true;        
        break;
  
      case msrBarline::kHeavyLight:
      //---------------------------------------
        // set the barline category
        barline->
          setBarlineCategory (msrBarline::kStandaloneBar);
        
        // append the bar line to the current voice chunk
        fCurrentVoice->
          appendBarlineToVoice (barline);
          
        barlineIsAlright = true;
        break;
         
      case msrBarline::kHeavyHeavy:
      //---------------------------------------    
        // set the barline category
        barline->
          setBarlineCategory (msrBarline::kStandaloneBar);
        
        // append the bar line to the current voice chunk
        fCurrentVoice->
          appendBarlineToVoice (barline);
  
        barlineIsAlright = true;
        break;
        
      case msrBarline::kTick:
      //---------------------------------------
        // set the barline category
        barline->
          setBarlineCategory (msrBarline::kStandaloneBar);
        
        // append the bar line to the current voice chunk
        fCurrentVoice->
          appendBarlineToVoice (barline);
  
        barlineIsAlright = true;
        break;
        
      case msrBarline::kShort:
      //---------------------------------------
        // set the barline category
        barline->
          setBarlineCategory (msrBarline::kStandaloneBar);
        
        // append the bar line to the current voice chunk
        fCurrentVoice->
          appendBarlineToVoice (barline);
  
        barlineIsAlright = true;
        break;
  
      case msrBarline::k_NoStyle:
      //---------------------------------------
        {
          // no <bar-style> has been found
    /*
          / *
          While repeats can have forward or backward direction, endings can have three different type attributes: start, stop, and discontinue. The start value is used at the beginning of an ending, at the beginning of a measure. A typical first ending starts like this:
          
            <barline location="left">
              <ending type="start" number="1"/>
            </barline>
          * /
          if (
            fCurrentBarlineLocation == msrBarline::msrBarline::kLeft
              &&
            fCurrentBarlineEndingType == msrBarline::kStart) {
          }
    
          else if (
            fCurrentBarlineLocation == msrBarline::msrBarline::kRight
              &&
            fCurrentBarlineEndingType == msrBarline::kStop) {
            / *
            The discontinue value is typically used for the last ending in a set,
            where there is no downward hook to mark the end of an ending:
            
            <barline location="right">
              <ending number="2" type="stop"/>
            </barline>
            * /
     //       if (fMsrOptions->fDebug)
              cerr <<
                idtr << "--> input line " << elt->getInputLineNumber () <<
                endl <<
                idtr <<
                "--> barline with right and stop:" << endl <<
                idtr <<
                "    end of an hooked ending" <<
                endl;
      
            // set the barline category
            barline->
              setBarlineCategory (msrBarline::kEndOfAHookedEnding);
            
            // append the bar line to the current voice chunk
            fCurrentVoice->
              appendBarlineToVoice (barline);
  
            barlineIsAlright = true;
          }
          
          else if (
            fCurrentBarlineLocation == msrBarline::msrBarline::kRight
              &&
            fCurrentBarlineEndingType == msrBarline::kDiscontinue) {
          }
        */
        }
    } // switch
  }
  
  // now we can display the barline in case of debug
  if (fMsrOptions->fDebug) {
    cerr << idtr <<
      "Creating a barline in voice " <<
      fCurrentVoice->getVoiceName () << ":" << endl;
    idtr++;
    cerr << idtr << barline;
    idtr--;
  }

  // has this barline been handled?
  if (! barlineIsAlright) {
    stringstream s;
    s << left <<
      "cannot handle a barline containing:" << endl <<
      idtr << "location = " << fCurrentLocation << endl <<
      idtr << "style = " << fCurrentStyle << endl <<
      idtr << "ending type = " << fCurrentEndingtype << endl <<
      idtr << "ending number = " << fCurrentEndingNumber << endl <<
      idtr << "repeat direction = " << fCurrentRepeatDirection << endl <<
      idtr << "repeat winged = " << fCurrentRepeatWinged;
      
    msrMusicXMLError (
      fMsrOptions->fInputSourceName,
      elt->getInputLineNumber (),
      s.str());
  }
}
  
  /*
Repeats and endings are represented by the <repeat> and <ending> elements with a <barline>, as defined in the barline.mod file.

In regular measures, there is no need to include the <barline> element. It is only need to represent repeats, endings, and graphical styles such as double barlines.

A forward repeat mark is represented by a left barline at the beginning of the measure (following the attributes element, if there is one):

  <barline location="left">
    <bar-style>heavy-light</bar-style>
    <repeat direction="forward"/>
  </barline>

The repeat element is what is used for sound generation; the bar-style element only indicates graphic appearance.

Similarly, a backward repeat mark is represented by a right barline at the end of the measure:

  <barline location="right">
    <bar-style>light-heavy</bar-style>
    <repeat direction="backward"/>
  </barline>

While repeats can have forward or backward direction, endings can have three different type attributes: start, stop, and discontinue. The start value is used at the beginning of an ending, at the beginning of a measure. A typical first ending starts like this:

  <barline location="left">
    <ending type="start" number="1"/>
  </barline>

The stop value is used when the end of the ending is marked with a downward hook, as is typical for a first ending. It is usually used together with a backward repeat at the end of a measure:

  <barline location="right">
    <bar-style>light-heavy</bar-style>
    <ending type="stop" number="1"/>
    <repeat direction="backward"/>
  </barline>

The discontinue value is typically used for the last ending in a set, where there is no downward hook to mark the end of an ending:

  <barline location="right">
    <ending type="discontinue" number="2"/>
  </barline>

    */

//______________________________________________________________________________
void xml2MsrVisitor::visitStart ( S_note& elt ) 
{
  //  cerr << "--> xml2MsrVisitor::visitStart ( S_note& elt ) " << endl;
  fMusicXMLNoteData.fMusicXMLStep = '_';
  fMusicXMLNoteData.fMusicXMLStepIsARest = false;
  fMusicXMLNoteData.fMusicXMLStepIsUnpitched = false;
  
  fMusicXMLNoteData.fMusicXMLAlteration = 0; // natural notes
  
  fMusicXMLNoteData.fMusicXMLOctave = -13;
  
  fMusicXMLNoteData.fMusicXMLDivisions = -13;
  fMusicXMLNoteData.fMusicXMLDuration = -13;
  fMusicXMLNoteData.fMusicXMLDotsNumber = 0;
  
  fMusicXMLNoteData.fMusicXMLNoteIsAGraceNote = false;
  
  // assume this note doesn't belong to a chord until S_chord is met
  fMusicXMLNoteData.fMusicXMLNoteBelongsToAChord = false;
  
  // assume this note doesn't belong to a tuplet until S_tuplet is met
  fMusicXMLNoteData.fMusicXMLNoteBelongsToATuplet = false;
  fMusicXMLNoteData.fMusicXMLTupletMemberNoteType = "";
  
  fMusicXMLNoteData.fMusicXMLNoteIsTied = false;
  
  fMusicXMLNoteData.fMusicXMLVoiceNumber = 0;

  // assuming staff number 1, unless S_staff states otherwise afterwards
  fCurrentStaffNumber = 1;

  // assuming voice number 1, unless S_voice states otherwise afterwards
  fCurrentVoiceNumber = 1;
  
  fCurrentStem = "";

  // assume this note hasn't got lyrics until S_lyric is met
  fCurrentNoteHasLyrics = false;
  
  fCurrentBeam = 0;
  
  fCurrentTiedType = "";
  fCurrentTiedOrientation = "";

  fCurrentSlurNumber = -1;
  fCurrentSlurType = "";
  fCurrentSlurPlacement = "";
  fCurrentSlurKind = msrSlur::k_NoSlur;

  fOnGoingNote = true;
}

//______________________________________________________________________________
void xml2MsrVisitor::visitStart ( S_step& elt )
{
  string step = elt->getValue();
  
  if (step.length() != 1) {
    stringstream s;
    s << "step value " << step << " should be a single letter from A to G";
    msrMusicXMLError (
      fMsrOptions->fInputSourceName,
      elt->getInputLineNumber (),
      s.str());
  }

  fMusicXMLNoteData.fMusicXMLStep = step[0];
}

void xml2MsrVisitor::visitStart ( S_alter& elt)
{
  fMusicXMLNoteData.fMusicXMLAlteration = (int)(*elt);
}

void xml2MsrVisitor::visitStart ( S_octave& elt)
{
  fMusicXMLNoteData.fMusicXMLOctave = (int)(*elt);
}

void xml2MsrVisitor::visitStart ( S_duration& elt )
{
  int musicXMLduration = (int)(*elt);

  if (fOnGoingBackup) {
  
    fCurrentBackupDuration = musicXMLduration;
    gCurrentMusicXMLLocation.fPositionInMeasure -=
      fCurrentBackupDuration;
    
  }
  else if (fOnGoingForward) {
  
    fCurrentForwardDuration = musicXMLduration;
    
  }
  else if (fOnGoingNote) {
  
    fMusicXMLNoteData.fMusicXMLDuration = musicXMLduration;
    
  }
  else {
    
    stringstream s;
    s << "duration " << musicXMLduration << " is out of context";
 // JMI   msrMusicXMLError (s.str());
    msrMusicXMLWarning (
      fMsrOptions->fInputSourceName,
      elt->getInputLineNumber (),
      s.str());
  }
    
//  cerr << "=== xml2MsrVisitor::visitStart ( S_duration& elt ), fCurrentMusicXMLDuration = " << fCurrentMusicXMLDuration << endl; JMI
}

void xml2MsrVisitor::visitStart ( S_dot& elt )
{
  fMusicXMLNoteData.fMusicXMLDotsNumber++;
}
       
void xml2MsrVisitor::visitStart ( S_type& elt )
{
  fCurrentNoteType=elt->getValue();
}

void xml2MsrVisitor::visitStart ( S_stem& elt )
{
  //         <stem default-y="28.5">up</stem>

  string        stem = elt->getValue();
  StemDirection stemDirection;
  
  if      (stem == "up")
    stemDirection = kStemUp;
  else if (stem == "down")
    stemDirection = kStemDown;
  else
    stemDirection = kStemNeutral; // JMI

  if (stemDirection != fCurrentStemDirection) {
  // JMI  if (fMsrOptions->fGenerateStems) {
      switch (stemDirection) {
        case kStemNeutral:
          // \stemNeutral JMI
          break;
        case kStemUp:
          // \stemUp JMI
          break;
        case kStemDown:
          // \stemDown JMI
          break;
      } // switch
 //   }
    fCurrentStemDirection = stemDirection;
  }
  
  fCurrentStem = stem;
}

void xml2MsrVisitor::visitStart ( S_beam& elt )
{
/*
Each beam in a note is represented with a separate beam element, starting with the eighth note beam using a number attribute of 1. Note that the beam number does not distinguish sets of beams that overlap, as it does for slur and other elements.
*/
  //        <beam number="1">begin</beam>

  fCurrentBeamValue = elt->getValue();

  fCurrentBeamNumber = 
    elt->getAttributeIntValue ("number", 0);

  bool beamIsOK = true;
  
  msrBeam::msrBeamKind beamKind;

  if (fCurrentBeamValue == "begin") {
    beamKind = msrBeam::kBeginBeam;
  }
  else if (fCurrentBeamValue == "continue") {
    beamKind = msrBeam::kContinueBeam;
  }
  else if (fCurrentBeamValue == "end") {
    beamKind = msrBeam::kEndBeam;
  }
  else {
    stringstream s;
    s <<
      "beam \"" << fCurrentBeamValue <<
      "\"" << "is not handled, ignored";
    msrMusicXMLWarning (
      fMsrOptions->fInputSourceName,
      elt->getInputLineNumber (),
      s.str());

    beamIsOK = false;
  }
    
  if (beamIsOK)
    fCurrentBeam =
      msrBeam::create (
        fMsrOptions,
        elt->getInputLineNumber (),
        fCurrentBeamNumber,
        beamKind);
}

//______________________________________________________________________________
void xml2MsrVisitor::visitStart ( S_staccato& elt )
{
  S_msrArticulation
    articulation =
      msrArticulation::create (
        fMsrOptions,
        elt->getInputLineNumber (),
        msrArticulation::kStaccato);
      
  fCurrentArticulations.push_back (articulation);
}

void xml2MsrVisitor::visitStart ( S_staccatissimo& elt )
{
  S_msrArticulation
    articulation =
      msrArticulation::create (
        fMsrOptions,
        elt->getInputLineNumber (),
        msrArticulation::kStaccatissimo);
      
  fCurrentArticulations.push_back (articulation);
}

void xml2MsrVisitor::visitStart ( S_fermata& elt )
{
  // type : upright inverted  (Binchois20.xml)
  S_msrArticulation
    articulation =
      msrArticulation::create (
        fMsrOptions,
        elt->getInputLineNumber (),
        msrArticulation::kFermata);
      
  fCurrentArticulations.push_back (articulation);
}

  /*

Component   Type  Occurs  Default   Description 
    0..*    
accent  empty-placement   1..1    

The accent element indicates a regular horizontal accent mark.
breath-mark   breath-mark   1..1    

The breath-mark element indicates a place to take a breath.
caesura   empty-placement   1..1    

The caesura element indicates a slight pause. It is notated using a "railroad tracks" symbol.
detached-legato   empty-placement   1..1    

The detached-legato element indicates the combination of a tenuto line and staccato dot symbol.
doit  empty-line  1..1    

The doit element is an indeterminate slide attached to a single note. The doit element appears after the main note and goes above the main pitch.
falloff   empty-line  1..1    

The falloff element is an indeterminate slide attached to a single note. The falloff element appears before the main note and goes below the main pitch.
other-articulation  placement-text  1..1    The other-articulation element is used to define any articulations not yet in the MusicXML format. This allows extended representation, though without application interoperability.
plop  empty-line  1..1    

The plop element is an indeterminate slide attached to a single note. The plop element appears before the main note and comes from above the main pitch.
scoop   empty-line  1..1    

The scoop element is an indeterminate slide attached to a single note. The scoop element appears before the main note and comes from below the main pitch.
spiccato  empty-placement   1..1    

The spiccato element is used for a stroke articulation, as opposed to a dot or a wedge.
staccatissimo   empty-placement   1..1    

The staccatissimo element is used for a wedge articulation, as opposed to a dot or a stroke.
staccato  empty-placement   1..1    

The staccato element is used for a dot articulation, as opposed to a stroke or a wedge.
stress  empty-placement   1..1    

The stress element indicates a stressed note.
strong-accent   strong-accent   1..1    

The strong-accent element indicates a vertical accent mark.
tenuto  empty-placement   1..1    

The tenuto element indicates a tenuto line symbol.
unstress
  */

//______________________________________________________________________________
void xml2MsrVisitor::visitStart( S_f& elt)
{        
  S_msrDynamics
    dyn =
      msrDynamics::create (
        fMsrOptions,
        elt->getInputLineNumber (),
        msrDynamics::kF);
  fPendingDynamics.push_back(dyn);
}
void xml2MsrVisitor::visitStart( S_ff& elt)
{        
  S_msrDynamics
    dyn =
      msrDynamics::create (
        fMsrOptions,
        elt->getInputLineNumber (),
        msrDynamics::kFF);
  fPendingDynamics.push_back(dyn);
}
void xml2MsrVisitor::visitStart( S_fff& elt)
{        
  S_msrDynamics
    dyn =
      msrDynamics::create (
        fMsrOptions,
        elt->getInputLineNumber (),
        msrDynamics::kFFF);
  fPendingDynamics.push_back(dyn);
}
void xml2MsrVisitor::visitStart( S_ffff& elt)
{        
  S_msrDynamics
    dyn =
      msrDynamics::create (
        fMsrOptions,
        elt->getInputLineNumber (),
        msrDynamics::kFFFF);
  fPendingDynamics.push_back(dyn);
}
void xml2MsrVisitor::visitStart( S_fffff& elt)
{        
  S_msrDynamics
    dyn =
      msrDynamics::create (
        fMsrOptions,
        elt->getInputLineNumber (),
        msrDynamics::kFFFFF);
  fPendingDynamics.push_back(dyn);
}
void xml2MsrVisitor::visitStart( S_ffffff& elt)
{        
  S_msrDynamics
    dyn =
      msrDynamics::create (
        fMsrOptions,
        elt->getInputLineNumber (),
        msrDynamics::kFFFFFF);
  fPendingDynamics.push_back(dyn);
}

void xml2MsrVisitor::visitStart( S_p& elt)
{        
  S_msrDynamics
    dyn =
      msrDynamics::create (
        fMsrOptions,
        elt->getInputLineNumber (),
        msrDynamics::kP);
  fPendingDynamics.push_back(dyn);
}
void xml2MsrVisitor::visitStart( S_pp& elt)
{        
  S_msrDynamics
    dyn =
      msrDynamics::create (
        fMsrOptions,
        elt->getInputLineNumber (),
        msrDynamics::kPP);
  fPendingDynamics.push_back(dyn);
}
void xml2MsrVisitor::visitStart( S_ppp& elt)
{        
  S_msrDynamics
    dyn =
      msrDynamics::create (
        fMsrOptions,
        elt->getInputLineNumber (),
        msrDynamics::kPP);
  fPendingDynamics.push_back(dyn);
}
void xml2MsrVisitor::visitStart( S_pppp& elt)
{        
  S_msrDynamics
    dyn =
      msrDynamics::create (
        fMsrOptions,
        elt->getInputLineNumber (),
        msrDynamics::kPPPP);
  fPendingDynamics.push_back(dyn);
}
void xml2MsrVisitor::visitStart( S_ppppp& elt)
{        
  S_msrDynamics
    dyn =
      msrDynamics::create (
        fMsrOptions,
        elt->getInputLineNumber (),
        msrDynamics::kPPPPP);
  fPendingDynamics.push_back(dyn);
}
void xml2MsrVisitor::visitStart( S_pppppp& elt)
{        
  S_msrDynamics
    dyn =
      msrDynamics::create (
        fMsrOptions,
        elt->getInputLineNumber (),
        msrDynamics::kPPPPPP);
  fPendingDynamics.push_back(dyn);
}

//______________________________________________________________________________
void xml2MsrVisitor::visitStart ( S_wedge& elt )
{
  string type = elt->getAttributeValue("type");
  msrWedge::msrWedgeKind wedgeKind;

  if (type == "crescendo") {
    wedgeKind = msrWedge::kCrescendoWedge;
  }
  else if (type == "diminuendo") {
    wedgeKind = msrWedge::kDecrescendoWedge;
  }
  else if (type == "stop") {
    wedgeKind = msrWedge::kStopWedge;
  }
  
  S_msrWedge
    wedge =
      msrWedge::create(
        fMsrOptions,
        elt->getInputLineNumber (),
        wedgeKind);
  fPendingWedges.push_back (wedge);
}
    
//______________________________________________________________________________
void xml2MsrVisitor::visitStart ( S_grace& elt )
{
  fMusicXMLNoteData.fMusicXMLNoteIsAGraceNote = true;;
}
       
//______________________________________________________________________________
void xml2MsrVisitor::visitStart ( S_chord& elt)
{
  fMusicXMLNoteData.fMusicXMLNoteBelongsToAChord = true;
}

//______________________________________________________________________________
void xml2MsrVisitor::visitStart ( S_time_modification& elt )
{
  // there may be no '<tuplet number="n" type="start" />'
  // in the tuplet notes after the first one,
  // so we detect tuplet notes on '<time-modification>'
  // so we detect tuplet notes on '<time-modification>'
  fMusicXMLNoteData.fMusicXMLNoteBelongsToATuplet = true;
}

void xml2MsrVisitor::visitStart ( S_actual_notes& elt )
{
  fCurrentActualNotes = (int)(*elt);
}

void xml2MsrVisitor::visitStart ( S_normal_notes& elt )
{
  fCurrentNormalNotes = (int)(*elt);
}

void xml2MsrVisitor::visitStart ( S_normal_type& elt )
{
  fCurrentNormalNoteType = elt->getValue();
}

void xml2MsrVisitor::visitStart ( S_tuplet& elt )
{
  fCurrentTupletNumber =
    elt->getAttributeIntValue ("number", 0);
    
  string tupletType =
    elt->getAttributeValue("type");
  
  /* JMI*/
  cerr <<
    "--> xml2MsrVisitor::visitStart ( S_tuplet, fCurrentTupletNumber = " <<
    fCurrentTupletNumber << ", tupletType = " << tupletType <<endl;
 // */
  
  fCurrentTupletKind = msrTuplet::k_NoTuplet;
  
  if      (tupletType == "start")
    fCurrentTupletKind = msrTuplet::kStartTuplet;
  else if (tupletType == "continue")
    fCurrentTupletKind = msrTuplet::kContinueTuplet;
  else if (tupletType == "stop")
    fCurrentTupletKind = msrTuplet::kStopTuplet;
  else {
    stringstream s;
    s << "tuplet type " << tupletType << " is unknown";
    msrMusicXMLError (
      fMsrOptions->fInputSourceName,
      elt->getInputLineNumber (),
      s.str());
  }
}

//______________________________________________________________________________
void xml2MsrVisitor::visitStart ( S_rest& elt)
{
  /*
        <note>
        <rest/>
        <duration>24</duration>
        <voice>1</voice>
      </note>
*/
  //  cerr << "--> xml2MsrVisitor::visitStart ( S_rest& elt ) " << endl;
  fMusicXMLNoteData.fMusicXMLStepIsARest = true;
}

//______________________________________________________________________________
void xml2MsrVisitor::visitStart ( S_display_step& elt)
{
  string displayStep = elt->getValue();
  
  if (displayStep.length() != 1) {
    stringstream s;
    s << "sdisplay step value " << displayStep << " should be a single letter from A to G";
    msrMusicXMLError (
      fMsrOptions->fInputSourceName,
      elt->getInputLineNumber (),
      s.str());
  }

  fDisplayStep = displayStep[0];
}

void xml2MsrVisitor::visitStart ( S_display_octave& elt)
{
  fDisplayOctave = (int)(*elt);
}

void xml2MsrVisitor::visitEnd ( S_unpitched& elt)
{
/*
        <unpitched>
          <display-step>E</display-step>
          <display-octave>5</display-octave>
        </unpitched>
*/
  fMusicXMLNoteData.fMusicXMLStepIsUnpitched = true;
  fMusicXMLNoteData.fMusicXMLStep = fDisplayStep;
  fMusicXMLNoteData.fMusicXMLOctave = fDisplayOctave;
}

//______________________________________________________________________________
S_msrChord xml2MsrVisitor::createChordFromCurrentNote ()
{
  if (fMsrOptions->fDebug)
    cerr << idtr <<
      "--> creating a chord on its 2nd note" <<
      fCurrentNote <<
      endl;
  
  // fCurrentNote has been registered standalone in the part element sequence,
  // but it is actually the first note of a chord
  
  // create a chord
  S_msrChord chord;
  
  chord =
    msrChord::create (
      fMsrOptions,
      fCurrentNote->getInputLineNumber (),
      fCurrentNote->getNoteMsrDuration ());
// JMI  fCurrentElement = chord; // another name for it
   
  if (fMsrOptions->fDebug)
    cerr << idtr <<
      "--> adding first note " << fCurrentNote->noteMsrPitchAsString() <<
      " to new chord" << endl;
    
  // register fCurrentNote as first member of chord
  chord->addNoteToChord (fCurrentNote);
  fCurrentNote->setNoteBelongsToAChord ();

  // move the pending articulations if any from the first note to the chord
  list<S_msrArticulation>
    noteArticulations =
      fCurrentNote->getNoteArticulations ();

  if (! noteArticulations.empty()) {
    if (fMsrOptions->fDebug)
      cerr << idtr <<
        "--> moving articulations from current note to chord" << endl;
        
    while (! noteArticulations.empty()) {
      S_msrArticulation
        art = noteArticulations.front();
      chord->addArticulation (art);
      noteArticulations.pop_front ();
    } // while
  }
  
  // move the pending dynamics if any from the first note to the chord
  list<S_msrDynamics>
    noteDynamics =
      fCurrentNote->getNoteDynamics();
    
  if (! noteDynamics.empty()) {
    if (fMsrOptions->fDebug)
      cerr << idtr <<
        "--> moving dynamics from current note to chord" << endl;
        
    while (! noteDynamics.empty()) {
      S_msrDynamics
        dyn = noteDynamics.front();
      chord->addDynamics (dyn);
      noteDynamics.pop_front ();
    } // while
  }
 
  // move the pending wedges if any from the first note to the chord
  list<S_msrWedge>
    noteWedges =
      fCurrentNote->getNoteWedges();
    
  if (! noteWedges.empty()) {
    if (fMsrOptions->fDebug)
      cerr << idtr <<
        "--> moving wedges from current note to chord" << endl;
        
    while (! noteWedges.empty()) {
      S_msrWedge
        wdg = noteWedges.front();
      chord->addWedge (wdg);
      noteWedges.pop_front();
    } // while
  }
  
  return chord;
}

//______________________________________________________________________________
void xml2MsrVisitor::createTupletWithItsFirstNote (S_msrNote note)
{
  // fCurrentNote is the first tuplet note,
  // and is currently at the end of the voice

  if (fMsrOptions->fDebug)
    cerr << idtr <<
      "xml2MsrVisitor::createTupletWithItsFirstNote " <<
      note <<
      endl;
      
  // create a tuplet
  S_msrTuplet
    tuplet =
      msrTuplet::create(
        fMsrOptions,
        note->getInputLineNumber ());
// JMI  fCurrentElement = tuplet; // another name for it

  // populate it
  tuplet->updateTuplet (
    fCurrentTupletNumber,
    fCurrentActualNotes,
    fCurrentNormalNotes);

  // register it in this visitor
  if (fMsrOptions->fDebug)
    cerr << idtr <<
      "--> pushing tuplet to tuplets stack" << endl;
  fCurrentTupletsStack.push(tuplet);
  
  // add note as first note of the tuplet
  if (fMsrOptions->fDebug)
    cerr << idtr <<
      "--> adding note " << note->noteMsrPitchAsString() <<
      " as first note of the tuplet" << endl;
  tuplet->addElementToTuplet (note);
}

//______________________________________________________________________________
void xml2MsrVisitor::finalizeTuplet (S_msrNote note)
{
  if (fMsrOptions->fDebug)
    cerr << idtr <<
      "xml2MsrVisitor::finalizeTuplet " <<
      note <<
      endl;
      
  // get tuplet from top of tuplet stack
  S_msrTuplet tup = fCurrentTupletsStack.top();

  // add note to the tuplet
  if (fMsrOptions->fDebug)
    cerr << idtr <<
      "--> adding note " << note->noteMsrPitchAsString () <<
      " to tuplets stack top" << endl;
  tup->addElementToTuplet(note);

  // pop from the tuplets stack
  if (fMsrOptions->fDebug)
    cerr << idtr <<
      "--> popping from tuplets stack" << endl;
  fCurrentTupletsStack.pop();        

  // add tuplet to current voice
  if (fMsrOptions->fDebug)
    cerr << idtr <<
      "=== adding tuplet to the part sequence" << endl;
  fCurrentVoice->
    appendTupletToVoice (tup);
}          

//______________________________________________________________________________
void xml2MsrVisitor::attachPendingDynamicsAndWedgesToNote (
  S_msrNote note)
{
  // attach the pending dynamics if any to the note
  if (! fPendingDynamics.empty()) {
    if (fMusicXMLNoteData.fMusicXMLStepIsARest) {
      if (fMsrOptions->fDelayRestsDynamics) {
      cerr << idtr <<
        "--> Delaying dynamics attached to a rest until next note" << endl;
      } else {
        msrMusicXMLWarning (
          fMsrOptions->fInputSourceName,
          note->getInputLineNumber (),
          "there is dynamics attached to a rest");
      }
    } else {
      while (! fPendingDynamics.empty()) {
        S_msrDynamics
          dyn =
            fPendingDynamics.front();
        note->addDynamics (dyn);
        fPendingDynamics.pop_front();
      } // while
    }
  }
  
  // attach the pending wedges if any to the note
  if (! fPendingWedges.empty()) {
    if (fMusicXMLNoteData.fMusicXMLStepIsARest) {
      if (fMsrOptions->fDelayRestsDynamics) {
      cerr << idtr <<
        "--> Delaying wedge attached to a rest until next note" << endl;
      } else {
        for (
            list<S_msrWedge>::const_iterator i = fPendingWedges.begin();
            i != fPendingWedges.end();
            i++) {
          msrMusicXMLWarning (
            fMsrOptions->fInputSourceName,
            (*i)->getInputLineNumber (),
            "there is a wedge attached to a rest");
        } // for
      }
    } else {
      while (! fPendingWedges.empty()) {
        S_msrWedge
          wdg =
            fPendingWedges.front();
        note->addWedge (wdg);
        fPendingWedges.pop_front();
      } // while
    }
  }
}

//______________________________________________________________________________
void xml2MsrVisitor::visitEnd ( S_note& elt )
{
  /*
  This is a complex method, due to the fact that
  dynamics, wedges, chords and tuplets
  are not ordered in the same way in MusicXML and LilyPond.

  Staff number is analyzed before voice number but occurs
  after it in the MusicXML tree.
  That's why the treatment below has been postponed until this method
  */

  /*
  Staff assignment is only needed for music notated on multiple staves.
  Used by both notes and directions.
  Staff values are numbers, with 1 referring to the top-most staff in a part.
  */
  
//  if (true || fMsrOptions->fDebug)
  if (fMsrOptions->fDebugDebug)
    cerr <<
      idtr <<
      "!!!! BEFORE visitEnd (S_note) we have:" << endl <<
      idtr << idtr <<
      "--> fCurrentStaffNumber = " << fCurrentStaffNumber << endl <<
      idtr << idtr <<
      "--> current staff name  = " << fCurrentStaff->getStaffName() << endl <<
      idtr << idtr <<
      "--> fCurrentVoiceNumber = " << fCurrentVoiceNumber << endl;

  // is fCurrentStaffNumber already present in fCurrentPart?
  fCurrentStaff =
    fCurrentPart->
      fetchStaffFromPart (fCurrentStaffNumber);

  if (! fCurrentStaff) 
    // no, add it to the current part
    fCurrentStaff =
      fCurrentPart->
        addStaffToPart (
          elt->getInputLineNumber (), fCurrentStaffNumber);
    
  // fetch the note's voice in the current staff
  fCurrentVoice =
    fCurrentStaff->
      fetchVoiceFromStaff (fCurrentVoiceNumber);

/* JMI*/
  // no, add it to the current staff
  if (! fCurrentVoice) 
    fCurrentVoice =
      fCurrentStaff->
        addVoiceToStaff (
          elt->getInputLineNumber (), fCurrentVoiceNumber);
/* */

  // store voice number in MusicXML note data
  fMusicXMLNoteData.fMusicXMLVoiceNumber = fCurrentVoiceNumber;

  fCurrentStemDirection = kStemNeutral;
  
  if (fMsrOptions->fDebugDebug)
    cerr << idtr <<
      "fMusicXMLNoteData.fMusicXMLDuration = " << 
      fMusicXMLNoteData.fMusicXMLDuration << ", " << 
      "fCurrentMusicXMLDivisions*4 = " <<
      fCurrentMusicXMLDivisions*4 << endl;
      
  fMusicXMLNoteData.fMusicXMLDivisions =
    fCurrentMusicXMLDivisions;
  fMusicXMLNoteData.fMusicXMLTupletMemberNoteType =
    fCurrentNoteType;
  
  //cerr << "::: creating a note" << endl;
  S_msrNote
    note =
      msrNote::createFromMusicXMLData (
        fMsrOptions,
        elt->getInputLineNumber (),
        fMusicXMLNoteData,
        fCurrentSlurKind);

  if (fCurrentBeam)
    note->
      setBeam (fCurrentBeam);

  // attach the articulations if any to the note
  while (! fCurrentArticulations.empty()) {
    S_msrArticulation
      art =
        fCurrentArticulations.front();
    note->
      addArticulation (art);
    fCurrentArticulations.pop_front();
  } // while
   
  attachPendingDynamicsAndWedgesToNote (note);

  /*
  A note can be standalone
  or a member of a chord,
  and the latter can belong to a tuplet.
  
  A rest can be standalone or belong to a tuplet
  */
  
  if (fMusicXMLNoteData.fMusicXMLNoteBelongsToAChord) {

    handleNoteBelongingToAChord (note);

  }
  else if (fMusicXMLNoteData.fMusicXMLNoteBelongsToATuplet) {

    handleNoteBelongingToATuplet (note);
    
  }
  else { // standalone note/rest

    handleStandaloneNoteOrRest (note);
    
  }

  // keep track of note/rest in this visitor
  fCurrentNote = note;
  gCurrentMusicXMLLocation.fPositionInMeasure +=
    fCurrentNote->
      getNoteMusicXMLDuration ();
    
// JMI  fCurrentElement = fCurrentNote; // another name for it

//  if (true || fMsrOptions->fDebug)
  if (fMsrOptions->fDebugDebug)
    cerr <<
      idtr <<
      "!!!! AFTER visitEnd (S_note) " << fCurrentNote->noteMsrPitchAsString () <<
      " we have:" << endl <<
      idtr << idtr <<
      "--> fCurrentStaffNumber = " << fCurrentStaffNumber << endl <<
      idtr << idtr <<
      "--> current staff name  = " << fCurrentStaff->getStaffName() << endl <<
      idtr << idtr <<
      "--> fCurrentVoiceNumber = " << fCurrentVoiceNumber << endl <<
      idtr << idtr <<
      "--> fCurrentVoice        = " << fCurrentVoice->getVoiceName() << endl;

  fOnGoingNote = false;
}

//______________________________________________________________________________
void xml2MsrVisitor::handleStandaloneNoteOrRest (
  S_msrNote newNote)
{
  if (fMsrOptions->fDebugDebug)
    cerr << idtr <<
      "xml2MsrVisitor::handleStandaloneNoteOrRest " <<
      newNote <<
      endl;

  if (fMusicXMLNoteData.fMusicXMLStepIsARest)
    newNote->
      setNoteKind (msrNote::kRestNote);
  else
    newNote->
      setNoteKind (msrNote::kStandaloneNote);
      
  // register note/rest as standalone
//  if (true || fMsrOptions->fDebugDebug)
  if (fMsrOptions->fDebugDebug)
    cerr <<  idtr <<
      "--> adding standalone " <<
      newNote->noteMsrPitchAsString () <<
      ":" << newNote->getNoteMsrDuration () <<
      " to current voice" << endl;

  // is voice fCurrentVoiceNumber present in current staff?
  fCurrentVoice =
    fCurrentStaff->
      fetchVoiceFromStaff (fCurrentVoiceNumber);

  if (! fCurrentVoice)
    // no, add it to the staff
    fCurrentVoice =
      fCurrentStaff->
        addVoiceToStaff (
          newNote->getInputLineNumber (), fCurrentVoiceNumber);
    
  fCurrentVoice->
    appendNoteToVoice (newNote);

/* JMI
  if (! fCurrentNoteHasLyrics)
    // lyrics have to be handled anyway JMI
    handleLyricsText (newNote->getInputLineNumber ());
*/

  // account for chord not being built
  fOnGoingChord = false;
}

//______________________________________________________________________________
void xml2MsrVisitor::handleNoteBelongingToAChord (
  S_msrNote newNote)
{
  if (fMsrOptions->fDebug)
    cerr << idtr <<
      "xml2MsrVisitor::handleNoteBelongingToAChord " <<
      newNote <<
      endl;
      
  if (fMusicXMLNoteData.fMusicXMLStepIsARest)
    msrMusicXMLError (
      fMsrOptions->fInputSourceName,
      newNote->getInputLineNumber (),
      "a rest cannot belong to a chord");

  newNote->
    setNoteKind (msrNote::kChordMemberNote);

  if (! fOnGoingChord) {
    // create a chord with fCurrentNote as its first note
    fCurrentChord =
      createChordFromCurrentNote ();

    // account for chord being built
    fOnGoingChord = true;
  }
  
  if (fMsrOptions->fDebug)
    cerr << idtr <<
      "--> adding new note " <<
      newNote->noteMsrPitchAsString() <<
      " to current chord" << endl;
    
  // register note as a member of fCurrentChord
  if (fMsrOptions->fDebug)
    cerr << idtr <<
      "--> registering new note " <<
      newNote->noteMsrPitchAsString() <<
      " as a member of current chord" << endl;
  fCurrentChord->
    addNoteToChord (newNote);

  // set note as belonging to a chord
  newNote->setNoteBelongsToAChord ();
    
  // remove previous current note or the previous state of the chord
  // from the current voice sequence
  if (fMsrOptions->fDebug)
    cerr << idtr <<
      "--> removing last element " <<
// JMI ???      fCurrentVoice->getVoiceSequentialMusicLastElement () <<
      " from current voice" << endl;
  fCurrentVoice->
    removeLastElementFromVoice ();

  // add fCurrentChord to the part sequence instead
  if (fMsrOptions->fDebug)
    cerr << idtr <<
      "--> appending chord " << fCurrentChord <<
      " to current voice" << endl;
  fCurrentVoice->
    appendChordToVoice (fCurrentChord);
}

//______________________________________________________________________________
void xml2MsrVisitor::handleNoteBelongingToATuplet (
  S_msrNote note)
{
  if (fMsrOptions->fDebug)
    cerr << idtr <<
      "xml2MsrVisitor::handleNoteBelongingToATuplet " <<
      note <<
      endl;
        
  note->
    setNoteKind (msrNote::kTupletMemberNote);

  switch (fCurrentTupletKind) {
    case msrTuplet::kStartTuplet:
      {
        createTupletWithItsFirstNote (note);
        fOnGoingTuplet = true;
      
        // swith to continuation mode
        // this is handy in case the forthcoming tuplet members
        // are not explictly of the "continue" type
        fCurrentTupletKind = msrTuplet::kContinueTuplet;
      }
      break;

    case msrTuplet::kContinueTuplet:
      {
        // populate the tuplet at the top of the stack
        if (fMsrOptions->fDebug)
          cerr << idtr <<
            "--> adding note " << note <<
            " to tuplets stack top" << endl;
        fCurrentTupletsStack.top()->
          addElementToTuplet (note);
      }
      break;

    case msrTuplet::kStopTuplet:
      {
        finalizeTuplet (note);

        // indicate the end of the tuplet
        fOnGoingTuplet = false;
      }
      break;

    case msrTuplet::k_NoTuplet:
      break;
  } // switch
}

//______________________________________________________________________________
void xml2MsrVisitor::handleLyricsText (
  int inputLineNumber)
{
//  if (true || fMsrOptions->fDebug) {
  if (fMsrOptions->fDebug) {
    cerr <<
      idtr <<
        "Handling lyrics on:" << endl <<
        fMusicXMLNoteData <<
      idtr <<
        "fCurrentText = \"" << fCurrentText <<
        "\":" << fMusicXMLNoteData.fMusicXMLDuration <<
        ", fCurrentElision = " << fCurrentElision << endl <<
      idtr <<
        "  fMusicXMLNoteData.fMusicXMLStepIsARest = ";
    if (fMusicXMLNoteData.fMusicXMLStepIsARest)
      cerr << "true";
    else
      cerr << "false";
    cerr << endl;

    cerr <<
      idtr <<
        "  fMusicXMLNoteData.fMusicXMLNoteIsTied  = ";
    if (fMusicXMLNoteData.fMusicXMLNoteIsTied)
      cerr << "true";
    else
      cerr << "false";
    cerr << endl;
    
    cerr <<
      idtr <<
        "  fCurrentLyricschunkType                = \"";
    switch (fCurrentLyricschunkType) {
      case msrLyricschunk::kSingleChunk:
        cerr << "single";
        break;
      case msrLyricschunk::kBeginChunk:
        cerr << "begin";
        break;
      case msrLyricschunk::kMiddleChunk:
        cerr << "middle";
        break;
      case msrLyricschunk::kEndChunk:
        cerr << "end";
        break;
      case msrLyricschunk::kSkipChunk:
        cerr << "skip";
        break;
      case msrLyricschunk::kSlurChunk:
        cerr << "slur";
        break;
      case msrLyricschunk::kTiedChunk:
        cerr << "tied";
        break;
      case msrLyricschunk::kBreakChunk:
        cerr << "break";
        break;
      case msrLyricschunk::k_NoChunk:
        cerr << "NO_CHUNK";
        break;
    } // switch
    cerr << "\"" << endl;
    
    cerr <<
      idtr <<
        "  fCurrentSlurKind                       = \"";
    switch (fCurrentSlurKind) {
      case msrSlur::kStartSlur:
        cerr << "start";
        break;
      case msrSlur::kContinueSlur:
        cerr << "start";
        break;
      case msrSlur::kStopSlur:
        cerr << "start";
        break;
      case msrSlur::k_NoSlur:
        cerr << "NO_SLUR";
        break;
    } // switch
    cerr << "\"" << endl;
  }

  //* JMI
  // is lyrics fCurrentLyricsNumber present in current voice?
  fCurrentLyrics =
    fCurrentVoice->
      fetchLyricsFromVoice (fCurrentLyricsNumber);

  //* JMI
  if (! fCurrentLyrics)
    // no, add it to the voice JMI ???
    fCurrentLyrics =
      fCurrentVoice->
        addLyricsToVoice (
          inputLineNumber, fCurrentLyricsNumber);

  S_msrDuration
    lyricMsrDuration =
      msrDuration::create (
        fMsrOptions,
        inputLineNumber,
        fMusicXMLNoteData.fMusicXMLDuration,
        fCurrentMusicXMLDivisions,
        fMusicXMLNoteData.fMusicXMLDotsNumber,
        fMusicXMLNoteData.fMusicXMLTupletMemberNoteType);

  msrLyricschunk::msrLyricschunkType
    chunkTypeToBeCreated =
      msrLyricschunk::k_NoChunk;

  if (fMusicXMLNoteData.fMusicXMLStepIsARest)
    chunkTypeToBeCreated = msrLyricschunk::kSkipChunk;

  else {

    switch (fCurrentLyricschunkType) {
      case msrLyricschunk::kSingleChunk:
        chunkTypeToBeCreated = fCurrentLyricschunkType;
        break;
      case msrLyricschunk::kBeginChunk:
        chunkTypeToBeCreated = fCurrentLyricschunkType;
        break;
      case msrLyricschunk::kMiddleChunk:
        chunkTypeToBeCreated = fCurrentLyricschunkType;
        break;
      case msrLyricschunk::kEndChunk:
        chunkTypeToBeCreated = fCurrentLyricschunkType;
        break;

 //     case msrLyricschunk::k_NoChunk:
      default:
        {
        if (fMusicXMLNoteData.fMusicXMLNoteIsTied)
          chunkTypeToBeCreated = msrLyricschunk::kTiedChunk;
          
        else {
          switch (fCurrentSlurKind) {
            case msrSlur::kStartSlur:
              chunkTypeToBeCreated = msrLyricschunk::kSingleChunk;
              break;
            case msrSlur::kContinueSlur:
              chunkTypeToBeCreated = msrLyricschunk::kSlurChunk;
              break;
            case msrSlur::kStopSlur:
              chunkTypeToBeCreated = msrLyricschunk::kSlurChunk;
              break;
    
            default:
              break;
          } // switch
        }
        }
        break;
    } // switch
  }

  switch (chunkTypeToBeCreated) {
    
    case msrLyricschunk::kSkipChunk:
      fCurrentLyrics->
        addSkipChunkToLyrics (
          inputLineNumber,
          lyricMsrDuration);
      break;

    case msrLyricschunk::kSlurChunk:
      fCurrentLyrics->
        addSlurChunkToLyrics (
          inputLineNumber,
          lyricMsrDuration);
      break;

    case msrLyricschunk::kTiedChunk:
      fCurrentLyrics->
        addTiedChunkToLyrics (
          inputLineNumber,
          lyricMsrDuration);
      break;

    case msrLyricschunk::kSingleChunk:
    case msrLyricschunk::kBeginChunk:
    case msrLyricschunk::kMiddleChunk:
    case msrLyricschunk::kEndChunk:
      fCurrentLyrics->
        addTextChunkToLyrics (
          inputLineNumber,
          fCurrentSyllabic,
          chunkTypeToBeCreated,
          fCurrentText,
          fCurrentElision,
          lyricMsrDuration);
      break;
      
    default: // JMI
      break;

  } // switch
 } // handleLyricsText


} // namespace
