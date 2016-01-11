#include "sync.h"
#include "topo.h"
#include "shm.h"
#include "mp.h"

/**
 * \brief
 *
 */
uintptr_t ab_forward(uintptr_t val, coreid_t sender)
{
    debug_printfff(DBG__HYBRID_AC, "hybrid_ac entered\n");

    coreid_t my_core_id = get_thread_id();
    bool does_mp = topo_does_mp_receive(my_core_id, false) ||
        (topo_does_mp_send(my_core_id, false));

    // Sender forwards message to the root
    if (get_thread_id()==sender) {

        mp_send(get_sequentializer(), val);
    }
    
    // Message passing
    // --------------------------------------------------
    if (does_mp) {

        debug_printfff(DBG__HYBRID_AC, "Starting message passing .. %d %d\n",
                       topo_does_mp_receive(my_core_id, false),
                       topo_does_mp_send(my_core_id, false));

        if (my_core_id == SEQUENTIALIZER) {
            val = mp_receive(sender);
            mp_send_ab(val);
        } else {
            val = mp_receive_forward(0);
        }
    }

    // Shared memory
    // --------------------------------------------------
    if (topo_does_shm_send(my_core_id) || topo_does_shm_receive(my_core_id)) {

        if (topo_does_shm_send(my_core_id)) {

            // send
            // --------------------------------------------------
            
            debug_printfff(DBG__HYBRID_AC, "Starting SHM .. -- send, \n");
            
            shm_send(val, 0, 0, 0, 0, 0, 0);
        }
        if (topo_does_shm_receive(my_core_id)) {

            // receive
            // --------------------------------------------------
            
            debug_printfff(DBG__HYBRID_AC, "Starting SHM .. -- receive, \n");

            uintptr_t val1, val2, val3, val4, val5, val6, val7;
            shm_receive(&val1, &val2, &val3, &val4, &val5, &val6, &val7);
        }

        debug_printfff(DBG__HYBRID_AC, "End SHM\n");
    }

    return val;

}
