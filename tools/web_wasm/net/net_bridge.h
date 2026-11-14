/* Online play: the game's side of the WebRTC transport (tools/web_wasm/web/net.js).
 *
 * Members are identified by their lobby member id (> 0). Messages are byte strings
 * whose first byte is the message type, 0x00-0xEF (0xF0-0xFF belong to the
 * transport). Two channel kinds per peer:
 *   PORT_NET_UNRELIABLE  unordered, no retransmits (state streams)
 *   PORT_NET_RELIABLE    ordered, retransmitted (events, control)
 * Incoming messages and transport events are queued in JS and pulled with
 * port_net_recv(), typically once per game frame until it returns 0. Events arrive
 * with kind PORT_NET_EVENT and an 8-byte payload: u8 code (PORT_NET_EV_*), 3 bytes
 * 0, u32 member id (little-endian).
 *
 * The session (started by the host in the lobby) is described by
 * port_net_session_json(): {"code","host","self","settings":{...host game
 * settings...},"members":[{"id","name","slot","profile":{...}}]}.
 * All functions are safe to call without a page or session (node, offline): they
 * then report "no session" and drop what is sent. */
#ifndef MKDD_NET_BRIDGE_H
#define MKDD_NET_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

#define PORT_NET_UNRELIABLE 0
#define PORT_NET_RELIABLE 1
#define PORT_NET_EVENT 2
#define PORT_NET_ALL (-1)

#define PORT_NET_EV_JOIN 1  /* peer's channels are open */
#define PORT_NET_EV_LEAVE 2 /* peer left the room or was removed */
#define PORT_NET_EV_LOST 3  /* peer silent for 5 s / connection interrupted */
#define PORT_NET_EV_BACK 4  /* ... and back */
#define PORT_NET_EV_START 5 /* the host started the session */
#define PORT_NET_EV_END 6   /* the host ended it */
#define PORT_NET_EV_HOST 7  /* the member id is the new host */

/* 1 while a session runs (the host pressed Start in the lobby). */
int port_net_active(void);
/* Own member id, the host's member id (0 without a room). */
int port_net_self(void);
int port_net_host(void);
/* Member ids of the session (or room), up to max; returns the count (self included). */
int port_net_members(int *ids, int max);
/* Slot the host gave a member (0-7), -1 for none/unknown. */
int port_net_slot(int member);
/* Smoothed round trip time to a member in ms (0 for self, -1 unknown). */
int port_net_rtt(int member);
/* The host's clock in ms (shared race clock; on the host its own clock) and whether
 * the estimate has samples yet. */
double port_net_host_time(void);
int port_net_clock_synced(void);
/* Send len bytes to a member or PORT_NET_ALL. Returns the number of peers it went
 * to (0: nobody connected), -1 on a bad message (empty, type >= 0xF0). */
int port_net_send(int to, int kind, const void *data, int len);
/* Pull the next message: returns its length and fills *from / *kind, 0 when the queue
 * is empty, or -length (message kept) when cap is too small. */
int port_net_recv(int *from, int *kind, void *buf, int cap);
/* Session description as JSON (see above); returns its length, 0 without a session,
 * or -length when cap is too small. */
int port_net_session_json(char *buf, int cap);

#ifdef __cplusplus
}
#endif

#endif
