/***************************************************************************
                          protocol.cpp  -  description
                             -------------------
    begin                : Thu Apr 17 2003
    copyright            : (C) 2003 by Patrik Carlsson, Anders Ekberg
    email                : patrik.carlsson@its.bth.se, anders.ekberg@bth.se
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "hosts.h"

hosts::hosts(){
  hostPair="";
  next=0;

  pktCounter=0;
  firstPkt=0.0;
  latestPkt=0.0;

  interPktTime=0.0;
  minimum_inter_PktTime=10e6;
  maximum_inter_PktTime=0.0;

  pktSize=0;
  minPktSize=100000;
  maxPktSize=0;

}

hosts::hosts(string pair, qd_real aTime, int pduSize){
  next=0;

  hostPair=pair;
  firstPkt=aTime;
  pktCounter=1;
  latestPkt=aTime;

  minimum_inter_PktTime=10e6;
  maximum_inter_PktTime=0.0;

  pktSize=pduSize;
  maxPktSize=pduSize;
  minPktSize=pduSize;

  return;
}

hosts::~hosts(){
  if(next!=0)
    delete next;
  next=0;
  //  printf("[%s (removed)]", hostPair.c_str());
}

void hosts::Reset(void){
  pktCounter=0;
  firstPkt=0.0;
  latestPkt=0.0;

  interPktTime=0.0;
  minimum_inter_PktTime=10e6;
  maximum_inter_PktTime=0.0;

  pktSize=0;
  minPktSize=100000;
  maxPktSize=0;

  if(next!=0){
    next->Reset();
  }
  return;
}

void hosts::debugPrint(){
 if(next!=0)
    next->debugPrint();

}

hosts* hosts::getNext(){
  return next;
}

void hosts::setNext(hosts* n){
  next=n;
  return;
}
int hosts::match(string pair){
  if(hostPair==pair){
    return(1);
  }
  return(0);
}

int hosts::count(void){
  return pktCounter;
}







void hosts::insert(string pair, qd_real aTime, int pduSize){
  qd_real diff;

  if(hostPair==pair){ // We are at the correct entry, add the statistics and exit.
    if(pktCounter==0){
      firstPkt=aTime;
    }
    pktCounter++;
    latestPkt=aTime;
    diff=latestPkt-firstPkt;
    interPktTime+=diff;
    if(diff>maximum_inter_PktTime) {
      maximum_inter_PktTime=diff;
    }
    if(diff<minimum_inter_PktTime) {
      minimum_inter_PktTime=diff;
    }
    pktSize+=pduSize;
    if(pduSize>maxPktSize) {
      maxPktSize=pduSize;
    }
    if(pduSize<minPktSize) {
      minPktSize=pduSize;
    }
    return;
  }

  if(next==0) {
    next=new hosts(pair,aTime,pduSize);
  } else {
    next->insert(pair,aTime,pduSize);
  }
  return;        
}


ostream &operator<<(ostream &outp, hosts &ts){
  hosts *tmp=&ts;

  outp << tmp->hostPair << ":" << tmp->pktCounter << endl;
  

  if(tmp->getNext()!=0) {
    outp << tmp->getNext();
  }
  return outp;
  

}

string hosts::print(void){
  ostringstream oss;
  oss << "*:" << hostPair << ":" << pktCounter << "\n";

  string myString;
  myString = oss.str();

  if(next!=0){
    myString=myString+next->print();
  }
  return myString;
}

string hosts::printMe(void){
  ostringstream oss;
  //  oss << "[" << hostPair << " ," << pktCounter << "]";
  oss << hostPair << " " << pktCounter; 

  string myString;
  myString = oss.str();

  return myString;
}

string hosts::printHosts(void){
  
  ostringstream oss;
  oss << hostPair;
  string myString;
  myString = oss.str();
  
  if(next!=0) {
      myString = myString +  "," + next->printHosts();
  }
  return myString;
}

string hosts::printCounters(void){
  ostringstream oss;
  oss << pktCounter;

  string myString;
  myString = oss.str();

  if(next!=0){
    myString = myString + "," + next->printCounters();
  }
  return myString;
}
