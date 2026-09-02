#ifndef CERO_BRIDGE_H
#define CERO_BRIDGE_H

/* Spawns the local TCP bridge thread (detached) and waits for it to bind,
 * exactly as main() used to: start thread, then sleep 100ms so
 * local_bridge_port is populated before single_instance_write_port(). */
void bridge_start(void);

#endif /* CERO_BRIDGE_H */
