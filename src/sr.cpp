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
int aseq , window_size , baseA , baseB , backA , cur_pos;
float timer_time = 35.0;

deque<pkt> all_pack; // holds all of the packets sent from layer 5 to A
deque<pkt> timer_packets;//stores the packets in order from time they are sent
deque<float> timer_times;// stores the times for each packet in ^^
deque<int> ack_rec;
deque<int> packet_rec;

deque<pkt> rec_buffer; // buffer for out of order packets at B


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
  all_pack.push_back(packet);
  ack_rec.push_back(0);// push back that we havent rec the ack for this packet yet

  //if there are currently no packets using the timer start it normally
  //and the packet falls within the window
  if(timer_packets.size() == 0){
    if(packet.seqnum > backA)
    backA = packet.seqnum;// set the new back packet
    tolayer3(0 , packet);
    timer_packets.push_back(packet); // push it into the timer queue
    timer_times.push_back(get_sim_time());//push the time it was sent into the time sent tracker
    starttimer(0,timer_time);
  }
  //if there is at least one packet already queued up we dont start the timer
  //and its within the window size we queue it for the timer
  else if(timer_packets.size() > 0 && (packet.seqnum - baseA) < window_size){
    if(packet.seqnum > backA)
    backA = packet.seqnum;//set the new back packet
    tolayer3(0 , packet);
    timer_packets.push_back(packet); // push it into the timer queue
    timer_times.push_back(get_sim_time());//push the time it was sent into the time sent tracker
  }

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
  //compute the checksum
  stoptimer(0);
  int sum = 0;

  for(int i = 0 ; i < 20 ; i++){
    if(packet.payload[i] == '\0')
      break;
     sum += packet.payload[i];
   }

  sum += packet.seqnum;
  sum += packet.acknum;

  // go through all of the currently sent packets to find if this ack goes with any
  auto t = timer_times.begin();
  for(auto i = timer_packets.begin() ; i != timer_packets.end() ; i++ ){
    // if the ack matches
    if(i->checksum == sum && i->seqnum == packet.acknum){
      //if were dealing with the front
      ack_rec.at(i->seqnum) = 1;// weve recieved the ack for this packet
      int base_num = i->seqnum;

      //if the ack was for the baseA increase the base and send any packet that now falls within the window
      if(i->seqnum == baseA){
        int num_moved = 0;
        // we got the base, now move it incase their were out of order acks recieved
        while(all_pack.size() - baseA >= window_size &&  ack_rec.at(baseA) == 1){
    //      cout << "HERE1\n";
          baseA++;
          num_moved++;
        }
        //if we have more packets that fall within the window send em
        if(all_pack.size() - baseA >= window_size){
          int new_moved = 0;
          //we havent sent the proper number of new packets or run out of packet or window space
          float current_time = get_sim_time();
          while(new_moved < num_moved && (backA +1) < all_pack.size()){
  //          cout << "HERE2\n";
            timer_packets.push_back(all_pack.at(backA+1));
            timer_times.push_back(current_time);
            backA++;
            new_moved++;
          }

        }
      }

       // remove the acked packet from the timer lists
        timer_packets.erase(i);
        timer_times.erase(t);

//        cout << "HERE3\n";
      break;
    }
    t++;
  }

  //if there are more packets sent, set the timer for the next one in line with the appropriate timer
  if(timer_packets.size() != 0)
  starttimer(0 , (timer_time-(get_sim_time()-timer_times.front())));

}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  //get the front packet cause it need to go back into the list
  struct pkt packet = timer_packets.front();
  timer_packets.pop_front();
  timer_times.pop_front();

  //cout << timer_times.size() << endl;

  //resend the packet again and then start the timer for the next packet in the list
  tolayer3(0 ,packet);
  timer_packets.push_back(packet);
  timer_times.push_back(get_sim_time());
  //if there is only one packet ie the same packet start the timer with our chosen timer
  //else we set the timer for the next packet in the list
  if(timer_packets.size() == 1)
  starttimer(0 , timer_time);
  else
  starttimer(0 , (timer_time-(get_sim_time()-timer_times.front())));
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  window_size = getwinsize();
  aseq = 0;
  baseA = 0;// the current base of the window at A
  backA = -1;// the current spot of the last packet in the window, ie the highest seqnum
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

  if(sum == packet.checksum && packet.seqnum < (baseB+window_size) && packet_rec.at(packet.seqnum) == -1){
    rec_buffer.push_back(packet);
    cout << "HERE4\n";
    packet_rec.at(packet.seqnum) = cur_pos;
    cout << "HERE5\n";
    cur_pos++;
    if(packet.seqnum == baseB){
      //if we got the current base of the receiver send it to layer 5
      tolayer5(1,packet.payload);
      baseB++;
      //take care of any out of order buffered packets
      while(packet_rec.at(baseB) != -1){
        tolayer5(1 , rec_buffer.at(packet_rec.at(baseB)).payload);
        baseB++;
      }
    }
    //send the ack for the packet
    cout << "HERE6\n";
    struct pkt back;
    back.seqnum = packet.acknum;
    back.acknum = packet.seqnum;
    for(int i = 0 ; i < 20 ; i++){
      if(packet.payload[i] == '\0')
        break;
       back.payload[i] = packet.payload[i];
     }
    back.checksum = sum;

    tolayer3(1 , back);
  }

}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  cur_pos = 0;
  baseB = 0; //the current base of the window at B
  packet_rec.resize(1000 ,-1); // at each seq number if its -1 it hasn't been rec, if not zero
                           // it holds the postion in th buffer at which the packet is held
}
