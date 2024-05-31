#include "rdt.h"
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
struct pkt A_packet;
int A_sendseq;
int B_acknum;

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
  int i, checksum = 0;
  for (i = 0; i < 20; ++i)
  {
    checksum += message.data[i];
    A_packet.payload[i] = message.data[i];
  }
  A_packet.seqnum = A_sendseq;
  A_packet.acknum = 0;
  A_packet.ack = 1;
  A_packet.checksum = checksum + A_packet.seqnum + A_packet.acknum + A_packet.ack;

  starttimer(A, send_time_MAX);
  tolayer3(A, A_packet);
}

void B_output(struct msg message) /* need be completed only for extra credit */
{
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
  int checksum = 0, i;
  for (i = 0; i < 20; ++i)
    checksum += packet.payload[i];
  checksum += (packet.ack + packet.acknum + packet.seqnum);
  if (checksum == packet.checksum && packet.ack)
  {
    A_sendseq += (packet.acknum - A_sendseq);
    stoptimer(A);
  }
  else if (!packet.ack || packet.acknum != A_sendseq + 1 || checksum != packet.checksum)
  {
    stoptimer(A);
    starttimer(A, send_time_MAX);
    tolayer3(A, A_packet);
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  starttimer(A, send_time_MAX);
  tolayer3(A, A_packet);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  A_sendseq = 0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
  int i, checksum = 0;
  for (i = 0; i < 20; ++i)
    checksum += packet.payload[i];
  checksum += (packet.seqnum + packet.acknum + packet.ack);
  struct pkt *p = (struct pkt *)malloc(sizeof(struct pkt));
  struct pkt send_packet = *p;
  for (i = 0; i < 20; ++i)
    send_packet.payload[i] = '\0';
  if (checksum == packet.checksum)
  {
    if (B_acknum < packet.seqnum + 1)
      tolayer5(B, packet.payload);
    send_packet.seqnum = 1;
    send_packet.acknum = packet.seqnum + 1;
    send_packet.ack = 1;
    B_acknum = send_packet.acknum;
    send_packet.checksum = send_packet.seqnum + send_packet.acknum + send_packet.ack;
  }
  else
  {
    send_packet.seqnum = 1;
    send_packet.acknum = packet.seqnum + 1;
    send_packet.ack = 0;
    send_packet.checksum = send_packet.seqnum + send_packet.acknum + send_packet.ack;
  }
  tolayer3(B, send_packet);
}

/* called when B's timer goes off */
void B_timerinterrupt()
{
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  B_acknum = -1;
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