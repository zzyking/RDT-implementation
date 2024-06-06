#include "gbn.h"
/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

const float send_time_MAX = 20.0;
int base, nextseq;
struct pkt buf[50];
int last_received_seq;

int calculate_checksum(struct pkt packet)
{
  int checksum = 0, i;
  for (i = 0; i < 20; ++i)
    checksum += packet.payload[i];
  checksum += packet.seqnum + packet.acknum;
  return checksum;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
  if(nextseq < base + WINDOW_SIZE)
  {
    struct pkt sent_packet;
    sent_packet.seqnum = nextseq;
    sent_packet.acknum = 0;
    for(int i = 0; i < 20; i++)
      sent_packet.payload[i] = message.data[i];

    sent_packet.checksum = calculate_checksum(sent_packet);

    buf[nextseq] = sent_packet;

    tolayer3(A, sent_packet);

    if(base == nextseq)
      starttimer(A, send_time_MAX);
    nextseq++;
  }
}

void B_output(struct msg message) /* need be completed only for extra credit */
{
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet){
  if (packet.checksum == calculate_checksum(packet)){
    if(packet.acknum == 1){
      if(packet.seqnum >= base){
        base = packet.seqnum + 1;
        if(base == nextseq)
          stoptimer(A);
        else{
          stoptimer(A);
          starttimer(A, send_time_MAX);
        }
      }
      printf("          ACK received\n");
      printf("          seqnum: %d, acknum: %d\n", packet.seqnum, packet.acknum);
    }
    else if (packet.acknum == -1){
      for(int i = base; i < nextseq; i++)
        tolayer3(A, buf[i]);
      starttimer(A, send_time_MAX);
      printf("          NAK received\n");
      printf("          seqnum: %d, acknum: %d\n", packet.seqnum, packet.acknum);
    }
  }
  else{
    printf("          Corrupted ACK/NAK packet received\n");
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  for(int i = base; i < nextseq; i++)
    tolayer3(A, buf[i]);
  starttimer(A, send_time_MAX);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  nextseq = 0;
  base = 0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
  int is_corrupted = 0, is_inorder = 0;
  if(packet.checksum != calculate_checksum(packet))
    is_corrupted = 1;
  struct pkt sent_packet;
  sent_packet.acknum = (is_corrupted) ? -1 : 1;

  if(packet.seqnum == last_received_seq+1)
    is_inorder = 1;
  sent_packet.seqnum = (is_inorder) ? packet.seqnum : last_received_seq;

  sent_packet.checksum = calculate_checksum(sent_packet);
  
  tolayer3(B, sent_packet);
  if(is_inorder){
    last_received_seq++;
    tolayer5(B, packet.payload);
  }
}

/* called when B's timer goes off */
void B_timerinterrupt()
{
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  last_received_seq = -1;
}

int main()
{
  struct event *eventptr;
  struct msg msg2give;
  struct pkt pkt2give;

  int i, j;
  char c;

  init();
  A_init();
  B_init();

  while (1)
  {
    eventptr = evlist; /* get next event to simulate */
    if (eventptr == NULL)
      goto terminate;
    evlist = evlist->next; /* remove this event from event list */
    if (evlist != NULL)
      evlist->prev = NULL;
    if (TRACE >= 2)
    {
      printf("\nEVENT time: %f,", eventptr->evtime);
      printf("  type: %d", eventptr->evtype);
      if (eventptr->evtype == 0)
        printf(", timerinterrupt  ");
      else if (eventptr->evtype == 1)
        printf(", fromlayer5 ");
      else
        printf(", fromlayer3 ");
      printf(" entity: %d\n", eventptr->eventity);
    }
    time = eventptr->evtime; /* update time to next event time */
    // if (nsim == nsimmax)
    //   break; /* all done with simulation */
    if (eventptr->evtype == FROM_LAYER5 && nsim < nsimmax)
    {
      generate_next_arrival(); /* set up future arrival */
      /* fill in msg to give with string of same letter */
      j = nsim % 26;
      for (i = 0; i < 20; i++)
        msg2give.data[i] = 97 + j;
      if (TRACE > 2)
      {
        printf("          MAINLOOP: data given to student: ");
        for (i = 0; i < 20; i++)
          printf("%c", msg2give.data[i]);
        printf("\n");
      }
      nsim++;
      if (eventptr->eventity == A)
        A_output(msg2give);
      else
        B_output(msg2give);
    }
    else if (eventptr->evtype == FROM_LAYER3)
    {
      pkt2give.seqnum = eventptr->pktptr->seqnum;
      pkt2give.acknum = eventptr->pktptr->acknum;
      pkt2give.checksum = eventptr->pktptr->checksum;
      for (i = 0; i < 20; i++)
        pkt2give.payload[i] = eventptr->pktptr->payload[i];
      if (eventptr->eventity == A) /* deliver packet by calling */
        A_input(pkt2give);         /* appropriate entity */
      else
        B_input(pkt2give);
      free(eventptr->pktptr); /* free the memory for packet */
    }
    else if (eventptr->evtype == TIMER_INTERRUPT)
    {
      if (eventptr->eventity == A)
        A_timerinterrupt();
      else
        B_timerinterrupt();
    }
    else
    {
      printf("INTERNAL PANIC: unknown event type \n");
    }
    free(eventptr);
  }

terminate:
  printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n", time, nsim);
}