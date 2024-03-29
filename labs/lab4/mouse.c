#include <lcom/lcf.h>
#include "mouse.h"
#include "kbc.h"
#include "macros/error.h"

/// --------------------------------------------------------------------------------
///                                     Settings
/// --------------------------------------------------------------------------------



/// --------------------------------------------------------------------------------
///                                  Interruptions
/// --------------------------------------------------------------------------------

int mouse_hook_id = MOUSE_IRQ_LINE;

int mouse_subscribe_int(uint16_t *bit_no) {
    kbc_write_mouse_byte(SET_STREAM_MODE);
    kbc_write_mouse_byte(ENABLE_DATA_REPORTING);
    *bit_no = BIT(mouse_hook_id);
    error(sys_irqsetpolicy(MOUSE_IRQ_LINE, IRQ_REENABLE | IRQ_EXCLUSIVE, &mouse_hook_id),
          "Error on mouse interruptions subscription");
    return 0;
}

int mouse_unsubscribe_int() {
    error(sys_irqrmpolicy(&mouse_hook_id), "Error on timer interruptions unsubscription");
    kbc_write_mouse_byte(DISABLE_DATA_REPORTING);
    return 0;
}

struct mouse_packet {
    bool y_overflow;
    bool x_overflow;
    bool middle_button;
    bool right_button;
    bool left_button;
    uint16_t x_delta;
    uint16_t y_delta;
    uint8_t bytes[3];
};

uint8_t packet_counter = 0;
uint8_t packet_bytes_count = 0;
uint8_t read_bytes[3];
struct mouse_packet last_packet;

uint32_t mouse_get_last_packet() {
    return (last_packet.bytes[2] << 16) | (last_packet.bytes[1] << 8) | last_packet.bytes[0];
}

uint8_t mouse_get_packet_counter(){
    return packet_counter;
}

int read_byte() {
    uint16_t packet_byte;
    if(kbc_read_return(&packet_byte))
        return 1;
    read_bytes[0] = read_bytes[1];
    read_bytes[1] = read_bytes[2];
    read_bytes[2] = (uint8_t) packet_byte;
    return 0;
}

bool create_packet(struct packet *pckt, uint8_t bytes[3]){
    //if(!valid_packet(bytes))
    //    return false;
    for(unsigned i = 0; i < 3; ++i){
        pckt->bytes[i] = bytes[i];
    }
    pckt->rb = (bytes[0] & MOUSE_RIGHT_BUTTON);
    pckt->mb = (bytes[0] & MOUSE_MIDDLE_BUTTON);
    pckt->lb = (bytes[0] & MOUSE_LEFT_BUTTON);
    pckt->delta_x = bytes[1];
    if(bytes[0] & MOUSE_MSB_X_DELTA)
        pckt->delta_x -= 256;
    pckt->delta_y = bytes[2];
    if(bytes[0] & MOUSE_MSB_Y_DELTA)
        pckt->delta_y -= 256;
    pckt->x_ov = bytes[0] & MOUSE_X_OVERFLOW;
    pckt->y_ov = bytes[0] & MOUSE_Y_OVERFLOW;
    return true;
}

void display_last_packet(){
    struct packet last_packet;
    create_packet(&last_packet, read_bytes);
    mouse_print_packet(&last_packet);
}

void (mouse_ih)() {
    packet_bytes_count++;
    if (read_byte())
        return;
    if(packet_bytes_count < 3)
        return;
    display_last_packet();
    packet_bytes_count = 0;
    packet_counter++;
}
