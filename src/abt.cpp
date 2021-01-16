#include "../include/simulator.h"
#include <vector>
#include <stdio.h>
#include <string.h>
#include <iostream>

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
float timer_time = 30.0; // the delay used with the timer, can change this to get better/worse simulation

int aseq , bseq;// our sequence and ack bit, these flip in this protocol
int  back;
int  sent_to_5;
int cur_index , ack_rec;// ack_rec usde to keep track of whether the current packet has been acknowlegded or not

vector<pkt> buffer; // Used to buffer if packets are sent to quick,FIFO thus front will be the "current"


/* called from layer 5, passed the data to be sent to other side */

//NOTE use to tolayer3(0 -> A , 1 -> B) here to send to B
void A_output(struct msg message)
{

  struct pkt  packet ;
  (packet).seqnum = aseq;
  (packet).acknum = aseq;
  strcpy((packet).payload , message.data);

  //compute the checksum
  int sum = 0;

  for(int i = 0 ; i < 20 ; i++){
    if(message.data[i] == '\0')
      break;
     sum += message.data[i];
   }

  sum += aseq;
  sum += aseq;

  (packet).checksum = sum;

  buffer.push_back((packet));


  // our buffer check and a bound check
  // if the ack has yet to be recieved do nothing
  if(ack_rec && cur_index < buffer.size() ){
      cout << "SENDING FROM A TO 3\n";
      ack_rec = 0;
      tolayer3(0 ,  buffer.at(cur_index));
      starttimer(0 , timer_time);

}

}

/* called from layer 3, when a packet arrives for layer 4 */
//NOTE this is where the incoming ack/etc will come from B
void A_input(struct pkt packet)
{
   stoptimer(0);
  int sum = 0;


  for(int i = 0 ; i < 20 ; i++){
    if(packet.payload[i] == '\0')
      break;
     sum += packet.payload[i];
   }

  sum += bseq;
  sum += back;
  //if we recieve the correct numbers
  if((packet.acknum == back) && (sum == packet.checksum)){

    ack_rec = 1;
    sent_to_5 = 0;
    cur_index++;

    // Trivial way to flip the bits used between 1 and 0
    aseq = 1 - aseq;
    bseq = 1 -bseq;
    back = 1 -back;

  }
  else{
    //if the ack was bad ( i.e. corrupted) resend the packet

      starttimer(0 , timer_time);
      tolayer3(0 , buffer.at(cur_index));
  }

}

/* called when A's timer goes off */
void A_timerinterrupt()
{

  starttimer(0 , timer_time);
  tolayer3(0 , buffer.at(cur_index));

}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{

  aseq = 0;
  ack_rec = 1; // acknowlegded flag, if this is 0, the current packet has not been acked yet

  //the index of the packet in the buffer vector we are working on
  cur_index = 0;

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

  if((sum == packet.checksum) && packet.seqnum == back){

    if(sent_to_5 == 0){
      sent_to_5 = 1;
    tolayer5(1 , packet.payload);
  }
    struct pkt  Back ;
    (Back).seqnum = bseq;
    (Back).acknum = back;
    sum -= aseq;
    sum -= aseq;
    sum += bseq;
    sum += back;
    (Back).checksum = sum;
    strcpy(Back.payload , packet.payload);
    tolayer3(1 , (Back));
  }

}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  sent_to_5 = 0; // used to make sure the same packet isn't delivered to layer 5 more than once

  bseq = 1; // the sequence number loaded into the packet from B to A
  back = 0; // the ack number from B to A

}
