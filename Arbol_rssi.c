/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         Testing the broadcast layer in Rime
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include <stdio.h>

/*############################################################################
Librerias propias
############################################################################*/
//#include "tree_n_ary.h"
#include "Arbol_lib.h"

/*############################################################################*/
//Estructura de datos
/*############################################################################*/
static struct beacon parent_data;
//static tree_n_ary *nodo_ary;
/* Rango en milisegundos */
#define MIN_MS   125
#define MAX_MS  4000
/*---------------------------------------------------------------------------*/
PROCESS(send_beacon, "Broadcast example");

//OCESS(unicast_msg, "envia unicast al padre" );
PROCESS(select_parent, "selecciona un padre basandose en mejor RSSI" );
//OCESS(unicast_rtx, "proceso para rtx unicast" );

// Nuevos procesos 

//PROCESS(send_routing_table,"Envia tabla de enrutamiento actual al padre");
//PROCESS(update_routing_table,"Actualiza tabla de enrutamiento");
//PROCESS(generate_one_one_msg,"Envia mensaje 1 a 1");
//PROCESS(rtx_one_one,"Reenvia mensaje 1-1");




AUTOSTART_PROCESSES(&send_beacon, &select_parent);
//,&unicast_rtx  &unicast_msg,
  //&send_routing_table, &update_routing_table, &generate_one_one_msg, &rtx_one_one);
/*############################################################################
Funcion de recepcion Broadcast
############################################################################*/
static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{

  if(linkaddr_node_addr.u8[0]!= 1){
        // leer beacon recibido
        struct beacon beacon_rcv = *(struct beacon   *)packetbuf_dataptr();
        // Evaluar si es diferente de uno, lo que indica que ya tiene ruta hacia AC
        //printf("Rssi Nodo anterior: %d. ",beacon_rcv.rssi_c);
        if(beacon_rcv.rssi_c != 1){
          //sumar el RSSI del enlace
          beacon_rcv.rssi_c += packetbuf_attr(PACKETBUF_ATTR_RSSI);
        }

        printf("\nMensaje Beacon recibido del nodo %d.%d: ID = %d, RSSI_ACUMULADO = %d.\n",
                from->u8[0], 
                from->u8[1], 
                beacon_rcv.id.u8[0], 
                beacon_rcv.rssi_c);


        add_parent(&beacon_rcv);

        process_post(&select_parent, PROCESS_EVENT_CONTINUE,NULL);
    }
}

/*############################################################################*/
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
/*############################################################################
Process send_beacon
############################################################################*/
PROCESS_THREAD(send_beacon, ev, data)
{
  // Las variables van antes del process PROCESS_BEGIN
  // estas instrucciones corren una vez cuando el proceso inicia
  static struct etimer et;

  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
  PROCESS_BEGIN();
//Inicializa un objeto de broadcast (broadcast) en el canal 129.
  broadcast_open(&broadcast, 129, &broadcast_call);

  static int16_t flag = 0;
  if(flag==0){
    flag=1;
    // Inicializar padre como 0 e inicializar valor de RSSI_a
    cond_1_neighbor_discovery(&linkaddr_node_addr,&parent_data);

  }
  
  while(1) {
    uint16_t rand_ms = (random_rand() % (MAX_MS - MIN_MS + 1)) + MIN_MS;
    clock_time_t jitter = (rand_ms * CLOCK_SECOND) / 1000;
    /* Delay 2-4 seconds */
    etimer_set(&et, jitter);
    //etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    llenar_beacon(&b,linkaddr_node_addr,parent_data.rssi_c);
    //print_beacon(&b);

    packetbuf_copyfrom(&b, sizeof(struct beacon));
    broadcast_send(&broadcast);
    printf("broadcast message sent with %d\n",parent_data.rssi_c);
  }

  PROCESS_END();

}

/*############################################################################
Process select_parent Se activa cada vez que llega un mensaje de broadcast
 permite que cada nodo elija el mejor nodo padre en función de la calidad de 
 la señal RSSI. 
############################################################################*/
PROCESS_THREAD(select_parent, ev, data){

  PROCESS_BEGIN();

  while(1) {
      PROCESS_WAIT_EVENT();
      if(linkaddr_node_addr.u8[0]!= 1){
        update_parent(&parent_data);
        printf("El parent es el nodo: %d.%d con un RSSI de:%d dB\n",
          parent_data.id.u8[0],
          parent_data.id.u8[1],
          parent_data.rssi_c);
      }
  }

  PROCESS_END();
}