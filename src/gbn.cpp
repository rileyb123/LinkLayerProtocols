#include "../include/simulator.h"
#include <vector>
#include <stdio.h>
#include <iostream>
#include <deque>
#include <string.h>

using namespace std;


/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

int window_size;
int aseq , bexpected , send_base , total_resend , total_packets;
float timer_time = 30.0;
int buffer_limit = 1000;

deque<pkt> Apackets ; // Buffer for packets at A, Used deque for these for easy front/back access

struct pkt B_in_order; // simply stores the current highest in order packet recieved at B

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
  //First we make the new packet
  struct pkt  packet ;
  (packet).seqnum = aseq;
  (packet).acknum = aseq;

  //compute the checksum
  int sum = 0;

  for(int i = 0 ; i < 20 ; i++){
    if(message.data[i] == '\0')
      break;
     sum += message.data[i];
     packet.payload[i] = message.data[i]; // had to change from strcpy to this to fix a segfault
   }

  sum += aseq;
  sum += aseq;
  aseq = aseq + 1; // after using this seqNum move to the next seqNum

  (packet).checksum = sum;
  Apackets.push_back(packet);

  //If the # number of buffered packets is to high just quit
  if((Apackets.size()- send_base) > buffer_limit )
    exit(0);

  //If the packet is the base one
  if(packet.seqnum == send_base){
    //  cout << "ADDED FIRST BASE\n" ;
      tolayer3(0 , packet);
      starttimer(0,timer_time);

  }
  //if the packet isn't the base but falls within the window still send it, just no timer
  else if((packet.seqnum - send_base) < window_size){
  //  cout << "NOT BASE BUT STILL SENT\n";
    tolayer3( 0 ,packet );
  }

  //If it reaches here without entering an if, then the packet is simply buffered


}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{

  stoptimer(0);
  int sum = 0;

  for(int i = 0 ; i < 20 ; i++){
    if(packet.payload[i] == '\0')
      break;
     sum += packet.payload[i];
   }

  sum += packet.seqnum;
  sum += packet.acknum;

//  cout << "BEFORE ACK CHECK\n";
  //check that the recieved packet is correct for what ever acknum it is, ignore duplicate acks(ie the ack is below the base)
  if((packet.acknum >= 0) && (packet.acknum < Apackets.size()) && (packet.acknum == Apackets.at(packet.acknum).seqnum)
  && (sum == Apackets.at(packet.acknum).checksum) && (packet.acknum >= send_base)){


  //from the base up to (including) the ack num, move the base up one, remove a total resend value
  for(int i = send_base ; i <= packet.acknum ; i++){
  //  cout << "BASE" << send_base << endl;
    send_base++;
  }



//  cout << "END OF ACK CHECK\n";
  }
  //if there are still packets in the buffer to be dealt with start the timer again so they will get sent out
  if((Apackets.size() - send_base) != 0)
  starttimer(0,timer_time);
}

/* called when A's timer goes off */
void A_timerinterrupt()
{


  //Resend all packets that you have up the window size
  int total_remian = Apackets.size() - send_base;
  int count = 0;
  // resend packets up to either the end of the buffer or window size
  while(count < total_remian && count < window_size){
  //  cout << "IN THE INTERUPT\n";
    tolayer3(0 , Apackets.at(send_base+count));
    count++;
  }

    starttimer(0,timer_time);

}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  window_size = getwinsize();//get the window size of the current simulation
  aseq = 0; //The next Sequence number we are on, increases as we send to B
  send_base = 0; // The base seq number for the window
  total_resend = 0 ; //the numer of packets to resend in event of a time out
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{

  int sum = 0;

  for(int i = 0 ; i < 20 ; i++){
    if(packet.payload[i] == '\0')
      break;
     sum += packet.payload[i];
   }

   sum += packet.seqnum;
   sum += packet.acknum;

   //if its the packet were expecting
   if(bexpected == packet.seqnum && sum == packet.checksum){

     bexpected++;

     tolayer5(1,packet.payload);
     //make our ack, the seq is the next
     struct pkt back;
     back.seqnum = packet.acknum;
     back.acknum = packet.seqnum;
     strcpy(back.payload , packet.payload);
     back.checksum = sum;

     tolayer3(1 , back);
     B_in_order = back;
   }

   else if(packet.seqnum <  bexpected)
     tolayer3(1 , B_in_order);


}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{

  bexpected = 0; // The expected sequence number
}
