/***************************************************************************
                          hosts.h  -  description
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

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <iostream>
#include <sstream>
#include <string>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <qd/qd_real.h>
#include <stdio.h>
#include <stdlib.h>
using std::string;
using namespace std;

/**
  *@author Patrik Carlsson, Anders Ekberg
  */

class hosts {
private:
  hosts* next;

  string hostPair;     // A string containing SRC:DST pair, 
  int pktCounter;         // Match Counter

  qd_real firstPkt;    // Arrival time of first packet
  qd_real latestPkt;   // Arrival time of latest packet

  qd_real interPktTime;          // Internal packet interarrival time (used to calc mean)
  qd_real minimum_inter_PktTime; // minimum
  qd_real maximum_inter_PktTime; // maximum

  double pktSize;                // Internal packet size counter
  int minPktSize;                // Min packet size
  int maxPktSize;                // Max packet size

public: 
  hosts();
  hosts(string pair, qd_real aTime, int pduSize);

  ~hosts();

  hosts* getNext();
  void setNext(hosts*);

  void debugPrint(void);
  void insert(string pair, qd_real aTime, int pduSize) ;
  void Reset(void);
  int match(string pair);
  int count(void);

  friend ostream &operator<<(ostream &outp, hosts &ts);
  string print(void);
  string printMe(void);
  string printHosts(void);
  string printCounters(void);
};

#endif
